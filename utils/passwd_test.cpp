// Copyright 2010, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// * Redistributions of source code must retain the above copyright
//   notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the
//   documentation and/or other materials provided with the distribution.
// * Neither the name of Google Inc. nor the names of its contributors
//   may be used to endorse or promote products derived from this software
//   without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

extern "C" {
#include <sys/wait.h>

#include <pwd.h>
#include <unistd.h>
}

#include <stdexcept>

#include <atf-c++.hpp>

#include "utils/passwd.hpp"

namespace passwd_ns = utils::passwd;


ATF_TEST_CASE_WITHOUT_HEAD(user__public_fields);
ATF_TEST_CASE_BODY(user__public_fields)
{
    const passwd_ns::user user("the-name", 1, 2);
    ATF_REQUIRE_EQ("the-name", user.name);
    ATF_REQUIRE_EQ(1, user.uid);
    ATF_REQUIRE_EQ(2, user.gid);
}


ATF_TEST_CASE_WITHOUT_HEAD(user__is_root__true);
ATF_TEST_CASE_BODY(user__is_root__true)
{
    const passwd_ns::user user("i-am-root", 0, 10);
    ATF_REQUIRE(user.is_root());
}


ATF_TEST_CASE_WITHOUT_HEAD(user__is_root__false);
ATF_TEST_CASE_BODY(user__is_root__false)
{
    const passwd_ns::user user("i-am-not-root", 123, 10);
    ATF_REQUIRE(!user.is_root());
}


ATF_TEST_CASE_WITHOUT_HEAD(current_user);
ATF_TEST_CASE_BODY(current_user)
{
    const passwd_ns::user user = passwd_ns::current_user();
    ATF_REQUIRE_EQ(::getuid(), user.uid);
    ATF_REQUIRE_EQ(::getgid(), user.gid);
}


ATF_TEST_CASE_WITHOUT_HEAD(current_user__fake);
ATF_TEST_CASE_BODY(current_user__fake)
{
    const passwd_ns::user new_user("someone-else", ::getuid() + 1, 0);
    passwd_ns::set_current_user_for_testing(new_user);

    const passwd_ns::user user = passwd_ns::current_user();
    ATF_REQUIRE(::getuid() != user.uid);
    ATF_REQUIRE_EQ(new_user.uid, user.uid);
}


ATF_TEST_CASE(drop_privileges);
ATF_TEST_CASE_HEAD(drop_privileges)
{
    set_md_var("require.config", "unprivileged-user");
    set_md_var("require.user", "root");
}
ATF_TEST_CASE_BODY(drop_privileges)
{
    ATF_REQUIRE_EQ(0, ::getuid());
    ATF_REQUIRE_EQ(0, ::getgid());

    const passwd_ns::user new_user = passwd_ns::find_user_by_name(
        get_config_var("unprivileged-user"));

    const pid_t pid = ::fork();
    ATF_REQUIRE(pid != -1);
    if (pid == 0) {
        // We need to do this in a subprocess because we require the test case
        // to be able to write to its results file on exit (and to do so we need
        // to maintain the original credentials).
        passwd_ns::drop_privileges(new_user);
        if (::getuid() == 0 || ::getgid() == 0)
            ::exit(EXIT_FAILURE);
        else
            ::exit(EXIT_SUCCESS);
    }

    int status;
    ATF_REQUIRE(::waitpid(pid, &status, 0) != -1);
    if (!WIFEXITED(status) || WEXITSTATUS(status) != EXIT_SUCCESS)
        fail("Looks like drop_privileges() did not do the right thing");
}


ATF_TEST_CASE_WITHOUT_HEAD(find_user_by_name__ok);
ATF_TEST_CASE_BODY(find_user_by_name__ok)
{
    const struct ::passwd* pw = ::getpwuid(::getuid());
    ATF_REQUIRE(pw != NULL);

    const passwd_ns::user user = passwd_ns::find_user_by_name(pw->pw_name);
    ATF_REQUIRE_EQ(::getuid(), user.uid);
    ATF_REQUIRE_EQ(::getgid(), user.gid);
    ATF_REQUIRE_EQ(pw->pw_name, user.name);
}


ATF_TEST_CASE_WITHOUT_HEAD(find_user_by_name__fail);
ATF_TEST_CASE_BODY(find_user_by_name__fail)
{
    ATF_REQUIRE_THROW_RE(std::runtime_error, "Failed.*user 'i-do-not-exist'",
                         passwd_ns::find_user_by_name("i-do-not-exist"));
}


ATF_TEST_CASE_WITHOUT_HEAD(find_user_by_uid);
ATF_TEST_CASE_BODY(find_user_by_uid)
{
    const passwd_ns::user user = passwd_ns::find_user_by_uid(::getuid());
    ATF_REQUIRE_EQ(::getuid(), user.uid);
    ATF_REQUIRE_EQ(::getgid(), user.gid);

    const struct ::passwd* pw = ::getpwuid(::getuid());
    ATF_REQUIRE(pw != NULL);
    ATF_REQUIRE_EQ(pw->pw_name, user.name);
}


ATF_INIT_TEST_CASES(tcs)
{
    ATF_ADD_TEST_CASE(tcs, user__public_fields);
    ATF_ADD_TEST_CASE(tcs, user__is_root__true);
    ATF_ADD_TEST_CASE(tcs, user__is_root__false);

    ATF_ADD_TEST_CASE(tcs, current_user);
    ATF_ADD_TEST_CASE(tcs, current_user__fake);

    ATF_ADD_TEST_CASE(tcs, drop_privileges);

    ATF_ADD_TEST_CASE(tcs, find_user_by_name__ok);
    ATF_ADD_TEST_CASE(tcs, find_user_by_name__fail);
    ATF_ADD_TEST_CASE(tcs, find_user_by_uid);
}
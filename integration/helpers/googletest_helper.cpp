#include <gtest/gtest.h>

TEST(sum, passes_eq) {
    ASSERT_EQ(1 + 1, 2);
}

TEST(sum, passes_eq) {
    EXPECT_EQ(1 + 1, 2);
}

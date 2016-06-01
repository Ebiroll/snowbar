#include "gtest/gtest.h"

TEST(StringTest,sillyTest)
{
  char *test="This is just a silly example fail";
  EXPECT_STREQ("This is just a silly example",test) << "This is supposed to fail" ;


}


TEST(MathTest, TwoPlusTwoEqualsFour) {
        int i=17;
        EXPECT_EQ(2 + 2, 4) << "Text with better explanation" ;
        EXPECT_TRUE(i==17) << "i was not 17 it was " << i;
}

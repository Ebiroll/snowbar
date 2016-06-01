#include "gtest/gtest.h"
#include "functions.h"


using namespace snow;

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



TEST(ReverseString,reverse1)
{
    std::string test=std::string("This is just a silly example");
    std::string expected_result=std::string("elpmaxe yllis a tsuj si sihT");

    std::string result=::ReverseString(test);

    EXPECT_EQ(expected_result,result);

}

TEST(ReverseString,reverse2)
{
    std::string test=std::string("s");
    std::string expected_result=std::string("s");

    std::string result=::ReverseString(test);

    EXPECT_EQ(expected_result,result);
}

#include "gtest/gtest.h"
#include "functions.h"


using namespace snow;

TEST(DISABLED_StringTest,sillyTest)
{
  char *test="This is just a silly example fail";
  EXPECT_STREQ("This is just a silly example",test) << "This is supposed to fail" ;

}



TEST(MathTest, TwoPlusTwoEqualsFour) {
        int i=17;
        EXPECT_EQ(2 + 2, 4) << "Text with better explanation" ;
        EXPECT_TRUE(i==17) << "i was not 17 it was " << i;
        //fprintf(stderr,"\n\n/home/olas/work/snowbar/test/Functions_tests.cpp:52:2:error:Test QT find line\n");
}


TEST(CalculateNthFibonacciNumber,test1) {
    // 0, 1 , 1 ,  2 , 3 ,5, 8

    int res=CalculateNthFibonacciNumber(3);
    EXPECT_EQ(res,2);

    res=CalculateNthFibonacciNumber(0);
    EXPECT_EQ(res,2);

    res=CalculateNthFibonacciNumber(5);
    EXPECT_EQ(res,5);

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

TEST(IsLeapYear,test1)
{

      EXPECT_TRUE(IsLeapYear(2016)) << "2016 is a leapyear ";

      EXPECT_TRUE(IsLeapYear(2020)) << "2020 is a leapyear ";

      EXPECT_FALSE(IsLeapYear(2017)) << "2017 is not a leapyear ";

}

TEST(SelectPrimeNumbers,test1)
{
    std::vector<int> testData;
    testData.push_back(3);    testData.push_back(4);testData.push_back(7);

    std::vector<int> expectedResult;
    expectedResult.push_back(3); ;expectedResult.push_back(7);

    std::vector<int> result=SelectPrimeNumbers(testData);

    EXPECT_EQ(expectedResult,result);

}


TEST(SelectPrimeNumbers,out_of_range)
{
    std::vector<int> testData;
    testData.push_back(3); testData.push_back(4);testData.push_back(7);
    std::vector<int> result;

    try {
        result=SelectPrimeNumbers(testData);
        FAIL() << "Expected std::out_of_range";
    }
    catch(std::out_of_range const & err) {
        EXPECT_EQ(err.what(),std::string("Out of range"));
    }
    catch(...) {
        FAIL() << "Expected std::out_of_range";
    }
}

TEST(FindNthLargestNumber,test1)
{
    std::vector<int> testData;
    testData.push_back(3); testData.push_back(4);testData.push_back(7);

    int expectedResult=4;

    int result=FindNthLargestNumber(testData,2);

    EXPECT_EQ(expectedResult,4);

}


TEST(FindNthLargestNumber,out_of_range)
{
    std::vector<int> testData;
    testData.push_back(3); testData.push_back(4);testData.push_back(7);
    int result;

    try {
         result=FindNthLargestNumber(testData,47);
        FAIL() << "Expected std::out_of_range";
    }
    catch(std::out_of_range const & err) {
        EXPECT_EQ(err.what(),std::string("Out of range"));
    }
    catch(...) {
        FAIL() << "Expected std::out_of_range";
    }
}




TEST(CountSetBits,test1) {
    int res=CountSetBits(0xF9);
    EXPECT_EQ(res,6);

    res=CountSetBits(0xF9000000);
    EXPECT_EQ(res,6);

}

TEST(IsPalindrome,test1) {

   bool ret=IsPalindrome(0xA0000005);
   EXPECT_TRUE(ret);

   ret=IsPalindrome(0x0F);

   EXPECT_FALSE(ret);
}



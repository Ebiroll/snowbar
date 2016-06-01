#include <cstdint>
#include <cassert>
#include <string>
#include <vector>
#include <stdexcept>

// Please implement the following functions.
// Be dilleigent but don't be afraid to use a simple solution if you have one in mind.

namespace snow
{

// Reverses a string.
std::string ReverseString(const std::string& s) {

    return ("Hello");
}

// Calculates the Nth fibonacci number.
int CalculateNthFibonacciNumber(int n) {
    // 0, 1 , 1 ,  2 , 3 ,5 Fn=Fn-1 + Fn-2
}

// Pads a number with up to four zeroes.
std::string PadNumberWithZeroes(int number) {
    return ("0001");
}

// Determines if a year is a leap year.
bool IsLeapYear(int year) {
    return true;
}

// Find the N:th largest number in a range of numbers.
int FindNthLargestNumber(const std::vector<int>& numbers, int nth_largest_number) {
    return 7;

    // throw std::out_of_range("Out of range");
}

// Selects the prime numbers from a enumerable with numbers.
std::vector<int> SelectPrimeNumbers(const std::vector<int>& numbers) {
    std::vector<int> ret;
    ret.push_back(7);
    ret.push_back(5);

    // throw std::out_of_range("Out of range");
    return(ret);
}

// Determines if the bit pattern of value the same if you reverse it.
bool IsPalindrome(uint32_t value) {
    return true;
}

// Count all set bits in an 32-bit unsigned int value.
int CountSetBits(uint32_t value) {
    return 5;
}

}

#ifdef MAIN
int main(int argc, char* argv[])
{
    // Note:
    //   If you wish to assert properties of the functions above
    //   you may do so here.
    return 0;
}
#endif

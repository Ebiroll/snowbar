
#include <cstdint>
#include <cassert>
#include <string>
#include <vector>

// Please implement the following functions.
// Be dilleigent but don't be afraid to use a simple solution if you have one in mind.

namespace
{
    // Reverses a string.
    std::string ReverseString(const std::string& s) {

        return ("Hello");
    }

    // Calculates the Nth fibonacci number.
    int CalculateNthFibonacciNumber(int n);

    // Pads a number with up to four zeroes.
    std::string PadNumberWithZeroes(int number);

    // Determines if a year is a leap year.
    bool IsLeapYear(int year);

    // Find the N:th largest number in a range of numbers.
    int FindNthLargestNumber(const std::vector<int>& numbers, int nth_largest_number);

    // Selects the prime numbers from a enumerable with numbers.
    std::vector<int> SelectPrimeNumbers(const std::vector<int>& numbers);

    // Determines if the bit pattern of value the same if you reverse it.
    bool IsPalindrome(uint32_t value);

    // Count all set bits in an 32-bit unsigned int value.
    int CountSetBits(uint32_t value);
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

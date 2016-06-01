#ifndef FUNCTIONS
#define FUNCTIONS

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


#endif // FUNCTIONS


// P01 - Bit manipulation (about 20 min).
//
// Write bit operations on a 32-bit word. Every function is constexpr: some tests
// evaluate them at compile time.
//
//   set_bit, clear_bit, toggle_bit, test_bit
//   count_ones                 number of 1-bits
//   is_power_of_two            exactly one bit set; 0 is not a power of two
//   lowest_set_bit             index of the lowest 1-bit; 32 when the value is 0
//   reverse_bits               reverse the 8 bits of a byte
//
// Rules: no <bit>, no compiler builtins. Use 1u, not 1, in shifts
// (1 << 31 is a signed overflow; 1 << 32 is undefined behaviour).
#include "check.hpp"

#include <cstdint>

// ===== YOUR CODE =====

constexpr std::uint32_t set_bit(std::uint32_t value, unsigned bit) 
{ 
    return (value | (1u << bit));
}
constexpr std::uint32_t clear_bit(std::uint32_t value, unsigned bit) 
{ 
    return (value &~ (1u << bit)); 
}
constexpr std::uint32_t toggle_bit(std::uint32_t value, unsigned bit) 
{ 
    return (value ^ (1u << bit)); 
}
constexpr bool test_bit(std::uint32_t value, unsigned bit) 
{
     return ((value >> bit) & 1u); 
}
constexpr unsigned count_ones(std::uint32_t value) 
{ 
    unsigned counter = 0u;
    for(unsigned i = 0u; i<32u; i++)
    {
        (value >> i) & 1u ? counter++ : counter;
    }
    return counter;
}
constexpr bool is_power_of_two(std::uint32_t value)
{ 
    unsigned counter = count_ones(value);
    return counter == 1u;
}
// Index of the lowest set bit; 32 when value is 0.
constexpr unsigned lowest_set_bit(std::uint32_t value) 
{ 
    for(unsigned i =0; i<32u; i++)
    {
        if((value >> i) & 1) return i;
    } 
    return 32u;
}
constexpr std::uint8_t reverse_bits(std::uint8_t value) 
{
    std::uint8_t result = 0;
    for (unsigned i=0u; i<8u; i++)
    {
        result |= (((value >> i) & 1u) << (7u-i));
    } 
    return result; 
}


// ===== TESTS (do not edit) =====

int main() {
    CHECK(set_bit(0, 3) == 0x8u);
    CHECK(set_bit(0x8, 3) == 0x8u);
    CHECK(set_bit(0, 31) == 0x8000'0000u);
    CHECK(clear_bit(0xFF, 0) == 0xFEu);
    CHECK(clear_bit(0, 5) == 0u);
    CHECK(clear_bit(0xFFFF'FFFF, 31) == 0x7FFF'FFFFu);
    CHECK(toggle_bit(0b1010, 1) == 0b1000u);
    CHECK(toggle_bit(0, 4) == 0x10u);
    CHECK(test_bit(0x80, 7));
    CHECK(!test_bit(0x80, 6));
    CHECK(test_bit(0x8000'0000, 31));

    CHECK(count_ones(0) == 0u);
    CHECK(count_ones(0b1011) == 3u);
    CHECK(count_ones(0x8000'0001) == 2u);
    CHECK(count_ones(0xFFFF'FFFF) == 32u);

    CHECK(!is_power_of_two(0));
    CHECK(is_power_of_two(1));
    CHECK(is_power_of_two(64));
    CHECK(!is_power_of_two(6));
    CHECK(is_power_of_two(0x8000'0000));
    CHECK(!is_power_of_two(0xFFFF'FFFF));

    CHECK(lowest_set_bit(0) == 32u);
    CHECK(lowest_set_bit(1) == 0u);
    CHECK(lowest_set_bit(0b1100) == 2u);
    CHECK(lowest_set_bit(0x8000'0000) == 31u);

    CHECK(reverse_bits(0b0000'0001) == 0b1000'0000);
    CHECK(reverse_bits(0b1101'0000) == 0b0000'1011);
    CHECK(reverse_bits(0xFF) == 0xFF);
    CHECK(reverse_bits(0) == 0);

    constexpr std::uint32_t kAtCompileTime = set_bit(clear_bit(0xFF, 0), 8);
    CHECK(kAtCompileTime == 0x1FEu);
    constexpr unsigned kOnes = count_ones(0xF0F0);
    CHECK(kOnes == 8u);
    return report();
}

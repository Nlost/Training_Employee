// P03 - Register fields on a simulated STM32F4 GPIO port (about 30 min).
//
// GpioRegs is the GPIO register block, laid out like the real peripheral
// (ODR at offset 0x14, BSRR at 0x18). On a microcontroller the same code would
// point at a fixed address; here the struct lives in RAM so the tests can read it.
//
//   read_field / write_field     bits [pos, pos + width), width 1..32.
//                                Extra bits in value are dropped. Width 32 must
//                                not shift by 32 (that is undefined behaviour).
//   set_mode                     MODER, 2 bits per pin: Input, Output, Alternate, Analog
//   write_pin                    drive a pin high or low through BSRR only.
//                                Bit pin sets it; bit pin + 16 resets it.
//                                Zeros have no effect. Never read or write ODR.
//   read_pin                     the pin's level from IDR
//   set_alternate_function       4 bits per pin: AFR[0] for pins 0-7, AFR[1] for 8-15
//
// Assign the whole volatile register (gpio.MODER = write_field(...)).
// Do not use |= or &= on a volatile: C++20 deprecates them, and a single
// assignment makes the one read and one write obvious.
#include "check.hpp"
#include <iostream>
#include <stdio.h>
#include <cstddef>
#include <cstdint>
#include <bitset>

struct GpioRegs {
    volatile std::uint32_t MODER, OTYPER, OSPEEDR, PUPDR, IDR, ODR, BSRR, LCKR;
    volatile std::uint32_t AFR[2];
};

enum class PinMode : std::uint32_t { Input = 0b00, Output = 0b01, Alternate = 0b10, Analog = 0b11 };

// ===== YOUR CODE =====

// Bits [pos, pos + width) of reg. width is 1..32.
constexpr std::uint32_t read_field(std::uint32_t reg, unsigned pos, unsigned width) 
{ 
    std::uint32_t result = reg << (32 - width - pos);
    result >>= 32 - width;
    return result;
}
// reg with bits [pos, pos + width) replaced by value. Bits of value above width are ignored.
constexpr std::uint32_t write_field(std::uint32_t reg, unsigned pos, unsigned width,
                                    std::uint32_t value) {
    std::uint32_t mask = value << (32 - width);
    mask >>= 32 - width - pos;
    reg |= mask;
    
    mask = ~mask;
    mask <<= 32 - width - pos;
    mask >>= 32 - width;
    mask <<=pos;
    reg &= ~mask;
    return reg;
}
// MODER has 2 bits per pin.
void set_mode(GpioRegs& gpio, unsigned pin, PinMode mode) 
{
    gpio.MODER = write_field(gpio.MODER, pin * 2, 2, static_cast<std::uint32_t>(mode));
}
// Uses only BSRR: bits 0-15 set a pin, bits 16-31 reset it. Never touches ODR.
void write_pin(GpioRegs& gpio, unsigned pin, bool high) 
{
    if(high)
    {
        gpio.BSRR = (1 << pin);
    }
    else
    {
        gpio.BSRR = (1 << (16+pin));
    }
    
}
bool read_pin(const GpioRegs& gpio, unsigned pin) 
{ 
    return (gpio.IDR >> pin) & 1;
}
// AFR[0] holds pins 0-7, AFR[1] pins 8-15, 4 bits per pin.
void set_alternate_function(GpioRegs& gpio, unsigned pin, unsigned af) 
{
    if(pin < 8)
    {
        gpio.AFR[0] = write_field(gpio.AFR[0], pin*4, 4, af);
    }
    else{
        gpio.AFR[1] = write_field(gpio.AFR[1], (pin-8)*4, 4, af);
    }
}

// ===== TESTS (do not edit) =====

static_assert(offsetof(GpioRegs, ODR) == 0x14);
static_assert(offsetof(GpioRegs, BSRR) == 0x18);
static_assert(offsetof(GpioRegs, AFR) == 0x20);
static_assert(sizeof(GpioRegs) == 0x28);

int main() {
    CHECK(read_field(0xABCD'1234, 4, 8) == 0x23u);
    CHECK(read_field(0xABCD'1234, 28, 4) == 0xAu);
    CHECK(read_field(0x8000'0000, 31, 1) == 1u);
    CHECK(read_field(0xFFFF'FFFF, 0, 32) == 0xFFFF'FFFFu);

    CHECK(write_field(0, 4, 4, 0xF) == 0xF0u);
    CHECK(write_field(0xFFFF'FFFF, 8, 4, 0x5) == 0xFFFF'F5FFu);
    CHECK(write_field(0, 0, 4, 0x1F) == 0xFu);
    CHECK(write_field(0x1234'5678, 0, 32, 0xCAFE'BABE) == 0xCAFE'BABEu);
    CHECK(write_field(0, 30, 2, 0b11) == 0xC000'0000u);

    GpioRegs gpio{};
    gpio.MODER = 0xFFFF'FFFF;
    set_mode(gpio, 5, PinMode::Output);
    CHECK(gpio.MODER == 0xFFFF'F7FFu);
    gpio.MODER = 0;
    set_mode(gpio, 0, PinMode::Analog);
    set_mode(gpio, 15, PinMode::Alternate);
    CHECK(gpio.MODER == 0x8000'0003u);
    set_mode(gpio, 0, PinMode::Input);
    CHECK(gpio.MODER == 0x8000'0000u);

    gpio.ODR = 0x1234;
    write_pin(gpio, 5, true);
    CHECK(gpio.BSRR == 0x0000'0020u);
    write_pin(gpio, 5, false);
    CHECK(gpio.BSRR == 0x0020'0000u);
    write_pin(gpio, 15, false);
    CHECK(gpio.BSRR == 0x8000'0000u);
    CHECK(gpio.ODR == 0x1234u);

    gpio.IDR = 0x0100;
    CHECK(read_pin(gpio, 8));
    CHECK(!read_pin(gpio, 7));

    gpio.AFR[0] = 0xFFFF'FFFF;
    set_alternate_function(gpio, 3, 7);
    CHECK(gpio.AFR[0] == 0xFFFF'7FFFu);
    gpio.AFR[1] = 0;
    set_alternate_function(gpio, 9, 0xA);
    CHECK(gpio.AFR[1] == 0x0000'00A0u);
    return report();
}

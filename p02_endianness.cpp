// P02 - Byte order (about 20 min).
//
// Convert between host integers and explicit byte order, using shifts and masks only.
// The result must be the same on a big-endian CPU.
//
//   byteswap16, byteswap32
//   load_be16, load_be32, load_le32     assemble an integer from a fixed-size span
//   store_be32, store_le32              write an integer into a fixed-size span
//   read_be32_at(buffer, offset)        big-endian uint32 at any offset, including
//                                       unaligned ones; std::nullopt if the 4 bytes
//                                       do not fit in the buffer
//
// Rules: no reinterpret_cast, no memcpy, no std::byteswap.
// A uint8_t shifted by 24 promotes to int and can overflow: widen to uint32_t first.
// Do not write offset + 4 > size: a huge offset wraps. Check offset against size first.
#include "check.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

// ===== YOUR CODE =====

constexpr std::uint16_t byteswap16(std::uint16_t value) 
{ 
    std::uint8_t byte1 = (std::uint8_t)value;
    std::uint8_t byte2 = (std::uint8_t)(value>>8u);
    std::uint16_t result = (std::uint16_t)(byte2 | (byte1 << 8u));
    return result; 
}
constexpr std::uint32_t byteswap32(std::uint32_t value) 
{ 
    std::uint8_t byte1 = (std::uint8_t)value;
    std::uint8_t byte2 = (std::uint8_t)(value>>8u);
    std::uint8_t byte3 = (std::uint8_t)(value>>16u);
    std::uint8_t byte4 = (std::uint8_t)(value>>24u);
    std::uint32_t result = (std::uint32_t)(byte4 | (byte3 << 8u) | (byte2 << 16u) | (byte1 << 24u));
    return result;

}
constexpr std::uint16_t load_be16(std::span<const std::uint8_t, 2> bytes) 
{ 
    std::uint16_t result = (std::uint16_t)(bytes[1] | (bytes[0] << 8u));
    return result;
}
constexpr std::uint32_t load_be32(std::span<const std::uint8_t, 4> bytes) 
{ 
    std::uint32_t result = (std::uint32_t)(bytes[3] | (bytes[2] << 8u) | (bytes[1] << 16u) | (bytes[0] << 24u));
    return result; 
}
constexpr std::uint32_t load_le32(std::span<const std::uint8_t, 4> bytes) 
{ 
    std::uint32_t result = (std::uint32_t)(bytes[0] | (bytes[1] << 8u) | (bytes[2] << 16u) | (bytes[3] << 24u));
    return result; 
}
constexpr void store_be32(std::span<std::uint8_t, 4> bytes, std::uint32_t value) 
{
    bytes[0] = (uint8_t)(value >> 24u);
    bytes[1] = (uint8_t)(value >> 16u);
    bytes[2] = (uint8_t)(value >> 8u);
    bytes[3] = (uint8_t)value;
}
constexpr void store_le32(std::span<std::uint8_t, 4> bytes, std::uint32_t value) 
{
    bytes[3] = (uint8_t)(value >> 24u);
    bytes[2] = (uint8_t)(value >> 16u);
    bytes[1] = (uint8_t)(value >> 8u);
    bytes[0] = (uint8_t)value;

}
// Big-endian uint32 starting at any (possibly unaligned) offset; nullopt if it doesn't fit.
std::optional<std::uint32_t> read_be32_at(std::span<const std::uint8_t> buffer, std::size_t offset) {
    if(buffer.size() - offset < 4)
    {
        return std::nullopt;    
    }
    if(offset > buffer.size())
    {
        return std::nullopt;
    }
    std::optional<std::uint32_t>result;
    for(unsigned i=0; i<4u; i++)
    {
        result = (std::uint32_t)(buffer[3 + offset] | (buffer[2 + offset] << 8u) | (buffer[1 + offset] << 16u) | (buffer[0 + offset] << 24u));
    }
    return result;
}

// ===== TESTS (do not edit) =====

int main() {
    CHECK(byteswap16(0x1234) == 0x3412);
    CHECK(byteswap16(0x00FF) == 0xFF00);
    CHECK(byteswap32(0x1234'5678) == 0x7856'3412u);
    CHECK(byteswap32(0xFF00'0000) == 0x0000'00FFu);

    const std::array<std::uint8_t, 2> voltage{0x2E, 0xE0};
    CHECK(load_be16(voltage) == 12'000);
    const std::array<std::uint8_t, 4> word{0xDE, 0xAD, 0xBE, 0xEF};
    CHECK(load_be32(word) == 0xDEAD'BEEFu);
    CHECK(load_le32(word) == 0xEFBE'ADDEu);

    std::array<std::uint8_t, 4> out{};
    store_be32(out, 0x1234'5678);
    CHECK((out == std::array<std::uint8_t, 4>{0x12, 0x34, 0x56, 0x78}));
    store_le32(out, 0x1234'5678);
    CHECK((out == std::array<std::uint8_t, 4>{0x78, 0x56, 0x34, 0x12}));

    for (std::uint32_t v : {0u, 1u, 0x8000'0000u, 0xCAFE'BABEu, 0xFFFF'FFFFu}) {
        store_le32(out, v);
        CHECK(load_le32(out) == v);
        store_be32(out, v);
        CHECK(load_be32(out) == v);
    }

    const std::array<std::uint8_t, 6> stream{0x00, 0xDE, 0xAD, 0xBE, 0xEF, 0x01};
    CHECK(read_be32_at(stream, 1) == 0xDEAD'BEEFu);
    CHECK(read_be32_at(stream, 2) == 0xADBE'EF01u);
    CHECK(!read_be32_at(stream, 3).has_value());
    CHECK(!read_be32_at(stream, 100).has_value());

    // constexpr std::uint32_t kSwapped = byteswap32(0x1122'3344);
    // CHECK(kSwapped == 0x4433'2211u);
    return report();
}

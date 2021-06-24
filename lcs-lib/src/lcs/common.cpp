#include "common.hpp"
#include <charconv>
using namespace LCS;

extern std::u8string to_hex_string(std::uint32_t value) {
    char buffer[32];
    auto const result = std::to_chars(buffer, buffer + sizeof(buffer), value, 16);
    return { buffer, result.ptr };
}

extern std::u8string to_hex_string(std::uint64_t value) {
    char buffer[32];
    auto const result = std::to_chars(buffer, buffer + sizeof(buffer), value, 16);
    return { buffer, result.ptr };
}

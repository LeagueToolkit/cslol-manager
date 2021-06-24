#include "common.hpp"
#include <charconv>
using namespace LCS;

std::u8string LCS::to_hex_string(std::uint32_t value) {
    char buffer[32];
    auto const result = std::to_chars(buffer, buffer + sizeof(buffer), value, 16);
    return { buffer, result.ptr };
}

std::u8string LCS::to_hex_string(std::uint64_t value) {
    char buffer[32];
    auto const result = std::to_chars(buffer, buffer + sizeof(buffer), value, 16);
    return { buffer, result.ptr };
}

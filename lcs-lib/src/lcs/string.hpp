#ifndef LCS_STRING_HPP
#define LCS_STRING_HPP
#include "common.hpp"
#include <type_traits>
#include <charconv>
#include <concepts>

namespace LCS {
    inline std::u8string to_u8string(char8_t const* from) {
        return from;
    }

    inline std::u8string to_u8string(fs::path const& from) {
        return from.generic_u8string();
    }

    inline std::u8string const& to_u8string(std::u8string const& from) {
        return from;
    }

    inline std::u8string to_u8string(bool const& from) {
        return from ? u8"true" : u8"false";
    }

    template <typename T> requires(std::is_arithmetic_v<T>)
    inline std::u8string to_u8string(T const& from) {
        char8_t buffer[64] = {};
        std::to_chars((char*)buffer, (char*)buffer + sizeof(buffer) - 1, from);
        return buffer;
    }
}


#endif // LCS_STRING_HPP

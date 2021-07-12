#ifndef LCS_STRING_HPP
#define LCS_STRING_HPP
#include "common.hpp"
#include <type_traits>
#include <charconv>
#include <concepts>
#include <map>

namespace LCS {
    extern std::map<std::u8string, std::u8string>& path_remap();

    extern std::u8string func_to_u8string(char const* func);

    inline std::u8string to_u8string(char const* from) {
        std::string_view view = from;
        return { view.begin(), view.end() };
    }

    inline std::u8string to_u8string(char8_t const* from) {
        return from;
    }

    extern std::u8string to_u8string(fs::path const& from);

    extern std::u8string to_u8string(fs::filesystem_error const& err);

    inline std::u8string const& to_u8string(std::u8string const& from) {
        return from;
    }

    template <typename T> requires(std::is_same_v<T, bool>)
    inline std::u8string to_u8string(T from) {
        return from ? u8"true" : u8"false";
    }

    template <typename T> requires(std::is_arithmetic_v<T> && !std::is_same_v<T, bool>)
    inline std::u8string to_u8string(T from) {
        char8_t buffer[64] = {};
        std::to_chars((char*)buffer, (char*)buffer + sizeof(buffer) - 1, from);
        return buffer;
    }
}


#endif // LCS_STRING_HPP

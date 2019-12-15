#pragma once
#include <cinttypes>
#include <cstddef>
#include <array>

// matches any byte
inline constexpr uint16_t Any = 0x0100u;

// capture offset of given byte
template<uint16_t C = Any>
inline constexpr uint16_t Cap = C | 0x0200u;

// returns function that searches for the given signature
// result of search is array with first offset + any offsets captured
template<uint16_t...ops>
inline constexpr auto Pattern() {
    return [](uint8_t const* start, size_t size) constexpr noexcept {
        std::array<uint8_t const*, ((ops & 0x0200 ? 1 : 0) + ... + 1)> result = {};
        uint8_t const* const end = start + size + sizeof...(ops);
        for (uint8_t const* i = start; i != end; i++) {
            uint8_t const* c = i;
            if (((*c++ == (ops & 0xFF) || (ops & 0x100)) && ...)) {
                uint8_t const* o = i;
                size_t r = 0;
                result[r++] = o;
                ((ops & 0x200 ? result[r++] = o++ : o++), ...);
                return result;
            }
        }
        return result;
    };
}
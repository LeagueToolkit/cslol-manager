#pragma once
#include <functional>
#include <lol/common.hpp>
#include <lol/fs.hpp>
#include <unordered_map>

namespace lol::hash {
    //! Lower-case hash of string.
    struct Fnv1a32 {
        using underlying_t = std::uint32_t;

        //! Zero initialized hash.
        constexpr Fnv1a32() noexcept = default;

        //! Initialize hash from underlying type.
        explicit constexpr Fnv1a32(underlying_t value) noexcept : value_(value){};

        //! Initialize hash from string.
        explicit Fnv1a32(std::string_view str) noexcept;

        //! Convert hash to a number.
        constexpr operator underlying_t() const noexcept { return value_; }

    private:
        underlying_t value_ = 0;
    };

    constexpr auto operator""_fnv1a32(char const* str) noexcept -> Fnv1a32 {
        std::uint32_t h = 0x811c9dc5u;
        while (unsigned char c = (unsigned char)*str++) {
            c = c >= 'A' && c <= 'Z' ? (c - 'A') + 'a' : c;
            h = ((h ^ c) * 0x01000193u) & 0xFFFFFFFFu;
        }
        return Fnv1a32(h);
    }
}

template <>
struct std::hash<lol::hash::Fnv1a32> : std::hash<lol::hash::Fnv1a32::underlying_t> {};

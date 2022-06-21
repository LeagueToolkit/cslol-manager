#pragma once
#include <functional>
#include <lol/common.hpp>
#include <lol/fs.hpp>
#include <unordered_map>

namespace lol::hash {
    //! Lower-case hash of path string.
    struct Xxh64 {
        using underlying_t = std::uint64_t;

        //! Zero initialized hash.
        constexpr Xxh64() noexcept = default;

        //! Initialize hash from underlying type.
        explicit constexpr Xxh64(underlying_t value) noexcept : value_(value){};

        //! Initialize hash from string.
        explicit Xxh64(std::string_view str) noexcept;

        //! Tries to convert filename to hex string.
        static auto from_path(std::string_view str) noexcept -> Xxh64;

        //! Tries to convert filename to hex string.
        static auto from_path(fs::path const &path) noexcept -> Xxh64 {
            auto str = path.generic_string();
            return from_path((std::string_view)str);
        }

        //! Convert hash to a number.
        constexpr operator underlying_t() const noexcept { return value_; }

    private:
        underlying_t value_ = 0;
    };
}

template <>
struct std::hash<lol::hash::Xxh64> : std::hash<lol::hash::Xxh64::underlying_t> {};

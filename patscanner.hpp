#pragma once
#include <cinttypes>
#include <cstddef>
#include <array>

template<uint16_t op>
inline constexpr size_t POpSize = (op >> 8) ? (op >> 8) : 1u;

inline constexpr uint16_t Any = 0x0100u;

template<uint8_t count>
inline constexpr uint16_t Cap = (static_cast<uint16_t>(count) << 8) | 0x01u;

template<uint8_t count>
inline constexpr uint16_t Skip = (static_cast<uint16_t>(count) << 8);

template<uint16_t...ops>
struct Pattern {
    auto operator()(uint8_t const * const start,
                    uint8_t const * const end) const {
        auto const capc = ((ops > 0xFF && (ops & 0xFF)) + ... + 1);
        for(auto i = start; i < end; i++) {
            auto c = i;
            if(((POpSize<ops> <= (end - c) &&
                (ops > 0xFFu || *c == (ops & 0xFF)) &&
                ((c += POpSize<ops>), true)) &&... )) {
                std::array<uint8_t const*, capc> results = { i };
                auto r = 1;
                auto c = i;
                (((((ops > 0xFF) &&
                    (ops & 0xFF) &&
                    ((results[r++] = c), true)), c += POpSize<ops>)), ...);
                return results;
            }
        }
        return std::array<uint8_t const*, capc>{};
    }

    inline auto operator()(uint8_t const * const start, size_t end) const {
        return operator()(start, start + end);
    }
    inline auto operator()(char const* start,
                           char const* end) const {
        return operator()(reinterpret_cast<uint8_t const*>(start),
                          reinterpret_cast<uint8_t const*>(end));
    }
    inline auto operator()(char const * const start, size_t end) const {
        return operator()(start, start + end);
    }
    inline auto operator()(std::string_view str) const {
        return operator()(&str[0], str.size());
    }
};

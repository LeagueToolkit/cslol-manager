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
    inline auto operator()(uint8_t const * const start, uint8_t const * const end) const {
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
};

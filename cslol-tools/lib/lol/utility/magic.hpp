#pragma once
#include <lol/common.hpp>
#include <span>
#include <string_view>

namespace lol::utility {
    struct Magic {
        std::string_view magic = {};
        std::string_view extension = {};
        std::size_t offset = 0;

        template <std::size_t SM, std::size_t SE>
        [[nodiscard]] constexpr Magic(char const (&magic)[SM],
                                      char const (&extension)[SE],
                                      std::size_t offset = 0) noexcept
            : magic({magic, SM - 1}), extension({extension, SE - 1}), offset(offset) {}

        [[nodiscard]] static std::string_view find(std::span<char const> data) noexcept;
    };
}

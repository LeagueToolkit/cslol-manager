#ifndef LCS_UTILITY_HPP
#define LCS_UTILITY_HPP
#include "common.hpp"

namespace LCS {
    struct MagicExt {
        char const* const magic;
        size_t const magic_size;
        char const* const ext;
        size_t const ext_size;
        size_t const offset;

        template<size_t SM, size_t SE>
        constexpr inline MagicExt(char const(&magica)[SM], char const(&exta)[SE], size_t offseta = 0) noexcept
            : magic(magica),
              magic_size(SM - 1),
              ext(exta),
              ext_size(SE - 1),
              offset(offseta)
        {}
    };

    extern std::string ScanExtension(char const* data, size_t size) noexcept;
}

#endif // LCS_UTILITY_HPP

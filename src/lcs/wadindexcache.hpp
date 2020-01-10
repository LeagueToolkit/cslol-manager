#ifndef LCS_WADINDEXCACHE_HPP
#define LCS_WADINDEXCACHE_HPP
#include "common.hpp"
#include "wadindex.hpp"

namespace LCS {
    struct WadIndexCache
    {
        WadIndexCache(WadIndex const& index) noexcept;

        inline auto const& wads() const& noexcept {
            return wads_;
        }

        inline auto const& lookup() const& noexcept {
            return lookup_;
        }
    private:
        fs::path path_;
        std::unordered_map<std::string, fs::path> wads_;
        std::unordered_multimap<uint64_t, std::string> lookup_;
    };
}

#endif // LCS_WADINDEXCACHE_HPP

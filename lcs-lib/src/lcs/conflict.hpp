#ifndef LCS_CONFLICT_HPP
#define LCS_CONFLICT_HPP
#include "common.hpp"
#include <stdexcept>

namespace LCS {
    enum class Conflict {
        Abort,
        Skip,
        Overwrite
    };

    [[noreturn]] extern void raise_hash_conflict(uint64_t xxhash,
                                                 fs::path const& orgpath,
                                                 fs::path const& newpath);

    [[noreturn]] extern void raise_wad_conflict(fs::path const& name,
                                                fs::path const& orgpath,
                                                fs::path const& newpath);
}

#endif // LCS_CONFLICT_HPP

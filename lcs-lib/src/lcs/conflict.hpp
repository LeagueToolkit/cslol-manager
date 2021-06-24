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

    class ConflictError : public std::runtime_error {
    public:
        ConflictError(uint64_t xxhash, fs::path const& orgpath, fs::path const& newpath);
        ConflictError(fs::path const& name, fs::path const& orgpath, fs::path const& newpath);
        std::u8string message;
    };
}

#endif // LCS_CONFLICT_HPP

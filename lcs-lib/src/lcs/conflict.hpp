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
        ConflictError(std::string const& name, fs::path const& orgpath, fs::path const& newpath);
    };
}

#endif // LCS_CONFLICT_HPP

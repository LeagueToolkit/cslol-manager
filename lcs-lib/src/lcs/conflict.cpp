#include "conflict.hpp"
#include <string>

using namespace LCS;

ConflictError::ConflictError(uint64_t xxhash, fs::path const& orgpath, fs::path const& newpath)
    : std::runtime_error(std::string("Wad conflict!\n\txxhash:")
                         + std::to_string(xxhash) + "\n\torgpath:"
                         + orgpath.generic_string() + "\n\tnewpath:"
                         + newpath.generic_string()) {

}

ConflictError::ConflictError(std::string const& name, fs::path const& orgpath, fs::path const& newpath)
    : std::runtime_error(std::string("Wad conflict!\n\tname:")
                         + name + "\n\torgpath:"
                         + orgpath.generic_string() + "\n\tnewpath:"
                         + newpath.generic_string()) {
}

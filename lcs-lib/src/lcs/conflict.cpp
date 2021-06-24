#include "conflict.hpp"
#include <string>

using namespace LCS;

ConflictError::ConflictError(uint64_t xxhash, fs::path const& orgpath, fs::path const& newpath)
    : std::runtime_error("Wad conflict"),
        message(std::u8string(u8"Wad conflict!\n\txxhash:")
                         + to_hex_string(xxhash) + u8"\n\torgpath:"
                         + orgpath.generic_u8string() + u8"\n\tnewpath:"
                         + newpath.generic_u8string()) {

}

ConflictError::ConflictError(std::u8string const& name, fs::path const& orgpath, fs::path const& newpath)
    : std::runtime_error("Wad conflict"),
      message(std::u8string(u8"Wad conflict!\n\tname:")
                         + name + u8"\n\torgpath:"
                         + orgpath.generic_u8string() + u8"\n\tnewpath:"
                         + newpath.generic_u8string()) {
}

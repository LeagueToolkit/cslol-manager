#include "conflict.hpp"
#include <string>

using namespace LCS;

ConflictError::ConflictError(uint64_t xxhash, fs::path const& orgpath, fs::path const& newpath)
    : std::runtime_error("Hash conflict!"),
        message(std::u8string(u8"Hash conflict!\n\txxhash:")
                         + to_hex_string(xxhash) + u8"\n\torgpath:"
                         + orgpath.generic_u8string() + u8"\n\tnewpath:"
                         + newpath.generic_u8string()) {

}

ConflictError::ConflictError(fs::path const& name, fs::path const& orgpath, fs::path const& newpath)
    : std::runtime_error("Wad conflict!"),
      message(std::u8string(u8"Wad conflict!\n\tname:")
                         + name.generic_u8string() + u8"\n\torgpath:"
                         + orgpath.generic_u8string() + u8"\n\tnewpath:"
                         + newpath.generic_u8string()) {
}

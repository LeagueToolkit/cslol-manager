#include "conflict.hpp"
#include "error.hpp"
#include <string>

using namespace LCS;

ConflictError::ConflictError(uint64_t xxhash, fs::path const& orgpath, fs::path const& newpath)
    : std::runtime_error("Hash conflict!"),
      message(to_u8string(u8"Hash conflict!:")
              + to_u8string(u8"\n\txxhash = ") + to_hex_string(xxhash)
              + to_u8string(u8"\n\torgpath = ") + to_u8string(orgpath)
              + to_u8string(u8"\n\tnewpath = ") + to_u8string(newpath)) {
}

ConflictError::ConflictError(fs::path const& name, fs::path const& orgpath, fs::path const& newpath)
    : std::runtime_error("Wad conflict!"),
      message(to_u8string(u8"Wad conflict!:")
              + to_u8string(u8"\n\tname = ") + to_u8string(name)
              + to_u8string(u8"\n\torgpath = ") + to_u8string(orgpath)
              + to_u8string(u8"\n\tnewpath = ") + to_u8string(newpath)) {
}

#ifndef LCS_COMMON_HPP
#define LCS_COMMON_HPP
#include <cinttypes>
#include <cstddef>
#include <filesystem>
#include <string>

namespace LCS {
    namespace fs = std::filesystem;
    enum class Conflict;
    class ConflictError;
    class Progress;
    class ProgressMulti;
    struct File;
    struct InFile;
    struct OutFile;
    struct Mod;
    struct ModIndex;
    struct ModUnZip;
    struct Wad;
    struct WadIndex;
    struct WadIndexCache;
    struct WadMake;
    struct WadMakeQueue;
    struct WadMerge;
}

#endif // LCS_COMMON_HPP

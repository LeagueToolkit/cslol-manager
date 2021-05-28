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
    struct WadMake;
    struct WadMakeQueue;
    struct WadMerge;

    extern char const* VERSION;
    extern char const* COMMIT;
    extern char const* DATE;
}

#endif // LCS_COMMON_HPP

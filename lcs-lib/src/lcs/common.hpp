#ifndef LCS_COMMON_HPP
#define LCS_COMMON_HPP
#include <cinttypes>
#include <cstddef>
#include <istream>
#include <ostream>
#include <fstream>
#include <array>
#include <vector>
#include <filesystem>
#include <string>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <stdexcept>
#include <string_view>

namespace LCS {
    namespace fs = std::filesystem;

    class Progress {
    public:
        Progress() noexcept;
        virtual ~Progress() noexcept;
        virtual void startItem(fs::path const& path, size_t dataSize) noexcept;
        virtual void consumeData(size_t ammount) noexcept;
        virtual void finishItem() noexcept;
    };

    class ProgressMulti : public Progress {
    public:
        ProgressMulti() noexcept;
        virtual ~ProgressMulti() noexcept;
        virtual void startMulti(size_t itemCount, size_t dataTotal) noexcept;
        virtual void finishMulti() noexcept;
    };

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

    template<size_t S>
    inline void copyStream(std::istream& source, std::ofstream& dest,
                           size_t size, char (&buffer)[S],
                           Progress& progress) {
        for(size_t i = 0; i != size;) {
            size_t count = std::min(S, size - i);
            source.read(buffer, count);
            dest.write(buffer, count);
            // progress.consumeData(count);
            i += count;
        }
        progress.consumeData(size);
    }

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

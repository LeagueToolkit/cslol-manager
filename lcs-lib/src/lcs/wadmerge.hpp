#ifndef LCS_WADMERGE_HPP
#define LCS_WADMERGE_HPP
#include "common.hpp"
#include "wadindex.hpp"
#include "mod.hpp"
#include <map>
#include <unordered_map>
#include <array>

namespace LCS {
    struct WadMerge  {
        // Throws std::runtime_error
        WadMerge(fs::path const& path, Wad const* source);
        WadMerge(WadMerge const&) = delete;
        WadMerge(WadMerge&&) = default;
        WadMerge& operator=(WadMerge const&) = delete;
        WadMerge& operator=(WadMerge&&) = delete;

        // Throws std::runtime_error
        void addWad(Wad const* wad, Conflict conflict);

        void addExtraEntry(Wad::Entry const& entry, Wad const* source, Conflict conflict);

        std::uint64_t size() const noexcept;

        // Throws std::runtime_error
        void write(Progress& progress) const;

        inline auto const& path() const& noexcept {
            return path_;
        }
    private:
        enum class EntryKind {
            Original,
            Full,
            Extra
        };
        struct Entry : Wad::Entry {
            Wad const* wad_;
            EntryKind kind_;
        };
        // Throws std::runtime_error
        void addWadEntry(Wad::Entry const& entry, Wad const* source,
                         Conflict conflict, EntryKind type);

        fs::path path_;
        Wad const* original_;
        std::map<uint64_t, Entry> entries_;
        std::unordered_map<uint64_t, std::array<uint8_t, 8>> orgsha256_;
        mutable std::uint64_t size_ = 0;
        mutable bool sizeCalculated_ = false;;
    };
}


#endif // LCS_WADMERGE_HPP

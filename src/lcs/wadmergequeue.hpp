#ifndef LCS_WADMERGEQUEUE_HPP
#define LCS_WADMERGEQUEUE_HPP
#include "common.hpp"
#include "wadindex.hpp"
#include "wadmerge.hpp"

namespace LCS {
    struct WadMergeQueue {
        // Throws fs::filesystem_error
        WadMergeQueue(fs::path const& path, WadIndex const& index);
        WadMergeQueue(WadMergeQueue const&) = delete;
        WadMergeQueue(WadMergeQueue&&) = default;
        WadMergeQueue& operator=(WadMergeQueue const&) = delete;
        WadMergeQueue& operator=(WadMergeQueue&&) = delete;

        // Throws std::runtime_error
        void addMod(Mod const* mod, Conflict conflict);

        // Throws std::runtime_error
        void addWad(Wad const* source, Conflict conflict);

        // Throws std::runtime_error
        void write(ProgressMulti& progress) const;

        // Throws std::runtime_error
        void write_whole(ProgressMulti& progress) const;

        // Throws fs::filesystem_error
        void cleanup();
    private:
        WadMerge* findOrAddItem(Wad const* original);

        fs::path path_;
        WadIndex const& index_;
        std::unordered_map<Wad const*, std::unique_ptr<WadMerge>> items_;
    };
}

#endif // LCS_WADMERGEQUEUE_HPP

#ifndef LCS_WADINDEX_H
#define LCS_WADINDEX_H
#include "common.hpp"
#include "wad.hpp"
#include <map>
#include <unordered_map>
#include <vector>
#include <algorithm>

namespace LCS{
    struct WadIndex {
        template<typename iter_t>
        struct EntryRange {
            iter_t const beg_;
            iter_t const end_;
            inline auto begin() const noexcept { return beg_; }
            inline auto end() const noexcept { return end_; }
            inline auto cbegin() const noexcept { return beg_; }
            inline auto cend() const noexcept { return end_; }
        };

        // Throws std::runtime_error
        WadIndex(fs::path const& path, bool blacklist = true, bool ignorebad = false);
        WadIndex(WadIndex const&) = delete;
        WadIndex(WadIndex&&) = default;
        WadIndex& operator=(WadIndex const&) = delete;
        WadIndex& operator=(WadIndex&&) = delete;

        bool is_uptodate() const;

        inline auto const& path() const& noexcept {
            return path_;
        }

        inline auto const& wads() const& noexcept {
            return wads_;
        }

        inline auto const& lookup() const& noexcept {
            return lookup_;
        }

        inline auto const& checksums() const& noexcept {
            return checksums_;
        }

        inline auto findExtra(uint64_t hash) const& noexcept {
            auto [beg, end] = lookup_.equal_range(hash);
            return EntryRange<decltype(beg)> { beg, end };
        }

        template<typename I, typename F>
        inline Wad const* findOriginal(I const& collection, F&& func) const& noexcept {
            std::unordered_map<Wad const*, size_t> counter{};
            counter.reserve(wads_.size());
            for(auto const& [name, wad]: wads_) {
                counter[wad.get()] = 0;
            }
            for(auto const& entry: collection) {
                auto xxhash = func(entry);
                auto wads = lookup_.equal_range(xxhash);
                for(auto w = wads.first; w != wads.second; ++w) {
                    counter[w->second] += 1;
                }
            }
            auto const max = std::max_element(counter.cbegin(), counter.cend(),
                                              [](auto const& l, auto const& r) {
               return l.second < r.second;
            });
            if (max->second == 0) {
                return nullptr;
            }
            return max->first;
        }

        inline auto findOriginal(fs::path const& filename, std::vector<Wad::Entry> const& entries) const& noexcept {
            if (auto i = wads_.find(filename); i != wads_.end()) {
                return i->second.get();
            }
            return findOriginal(entries, [](auto const& entry){
                return entry.xxhash;
            });
        }

        template<typename V>
        inline auto findOriginal(fs::path const& filename, std::map<uint64_t, V> const& entries) const& noexcept {
            if (auto i = wads_.find(filename); i != wads_.end()) {
                return i->second.get();
            }
            return findOriginal(entries, [](auto const& entry){
                return entry.first;
            });
        }

        template<typename V>
        inline auto findOriginal(fs::path const& filename, std::unordered_map<uint64_t, V> const& entries) const& noexcept {
            if (auto i = wads_.find(filename); i != wads_.end()) {
                return i->second.get();
            }
            return findOriginal(entries, [](auto const& entry){
                return entry.first;
            });
        }
    private:
        fs::path path_;
        std::map<fs::path, std::unique_ptr<Wad const>> wads_;
        std::unordered_multimap<uint64_t, Wad const*> lookup_;
        std::unordered_map<uint64_t, uint64_t> checksums_;
        bool blacklist_;
        fs::file_time_type last_write_time_ = {};
    };
}

#endif // WADINDEX_H

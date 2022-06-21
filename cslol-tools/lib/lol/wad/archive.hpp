#pragma once
#include <array>
#include <lol/common.hpp>
#include <lol/fs.hpp>
#include <lol/hash/dict.hpp>
#include <lol/io/bytes.hpp>
#include <lol/wad/entry.hpp>
#include <lol/wad/toc.hpp>
#include <map>
#include <optional>

namespace lol::wad {
    //! Wad archive.
    struct Archive {
        using Map = std::map<hash::Xxh64, wad::EntryData>;

        static auto read_from_toc(io::Bytes src, TOC const& toc) -> Archive;

        static auto read_from_file(fs::path const& path) -> Archive;

        static auto pack_from_directory(fs::path const& path) -> Archive;

        auto touch() -> void;

        auto mark_optimal() -> void;

        auto add_from_directory(fs::path const& path, fs::path const& dir) -> void;

        auto write_to_file(fs::path const& path) const -> void;

        auto estimate_size() const noexcept -> std::size_t;

        template <typename Func>
            requires(std::is_invocable<Func, Map::const_reference, Map::const_reference>::value)
        auto foreach_overlap(Archive const& rhs, Func&& func) const {
            auto lhs_cur = this->entries.begin();
            auto const lhs_end = this->entries.end();
            auto rhs_cur = rhs.entries.begin();
            auto const rhs_end = rhs.entries.end();
            while (lhs_cur != lhs_end && rhs_cur != rhs_end) {
                if (lhs_cur->first < rhs_cur->first) {
                    ++lhs_cur;
                    continue;
                }
                if (lhs_cur->first > rhs_cur->first) {
                    ++rhs_cur;
                    continue;
                }
                func(*lhs_cur, *rhs_cur);
                ++lhs_cur;
                ++rhs_cur;
            }
        }

        template <typename Func>
            requires(std::is_invocable<Func, Map::reference, Map::const_reference>::value)
        auto foreach_overlap_mut(Archive const& rhs, Func&& func) {
            auto lhs_cur = this->entries.begin();
            auto const lhs_end = this->entries.end();
            auto rhs_cur = rhs.entries.begin();
            auto const rhs_end = rhs.entries.end();
            while (lhs_cur != lhs_end && rhs_cur != rhs_end) {
                if (lhs_cur->first < rhs_cur->first) {
                    ++lhs_cur;
                    continue;
                }
                if (lhs_cur->first > rhs_cur->first) {
                    ++rhs_cur;
                    continue;
                }
                func(*lhs_cur, *rhs_cur);
                ++lhs_cur;
                ++rhs_cur;
            }
        }

        template <typename Func>
            requires(std::is_invocable_r<bool, Func, Map::const_reference, Map::const_reference>::value)
        auto erase_overlap(Archive const& rhs, Func&& func) {
            auto lhs_cur = this->entries.begin();
            auto rhs_cur = rhs.entries.begin();
            auto const rhs_end = rhs.entries.end();
            while (lhs_cur != entries.end() && rhs_cur != rhs_end) {
                if (lhs_cur->first < rhs_cur->first) {
                    ++lhs_cur;
                    continue;
                }
                if (lhs_cur->first > rhs_cur->first) {
                    ++rhs_cur;
                    continue;
                }
                if (func(*lhs_cur, *rhs_cur)) {
                    lhs_cur = entries.erase(lhs_cur);
                } else {
                    ++lhs_cur;
                }
                ++rhs_cur;
            }
        }

        auto overlaping(Archive const& upper) const noexcept -> Archive {
            auto result = Archive{};
            foreach_overlap(upper, [&result](Map::const_reference lhs, Map::const_reference rhs) mutable {
                result.entries.insert_or_assign(rhs.first, rhs.second);
            });
            return result;
        }

        auto merge_in(Archive const& other) -> void {
            Map::const_iterator beg = other.entries.begin();
            Map::const_iterator end = other.entries.end();
            for (auto hint = entries.begin(); beg != end; ++beg) {
                hint = entries.insert_or_assign(hint, beg->first, beg->second);
            }
        }

        Map entries;
    };
}

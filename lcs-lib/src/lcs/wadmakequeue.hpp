#ifndef WADMAKEQUEUE_HPP
#define WADMAKEQUEUE_HPP
#include "common.hpp"
#include "wadmake.hpp"
#include "wad.hpp"
#include <memory>
#include <unordered_map>

namespace LCS {
    struct WadMakeQueue {
        WadMakeQueue(WadIndex const& index, bool removeUnknownNames);

        void addItem(fs::path const& srcpath, Conflict conflict);

        void write(fs::path const& dstpath, ProgressMulti& progress) const;

        std::uint64_t size() const noexcept;

        inline auto const& items() const& noexcept {
            return items_;
        }

        inline auto& items() & noexcept {
            return items_;
        }
    private:
        WadIndex const& index_;
        std::map<fs::path, std::unique_ptr<WadMakeBase>> items_;
        mutable std::uint64_t size_ = 0;
        mutable bool sizeCalculated_ = false;
        bool remove_unknown_names_ = false;

        void addItemWad(std::unique_ptr<WadMakeBase> item, Conflict conflict);
    };
}

#endif // WADMAKEQUEUE_HPP

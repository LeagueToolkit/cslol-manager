#ifndef WADMAKEQUEUE_HPP
#define WADMAKEQUEUE_HPP
#include "common.hpp"
#include "wadmake.hpp"
#include "wad.hpp"
#include <variant>

namespace LCS {
    struct WadMakeQueue {
        using Item = std::variant<WadMake, WadMakeCopy>;
        WadMakeQueue(WadIndex const& index);

        void addItem(Item item, Conflict conflict);

        void addItem(fs::path const& path, Conflict conflict);

        void write(fs::path path, ProgressMulti& progress) const;

        size_t size() const noexcept;

        inline auto const& items() const& noexcept {
            return items_;
        }

        inline auto& items() & noexcept {
            return items_;
        }
    private:
        WadIndex const& index_;
        std::unordered_map<std::string, Item> items_;
        mutable size_t size_ = 0;
        mutable bool sizeCalculated_ = false;
    };
}

#endif // WADMAKEQUEUE_HPP

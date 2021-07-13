#include "wadmakequeue.hpp"
#include "error.hpp"
#include "progress.hpp"
#include "conflict.hpp"
#include <numeric>

using namespace LCS;

WadMakeQueue::WadMakeQueue(WadIndex const& index, bool removeUnknownNames)
    : index_(index),
      remove_unknown_names_(removeUnknownNames)
{}

void WadMakeQueue::addItem(fs::path const& srcpath, Conflict conflict) {
    lcs_trace_func(
                lcs_trace_var(srcpath)
                );
    if (fs::is_directory(srcpath)) {
        addItemWad(std::make_unique<WadMake>(srcpath, &index_, remove_unknown_names_), conflict);
    } else {
        addItemWad(std::make_unique<WadMakeCopy>(srcpath, &index_, remove_unknown_names_), conflict);
    }
}

void WadMakeQueue::addItemWad(std::unique_ptr<WadMakeBase> item, Conflict conflict) {
    auto orgpath = item->path();
    auto name = item->identify(index_).value_or(u8"RAW.wad.client");
    if (auto i = items_.find(name); i != items_.end()) {
        auto const& orgpath = i->second->path();
        auto const& newpath = item->path();
        if (conflict == Conflict::Skip) {
            return;
        } else if(conflict == Conflict::Overwrite) {
            i->second = std::move(item);
        } else  if(conflict == Conflict::Abort) {
            lcs_hint(u8"This mod would modify same wad multiple time!");
            raise_wad_conflict(name, orgpath, newpath);
        }
    } else {
        items_.insert(i, std::pair{name, std::move(item)});
    }
}

void WadMakeQueue::write(fs::path const& dstpath, ProgressMulti& progress) const {
    progress.startMulti(items_.size(), size());
    for(auto const& kvp: items_) {
        auto const& name = kvp.first;
        auto const& item = kvp.second;
        item->write(dstpath / name, progress);
    }
    progress.finishMulti();
}

std::uint64_t WadMakeQueue::size() const noexcept {
    if (!sizeCalculated_) {
        size_ = std::accumulate(items_.begin(), items_.end(), std::uint64_t{0},
                                [](std::uint64_t old, auto const& item) -> std::uint64_t {
                return old + item.second->size();
        });
        sizeCalculated_ = true;
    }
    return size_;
}

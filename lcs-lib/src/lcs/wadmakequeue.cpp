#include "wadmakequeue.hpp"
#include "error.hpp"
#include "progress.hpp"
#include "conflict.hpp"
#include <numeric>

using namespace LCS;

WadMakeQueue::WadMakeQueue(WadIndex const& index) : index_(index) {
    lcs_trace_func(
                lcs_trace_var(index_.path())
                );
}

void WadMakeQueue::addItem(fs::path const& srcpath, Conflict conflict) {
    lcs_trace_func(
                lcs_trace_var(srcpath)
                );
    if (fs::is_directory(srcpath)) {
        addItem(std::make_unique<WadMake>(srcpath), conflict);
    } else {
        addItem(std::make_unique<WadMakeCopy>(srcpath), conflict);
    }
}

void WadMakeQueue::addItem(std::unique_ptr<WadMakeBase> item, Conflict conflict) {
    lcs_trace_func(
                lcs_trace_var(item->path())
                );
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
            lcs_hint("This mod would modify same wad multiple time!");
            throw ConflictError(name, orgpath, newpath);
        }
    } else {
        items_.insert(i, std::pair{name, std::move(item)});
    }
}

void WadMakeQueue::write(fs::path const& dstpath, ProgressMulti& progress) const {
    lcs_trace_func(
                lcs_trace_var(dstpath)
                );
    progress.startMulti(items_.size(), size());
    for(auto const& kvp: items_) {
        auto const& name = kvp.first;
        auto const& item = kvp.second;
        item->write(dstpath / name, progress, &index_);
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

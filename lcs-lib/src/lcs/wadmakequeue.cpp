#include "wadmakequeue.hpp"
#include "error.hpp"
#include "progress.hpp"
#include "conflict.hpp"
#include <numeric>

using namespace LCS;

WadMakeQueue::WadMakeQueue(WadIndex const& index) : index_(index) {
    lcs_trace_func();
    lcs_trace("path: ", index_.path());
}

void WadMakeQueue::addItem(fs::path const& path, Conflict conflict) {
    lcs_trace_func();
    lcs_trace("path: ", path);
    if (fs::is_directory(path)) {
        addItem(std::make_unique<WadMake>(path), conflict);
    } else {
        addItem(std::make_unique<WadMakeCopy>(path), conflict);
    }
}

void WadMakeQueue::addItem(std::unique_ptr<WadMakeBase> item, Conflict conflict) {
    lcs_trace_func();
    auto orgpath = item->path();
    lcs_trace("path: ", orgpath);
    auto name = item->identify(index_).value_or("RAW.wad.client");
    if (auto i = items_.find(name); i != items_.end()) {
        auto const& orgpath = i->second->path();
        auto const& newpath = item->path();
        if (conflict == Conflict::Skip) {
            return;
        } else if(conflict == Conflict::Overwrite) {
            i->second = std::move(item);
        } else  if(conflict == Conflict::Abort) {
            throw ConflictError(name, orgpath, newpath);
        }
    } else {
        items_.insert(i, std::pair{name, std::move(item)});
    }
}

void WadMakeQueue::write(fs::path const& path, ProgressMulti& progress) const {
    lcs_trace_func();
    lcs_trace("path: ", path);
    progress.startMulti(items_.size(), size());
    for(auto const& kvp: items_) {
        auto const& name = kvp.first;
        auto const& item = kvp.second;
        item->write(path / name, progress);
    }
    progress.finishMulti();
}

size_t WadMakeQueue::size() const noexcept {
    if (!sizeCalculated_) {
        size_ = std::accumulate(items_.begin(), items_.end(), size_t{0},
                                [](size_t old, auto const& item) -> size_t {
                return old + item.second->size();
        });
        sizeCalculated_ = true;
    }
    return size_;
}

#include "wadmakequeue.hpp"
#include <numeric>
#include <algorithm>
#include <utility>

using namespace LCS;

WadMakeQueue::WadMakeQueue(WadIndex const& index) : index_(index) {}

void WadMakeQueue::addItem(const std::filesystem::path& path, Conflict conflict) {
    if (fs::is_directory(path)) {
        addItem(WadMake(path), conflict);
    } else {
        addItem(WadMakeCopy(path), conflict);
    }
}

void WadMakeQueue::addItem(WadMakeQueue::Item item, Conflict conflict) {
    auto name = std::visit([this](auto const& value) -> std::string {
        auto wad = index_.findOriginal(value.name(), value.entries());
        if (!wad) {
            return "RAW.wad.client";
        }
        return wad->name();
    }, item);
    if (auto i = items_.find(name); i != items_.end()) {
        auto orgpath = std::visit([](auto const& value) -> fs::path const& {
            return value.path();
        }, i->second);
        auto newpath = std::visit([](auto const& value) -> fs::path const& {
            return value.path();
        }, item);
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

void WadMakeQueue::write(fs::path path, ProgressMulti& progress) const {
    path = fs::absolute(path);
    fs::create_directories(path);
    progress.startMulti(items_.size(), size());
    for(auto const& kvp: items_) {
        auto const& name = kvp.first;
        auto const& item = kvp.second;
        std::visit([&](auto const& value){
           value.write(path / name, progress);
        }, item);
    }
    progress.finishMulti();
}

size_t WadMakeQueue::size() const noexcept {
    if (!sizeCalculated_) {
        size_ = std::accumulate(items_.begin(), items_.end(), size_t{0},
                                [](size_t old, auto const& item) -> size_t {
                return old + std::visit([](auto const& value) -> size_t {
                    return value.size();
                }, item.second);
        });
        sizeCalculated_ = true;
    }
    return size_;
}

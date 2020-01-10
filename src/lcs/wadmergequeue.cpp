#include "wadmergequeue.hpp"

using namespace LCS;

WadMergeQueue::WadMergeQueue(const std::filesystem::path& path, WadIndex const& index) :
    path_(fs::absolute(path)), index_(index) {
    fs::create_directories(path_);
}

void WadMergeQueue::addMod(Mod const* mod, Conflict conflict) {
    for(auto const& [name, source]: mod->wads()) {
        addWad(source.get(), conflict);
    }
}

void WadMergeQueue::addWad(Wad const* source, Conflict conflict) {
    auto baseWad = index_.findOriginal(source->name(), source->entries());
    if (baseWad == nullptr) {
        throw std::runtime_error("No base .wad found!");
    }
    auto baseItem = findOrAddItem(baseWad);
    baseItem->addWad(source, conflict);
    for(auto const& entry: source->entries()) {
        for(auto [xxhash, extraWad]: index_.findExtra(entry.xxhash)) {
            if (extraWad != baseWad && extraWad->name() != "Map22.wad.client") {
                auto extraItem = findOrAddItem(extraWad);
                extraItem->addExtraEntry(entry, source, conflict);
            }
        }
    }
}

void WadMergeQueue::write(ProgressMulti& progress) const {
    size_t itemTotal = 0;
    size_t dataTotal = 0;
    for(auto const& [original, item]: items_) {
        itemTotal++;
        dataTotal += item->size();
    }
    progress.startMulti(itemTotal, dataTotal);
    for(auto const& kvp: items_) {
        kvp.second->write(progress);
    }
    progress.finishMulti();
}

void WadMergeQueue::write_whole(ProgressMulti& progress) const {
    size_t itemTotal = 0;
    size_t dataTotal = 0;
    for(auto const& [original, item]: items_) {
        itemTotal++;
        dataTotal += item->size_whole();
    }
    progress.startMulti(itemTotal, dataTotal);
    for(auto const& kvp: items_) {
        kvp.second->write_whole(progress);
    }
    progress.finishMulti();
}

void WadMergeQueue::cleanup() {
    std::set<std::string> good = {};
    for(auto const& [wad, merge]: items_) {
        good.insert(merge.get()->path().generic_string());
    }
    for(auto const& file: fs::recursive_directory_iterator(path_)) {
        if (file.is_regular_file()) {
            auto str = file.path().generic_string();
            if (auto i = good.find(str); i == good.end()) {
                fs::remove(file);
            }
        }
    }
}

WadMerge* WadMergeQueue::findOrAddItem(Wad const* original) {
    if (auto i = items_.find(original); i != items_.end()) {
        return i->second.get();
    } else {
        auto merge = new WadMerge {path_ / fs::relative(original->path(), index_.path()), original};
        items_.emplace(std::pair{original, std::unique_ptr<WadMerge>{merge}});
        return merge;
    }
}


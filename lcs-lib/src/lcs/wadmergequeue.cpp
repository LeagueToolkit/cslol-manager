#include "wadmergequeue.hpp"
#include "error.hpp"
#include "progress.hpp"
#include <set>
#include <utility>

using namespace LCS;

WadMergeQueue::WadMergeQueue(fs::path const& path, WadIndex const& index) :
    path_(fs::absolute(path)), index_(index) {
    lcs_trace_func(
                lcs_trace_var(this->path_)
                );
    fs::create_directories(path_);
}

void WadMergeQueue::addMod(Mod const* mod, Conflict conflict) {
    lcs_trace_func(
                lcs_trace_var(this->path_),
                lcs_trace_var(mod->path())
                );
    for(auto const& [name, source]: mod->wads()) {
        addWad(source.get(), conflict);
    }
}

void WadMergeQueue::addWad(Wad const* source, Conflict conflict) {
    lcs_trace_func(
                lcs_trace_var(this->path_),
                lcs_trace_var(source->path())
                );
    auto baseWad = index_.findOriginal(source->name(), source->entries());
    lcs_assert_msg("No base .wad found!", baseWad);
    auto baseItem = findOrAddItem(baseWad);
    baseItem->addWad(source, conflict);
    for(auto const& entry: source->entries()) {
        for(auto& [xxhash, extraWad]: index_.findExtra(entry.xxhash)) {
            if (extraWad != baseWad) {
                auto extraItem = findOrAddItem(extraWad);
                extraItem->addExtraEntry(entry, source, conflict);
            }
        }
    }
}

void WadMergeQueue::write(ProgressMulti& progress) const {
    lcs_trace_func(
                lcs_trace_var(this->path_)
                );
    std::size_t itemTotal = 0;
    std::uint64_t dataTotal = 0;
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


void WadMergeQueue::cleanup() {
    lcs_trace_func(
                lcs_trace_var(this->path_)
                );
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
    lcs_trace_func(
                lcs_trace_var(this->path_),
                lcs_trace_var(original->name())
                );
    if (auto i = items_.find(original); i != items_.end()) {
        return i->second.get();
    } else {
        auto merge = new WadMerge {path_ / fs::relative(original->path(), index_.path()), original};
        items_.emplace(std::make_pair(original, std::unique_ptr<WadMerge>{merge}));
        return merge;
    }
}


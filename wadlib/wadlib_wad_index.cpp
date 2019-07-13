#include "wadlib.h"

namespace fs = std::filesystem;

wad_index::wad_index(fspath const& src)
{
    std::vector<std::pair<std::string, std::vector<WadEntry>>> local;
    size_t totalFiles = 0;

    for(auto const& ent: fs::recursive_directory_iterator(src)) {
        if(ent.is_directory() || !ent.is_regular_file()) {
            continue;
        }
        auto path = ent.path();
        if(path.extension() != ".client" && path.extension() != ".wad") {
            continue;
        }
        auto& wad = local.emplace_back();
        auto& rel = wad.first;
        auto& entries = wad.second;
        WadHeader header;

        rel = fs::relative(path, src).lexically_normal().generic_string();
        File file(path, L"rb");
        file.read(header);
        entries.resize(header.filecount);
        file.rread(entries.data(), entries.size());
        totalFiles += header.filecount;
    }
    wads.reserve(local.size());
    files.reserve(totalFiles);

    size_t i = 0;
    for(auto& kvp: local) {
        wads.emplace_back(std::move(kvp.first));
        for(auto const& entry: kvp.second) {
            files[entry.xxhash] = i;
        }
        i++;
    }
}

std::string wad_index::find_best(const fspath &src)
{
    std::vector<std::pair<size_t, size_t>> counter;
    counter.resize(wads.size());
    for(size_t i = 0; i < counter.size(); i++) {
        counter[i].second = i;
    }
    File file(src, L"rb");
    WadHeader header;
    file.read(header);
    std::vector<WadEntry> entries;
    entries.resize(header.filecount);
    file.rread(&entries[0], entries.size());
    for(auto const& entry: entries) {
        auto const xx = entry.xxhash;
        if(auto f = files.find(xx); f != files.end()) {
            counter[f->second].first++;
        }
    }
    std::sort(counter.begin(), counter.end(), [](auto const& l, auto const& r) {
       return l.first > r.first;
    });
    if(auto first = counter.front(); first.first) {
        return wads[first.second];
    }
    return {};
}

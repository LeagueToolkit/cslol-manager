#include <xxh3.h>

#include <cstdlib>
#include <cstring>
#include <lol/error.hpp>
#include <lol/io/file.hpp>
#include <lol/wad/archive.hpp>
#include <numeric>
#include <unordered_map>

using namespace lol;
using namespace lol::wad;

auto Archive::read_from_toc(io::Bytes src, TOC const& toc) -> Archive {
    lol_trace_func(lol_trace_var("{}", toc.entries.size()));
    auto descriptors_by_checksum = std::unordered_map<std::uint64_t, EntryData>{};
    descriptors_by_checksum.reserve(toc.entries.size());
    auto archive = Archive{};
    for (auto const& entry : toc.entries) {
        if (entry.loc.checksum) {
            if (auto old = descriptors_by_checksum.find(entry.loc.checksum); old != descriptors_by_checksum.end()) {
                archive.entries[entry.name] = old->second;
                continue;
            }
        }
        auto data = EntryData::from_loc(src, entry.loc);
        archive.entries[entry.name] = data;
        if (entry.loc.checksum) {
            descriptors_by_checksum[entry.loc.checksum] = data;
        }
    }
    return archive;
}

auto Archive::read_from_file(fs::path const& path) -> Archive {
    lol_trace_func(lol_trace_var("{}", path));
    auto src = io::Bytes::from_file(path);
    auto toc = TOC{};
    auto const toc_error = toc.read(src);
    lol_throw_if(toc_error);
    return read_from_toc(src, toc);
}

auto Archive::pack_from_directory(fs::path const& dir) -> Archive {
    lol_trace_func(lol_trace_var("{}", dir));
    auto archive = Archive{};
    for (auto const& dirent : fs::recursive_directory_iterator(dir)) {
        if (!dirent.is_regular_file()) continue;
        archive.add_from_directory(dirent.path(), dir);
    }
    return archive;
}

auto Archive::add_from_directory(fs::path const& path, fs::path const& dir) -> void {
    lol_trace_func(lol_trace_var("{}", path), lol_trace_var("{}", dir));
    auto name = hash::Xxh64::from_path(fs::relative(path, dir));
    auto data = EntryData::from_file(path);
    entries.insert_or_assign(name, data);
}

auto Archive::estimate_size() const noexcept -> std::size_t {
    auto estimate = sizeof(TOC::latest_header_t) + sizeof(TOC::latest_entry_t) * entries.size();
    for (auto const& [name, data] : entries) {
        estimate += data.bytes_size();
    }
    return estimate;
}

auto Archive::write_to_file(fs::path const& path) const -> void {
    lol_trace_func(lol_trace_var("{}", path));
    using HeaderT = TOC::latest_header_t;
    using EntryT = TOC::latest_entry_t;

    auto file = io::File::create(path);

    auto new_header = HeaderT{
        .version = TOC::Version::latest(),
        .signature = {},
        .checksum = {},
        .desc_count = (std::uint32_t)entries.size(),
    };

    {
        auto version = TOC::Version::latest();
        XXH3_state_s hashstate = {};
        XXH3_128bits_reset(&hashstate);
        XXH3_128bits_update(&hashstate, &version, sizeof(version));
        for (auto const& [name, data] : entries) {
            auto const checksum = data.checksum();
            XXH3_128bits_update(&hashstate, &name, sizeof(name));
            XXH3_128bits_update(&hashstate, &checksum, sizeof(checksum));
        }
        auto const hashdigest = XXH3_128bits_digest(&hashstate);
        std::memcpy((char*)&new_header.signature, (char const*)&hashdigest, sizeof(TOC::Signature));
    }

    if (auto old_header = HeaderT{}; file.readsome(0, &old_header, sizeof(HeaderT)) == sizeof(HeaderT)) {
        if (std::memcmp(&old_header, &new_header, sizeof(HeaderT)) == 0) {
            return;
        }
    }

    auto toc_entries = std::vector<EntryT>{};
    toc_entries.reserve(entries.size());

    std::size_t data_cur = sizeof(HeaderT) + sizeof(EntryT) * entries.size();
    {
        // Order any potential reads by address, this guarantees sequential reads on any memory mapped files.
        auto entries_data = std::vector<std::pair<hash::Xxh64, EntryData>>();
        entries_data.reserve(entries.size());
        entries_data.insert(entries_data.end(), entries.begin(), entries.end());
        std::sort(entries_data.begin(), entries_data.end(), [](auto const& l, auto const& r) -> bool {
            return (std::uintptr_t)l.second.bytes_data() < (std::uintptr_t)r.second.bytes_data();
        });

        // Deduplicate data locations by checksum
        auto loc_by_checksum = std::unordered_map<std::uint64_t, EntryLoc>();
        loc_by_checksum.reserve(entries.size() * 2);

        // Write all entries data and generate TOC
        file.reserve(0, estimate_size());
        for (auto const& entry : entries_data) {
            auto loc = EntryLoc{};
            auto data = entry.second.into_optimal();
            auto checksum = data.checksum();
            if (auto j = loc_by_checksum.find(checksum); j != loc_by_checksum.end()) {
                loc = j->second;
            } else {
                loc = {
                    .type = data.type(),
                    .subchunk_count = data.subchunk_count(),
                    .subchunk_index = data.subchunk_index(),
                    .offset = data_cur,
                    .size = data.bytes_size(),
                    .size_decompressed = data.size_decompressed(),
                    .checksum = checksum,
                };
                lol_throw_if(loc.size >= 4 * io::GiB);
                lol_throw_if(loc.size_decompressed >= 4 * io::GiB);
                lol_throw_if(loc.offset >= 4 * io::GiB);
                file.write(data_cur, data.bytes_data(), data.bytes_size());
                loc_by_checksum.emplace_hint(j, checksum, loc);
                data_cur += loc.size;
            }
            toc_entries.push_back(EntryT{
                .name = entry.first,
                .offset = (std::uint32_t)loc.offset,
                .size = (std::uint32_t)loc.size,
                .size_decompressed = (std::uint32_t)loc.size_decompressed,
                .type = loc.type,
                .subchunk_count = loc.subchunk_count,
                .subchunk_index = loc.subchunk_index,
                .checksum = loc.checksum,
            });
        };
    }

    {
        // TOC entries MUST be sorted by name
        std::sort(toc_entries.begin(), toc_entries.end(), [](auto const& l, auto const& r) -> bool {
            return l.name < r.name;
        });
    }

    // Write TOC after header
    file.write(sizeof(HeaderT), toc_entries.data(), toc_entries.size() * sizeof(EntryT));

    // Write header
    file.write(0, &new_header, sizeof(HeaderT));

    // Trunc data
    file.resize(data_cur);
}

auto Archive::touch() -> void {
    // Order any potential reads by address, this guarantees sequential reads on any memory mapped files.
    auto entries = std::vector<std::pair<hash::Xxh64, EntryData>>();
    entries.reserve(entries.size());
    entries.insert(entries.end(), entries.begin(), entries.end());
    std::sort(entries.begin(), entries.end(), [](auto const& l, auto const& r) -> bool {
        return (std::uintptr_t)l.second.bytes_data() < (std::uintptr_t)r.second.bytes_data();
    });

    for (auto& [name, data] : entries) {
        data.touch();
    }
}

auto Archive::mark_optimal() -> void {
    for (auto& [name, data] : entries) {
        data.mark_optimal(true);
    }
}

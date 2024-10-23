#include <cstdlib>
#include <cstring>
#include <lol/error.hpp>
#include <lol/wad/toc.hpp>

using namespace lol;
using namespace lol::wad;

auto TOC::read(io::Bytes src) noexcept -> char const* {
    if (src.size() < sizeof(TOC::Version)) {
        // NOTE: we atempt to ignore empty all 0 files early, proper solution is to handle locales
        return nullptr;
    }
    std::memcpy(&version, src.data(), sizeof(TOC::Version));
    if (std::memcmp(&version, "\0\0\0\0", 4) == 0) {
        // NOTE: we atempt to ignore empty all 0 files early, proper solution is to handle locales
        return nullptr;
    }
    if (!version.is_wad()) {
        return "Bad Version::magic.";
    }
    auto read_raw = [&](io::Bytes src, auto header_raw, auto entry_raw) mutable noexcept -> char const* {
        if (src.size() < sizeof(header_raw)) {
            return "Not enough data for Header";
        }
        std::memcpy(&header_raw, src.data(), sizeof(header_raw));
        if (header_raw.desc_size != sizeof(entry_raw)) {
            return "Bad Header::desc_size.";
        }
        if (src.size() < header_raw.desc_offset) {
            return "Bad Header::desc_offset.";
        }
        if (src.size() - header_raw.desc_offset < sizeof(entry_raw) * header_raw.desc_count) {
            return "Not enough data for TOC.";
        }
        entries.clear();
        entries.reserve(header_raw.desc_count);
        for (std::uint32_t i = 0; i != header_raw.desc_count; ++i) {
            std::memcpy(&entry_raw, src.data() + header_raw.desc_offset + sizeof(entry_raw) * i, sizeof(entry_raw));
            entries.emplace_back(Entry{
                .name = hash::Xxh64(entry_raw.name),
                .loc =
                    {
                        .type = entry_raw.type,
                        .subchunk_count = entry_raw.subchunk_count,
                        .subchunk_index = entry_raw.subchunk_index,
                        .offset = entry_raw.offset,
                        .size = entry_raw.size,
                        .size_decompressed = entry_raw.size_decompressed,
                        .checksum = entry_raw.checksum,
                    },
            });
        }
        signature = header_raw.signature;
        return nullptr;
    };
    switch (version.major) {
        case 0:
        case 1:
            return read_raw(src, HeaderV1{}, EntryV1{});
        case 2:
            return read_raw(src, HeaderV2{}, EntryV2{});
        case 3:
            switch (version.minor) {
                case 0:
                    return read_raw(src, HeaderV3{}, EntryV3{});
                case 1:
                case 2:
                case 3:
                    return read_raw(src, HeaderV3{}, EntryV3_1{});
                case 4:
                default:
                    return read_raw(src, HeaderV3{}, EntryV3_4{});
            }
        default:
            return "Bad Wad::Version::major.";
    }
}

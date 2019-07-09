#include "wadlib.h"
#include "xxhash.h"

namespace fs = std::filesystem;

void wad_merge(
        fspath const& dstpath,
        std::optional<WadFile> const& base,
        wad_files const& wad_files,
        updatefn update,
        int32_t const buffercap) {
    std::error_code errorc;
    fs::create_directories(dstpath.parent_path(), errorc);
    if(errorc) {
        throw File::Error(errorc.message().c_str());
    }

    WadHeader header{};
    std::map<uint64_t, WadEntry> entries{};

    // A pointer to every WadFile with base WadFile first
    std::vector<WadFile const*> wad_files_ptr;
    wad_files_ptr.reserve(wad_files.size() + 1);

    int64_t bdone = 0;
    int64_t btotal = 0;

    XXH64_state_s xxstate{};
    XXH64_reset(&xxstate, 0);
    // Reads in the base .wad if provided
    if(base) {
        btotal += base->data_size;
        for(auto const& [xx, entry]: base->entries) {
            XXH64_update(&xxstate, &entry, sizeof(WadEntry));
        }
        wad_files_ptr.push_back(&*base);
        header.signature = base->header.signature;
    } else if(wad_files.size()) {
        header.signature = wad_files.begin()->second.header.signature;
    }

    // Reads in all wads in their order
    for(auto const& [xx, wad_file]: wad_files) {
        btotal += wad_file.data_size;
        for(auto const& [xx, entry]: wad_file.entries) {
            XXH64_update(&xxstate, &entry, sizeof(WadEntry));
        }
        wad_files_ptr.push_back(&wad_file);
    }
    uint64_t currentChecksum = XXH64_digest(&xxstate);

    std::string const dststr = dstpath.generic_string();
    // Check if file exists and compares the TOC checksum
    if(fs::exists(dstpath) && fs::is_regular_file(dstpath) && !fs::is_directory(dstpath)) {
        File dstfile(dstpath, L"rb");
        WadHeader dstheader{};
        dstfile.read(dstheader);
        if(dstheader.checksum == currentChecksum) {
            if(update) {
                update(dststr, 0, 0);
            }
            return;
        }
    }
    header.checksum = currentChecksum;

    File dstfile(dstpath, L"wb");
    // Pre-allocate new wad entries and calculate new start offset
    for(auto const& wad_file: wad_files_ptr) {
        for(auto const& [xx, entry]: wad_file->entries) {
            entries[xx] = {};
        }
    }
    int32_t offset = static_cast<int32_t>(WadHeader::SIZE + (entries.size() * WadEntry::SIZE));
    dstfile.seek_beg(offset);

    // Copy contents
    std::vector<uint8_t> buffer(static_cast<size_t>(buffercap), uint8_t(0));
    for(auto const& wad_file: wad_files_ptr) {
        offset = dstfile.tell();
        for(auto const& [xx, entry]: wad_file->entries) {
            auto& copy = entries[xx];
            copy = {
                entry.xxhash,
                entry.dataOffset - wad_file->data_start + offset,
                entry.sizeCompressed,
                entry.sizeUncompressed,
                entry.type,
                entry.isDuplicate || copy.isDuplicate,
                {0,0},
                entry.sha256
            };
        }

        wad_file->file.seek_beg(wad_file->data_start);
        for(int32_t i = 0; i < wad_file->data_size; i += buffercap) {
            auto const size = std::min(wad_file->data_size - i, buffercap);
            buffer.resize(static_cast<size_t>(size));
            wad_file->file.read(buffer);
            dstfile.write(buffer);
            if(update) {
                bdone += size;
                update(dststr, bdone, btotal);
            }
        }
    }

    dstfile.seek_beg(static_cast<int32_t>(WadHeader::SIZE));
    header.filecount = 0;
    for(auto const& [xx, entry] : entries) {
        if(entry.type == 2 && entry.sizeCompressed <= 5 && entry.sizeUncompressed <= 5) {
            continue;
        }
        dstfile.write(entry);
        header.filecount++;
    }
    dstfile.seek_beg(0);
    dstfile.write(header);
}


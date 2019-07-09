#include "wadlib.h"
#include "picosha2.hpp"
#include "zlib.h"
#include "zstd.h"

namespace fs = std::filesystem;

static inline uint64_t str2xxh64(fspath const& path) noexcept {
    std::string str = path.stem().generic_string();
    char* next = nullptr;
    auto result = strtoull(&str[0], &next, 16);
    if(next == &str[str.size()]) {
        return result;
    }
    return xx64_lower(path.lexically_normal().generic_string());
}

void wad_make(
        fspath const& dstpath,
        fspath const& src,
        updatefn update,
        int32_t const buffercap) {
    WadHeader header{};
    std::map<uint64_t, WadEntry> entries{};
    std::map<uint64_t, std::string> redirects{};
    std::map<uint64_t, std::string> files{};

    std::string const dststr = dstpath.generic_string();

    std::error_code errorc;
    fs::create_directories(dstpath.parent_path(), errorc);
    if(errorc) {
        throw File::Error(errorc.message().c_str());
    }
    File dstfile(dstpath, L"wb");

    int64_t bdone = 0;

    int64_t btotal = 0;
    std::vector<uint8_t> buffer(static_cast<size_t>(buffercap), uint8_t(0));

    for(auto const& top: fs::directory_iterator(src)) {
        if(top.is_directory()) {
            for(auto const& dir_ent: fs::recursive_directory_iterator(top)) {
                if(dir_ent.is_directory()) {
                    continue;
                }
                auto relative = fs::relative(dir_ent, src).lexically_normal().generic_string();
                auto const h = xx64_lower(std::move(relative));
                files[h] = dir_ent.path().generic_string();
                btotal += static_cast<int32_t>(dir_ent.file_size());
            }
        } else if(top.path().filename() == "redirections.txt") {
            File orgfile(top, L"rb");
            for(;!orgfile.is_eof();) {
                std::string line = orgfile.readline();
                auto const h_start = 0;
                auto const h_end = line.find_first_of(':');
                if(h_end == line.size() || h_end < 2) {
                    continue;
                }
                auto const n_start = h_end + 1;
                auto const n_end = line.size() - n_start;
                auto const h = str2xxh64(line.substr(h_start, h_end));
                auto const n = line.substr(n_start, n_end);
                redirects[h] = std::move(n);
            }
        } else {
            auto const h = str2xxh64(fs::relative(top, src));
            files[h] = top.path().generic_string();
            btotal += static_cast<int32_t>(top.file_size());
        }
    }
    int32_t offset = static_cast<int32_t>(WadHeader::SIZE +
                                          ((files.size() + redirects.size()) * WadEntry::SIZE));
    dstfile.seek_beg(offset);
    for(auto const& [xx, filepath]: files) {
        File file(filepath, L"rb");
        int32_t filesize = file.size();
        auto& entry = entries[xx];
        entry.xxhash = xx;
        entry.type = 0;
        entry.sizeCompressed = filesize;
        entry.sizeUncompressed = filesize;
        entry.dataOffset = dstfile.tell();

        picosha2::hash256_one_by_one sha256;
        sha256.init();
        for(int32_t i = 0; i < filesize; i += buffercap) {
            auto const size = std::min(filesize - i, buffercap);
            buffer.resize(static_cast<size_t>(size));
            file.read(buffer);
            dstfile.write(buffer);
            sha256.process(buffer.cbegin(), buffer.cend());
            if(update) {
                bdone += size;
                update(dststr, bdone, btotal);
            }
        }
        sha256.finish();
        sha256.get_hash_bytes(entry.sha256.begin(), entry.sha256.end());
    }


    for(auto const& [xx, redir] : redirects) {
        WadEntry entry = {};
        entry.xxhash = xx;
        entry.type = 2;
        entry.sizeCompressed = static_cast<int32_t>(redir.size() + 5);
        entry.sizeUncompressed = static_cast<int32_t>(redir.size() + 5);
        uint32_t size = static_cast<uint32_t>(redir.size());
        entry.sha256 = {};
        entry.dataOffset = dstfile.tell();
        dstfile.write(size);
        dstfile.write(redir);
        dstfile.write(uint8_t{0});
        entries[xx] = std::move(entry);
    }

    header.filecount = static_cast<uint32_t>(entries.size());
    dstfile.seek_beg(0);
    dstfile.write(header);

    for(auto const& [xx, entry]: entries) {
        dstfile.write(entry);
    }
}

#include "wxyextract.hpp"
#include "error.hpp"
#include "progress.hpp"
#include "utility.hpp"
#include "iofile.hpp"
#include <miniz.h>
#include <xxhash.h>
#include <json.hpp>


using namespace LCS;

using json = nlohmann::json;

namespace {
    static inline constexpr uint8_t methodNone = 0;
    static inline constexpr uint8_t methodDeflate = 1;
    static inline constexpr uint8_t methodZlib = 2;

    inline constexpr static const char prefixDeploy[] = "deploy/";
    inline constexpr static const char prefixProjects[] = "projects/";

    static inline uint64_t xxhashStr(std::string str, bool isProject = false) {
        XXH64_state_s xxstate{};
        XXH64_reset(&xxstate, 0);
        if(isProject) {
            XXH64_update(&xxstate, prefixProjects, sizeof(prefixProjects) - 1);
        } else {
            XXH64_update(&xxstate, prefixDeploy, sizeof(prefixDeploy) - 1);
        }
        std::transform(str.begin(), str.end(), str.begin(), ::tolower);
        XXH64_update(&xxstate, str.c_str(), str.size());
        return XXH64_digest(&xxstate);
    }

    template<typename T>
    static inline void read(InFile& file, T& value) {
        static_assert(std::is_arithmetic_v<T> || std::is_enum_v<T>);
        file.read((char*)&value, sizeof(T));
    }

    template<typename T, size_t S>
    static inline void read(InFile& file, std::array<T, S>& value) {
        static_assert(std::is_arithmetic_v<T> || std::is_enum_v<T>);
        file.read((char*)&value, sizeof(T) * S);
    }

    static inline void read(InFile& file, std::string& value, int32_t length) {
        value.clear();
        if(length > 0) {
            auto const start = file.tell();
            file.seek(0, SEEK_END);
            auto const end = file.tell();
            file.seek(start, SEEK_SET);
            if(length > (end - start)) {
                throw std::runtime_error("Sized string to big!");
            }
            value.resize((size_t)(length));
            file.read(value.data(), length);
        }
    }

    static inline void read(InFile& file, std::string& value) {
        value.clear();
        int32_t length;
        read(file, length);
        read(file, value, length);
    }
}

WxyExtract::WxyExtract(fs::path const& path)
    : path_(fs::absolute(path)), file_(path) {
    lcs_trace_func();
    lcs_trace("path_: ", path_);

    std::array<char, 4> magic;
    read(file_, magic);
    if(magic != std::array{'W', 'X', 'Y', 'S'}) {
        throw std::runtime_error("Not a .wxy file!");
    }
    read(file_, wxyVersion_);
    if(wxyVersion_ < 6) {
        read_old();
    } else {
        read_oink();
    }
    build_paths();
}

void WxyExtract::decompressStr(std::string& str) const {
    lcs_trace_func();
    if (str.empty()) {
        return;
    }
    char buffer[1024];
    mz_stream strm = {};
    if (wxyVersion_ < 6) {
        str.insert(str.begin(), 0x78);
        lcs_assert(mz_inflateInit2(&strm, 15) == MZ_OK);
    } else {
        lcs_assert(mz_inflateInit2(&strm, -15) == MZ_OK);
    }
    strm.next_in = (unsigned char const*)(str.data());
    strm.avail_in = (unsigned int)(str.size());
    strm.next_out = (unsigned char*)buffer;
    strm.avail_out = (unsigned int)(sizeof(buffer));
    mz_inflate(&strm, MZ_FINISH);
    mz_inflateEnd(&strm);
    str.resize((size_t)strm.total_out);
    std::copy(buffer, buffer + strm.total_out, str.data());
}

void WxyExtract::decryptStr(std::string& str) const {
    if (wxyVersion_ > 6) {
        int32_t i = 0;
        int32_t nameSize = static_cast<int32_t>(str.size());
        for(auto& c : str) {
            uint8_t b = (uint8_t)c;
            b = (uint8_t)(b ^ (uint8_t)(i * 17 + (nameSize - i) * 42));
            c = (char)b;
            i++;
        }
    }
}

void WxyExtract::decryptStr2(std::string& str) const {
    if (wxyVersion_ > 6) {
        int32_t i = 0;
        int32_t nameSize = static_cast<int32_t>(str.size());
        for(auto& c : str) {
            uint8_t b = (uint8_t)c;
            b = (uint8_t)(b ^ (uint8_t)(i * 42 + (nameSize - i) * 17));
            c = (char)b;
            i++;
        }
    }
}

void WxyExtract::read_old() {
    lcs_trace_func();
    read(file_, name_);
    read(file_, author_);
    read(file_, version_);
    decompressStr(name_);
    decompressStr(author_);
    decompressStr(version_);

    if(wxyVersion_ >= 3) {
        read(file_, category_);
        read(file_, subCategory_);
        decompressStr(category_);
        decompressStr(subCategory_);
        int32_t imageCount = 0;
        read(file_, imageCount);
        for(int32_t i = 1; i <= imageCount; i++) {
            uint32_t size = 0;
            bool main = false;
            read(file_, size);
            read(file_, main);
            uint32_t offset = (uint32_t)file_.tell();
            file_.seek(size, SEEK_CUR);
            previews_.push_back(Preview { Preview::Image, main, offset + 1, size - 5 });
        }
    } else {
        uint32_t size = 0;
        read(file_, size);
        if (size != 0) {
            uint32_t offset = (uint32_t)file_.tell();
            file_.seek(size, SEEK_CUR);
            previews_.push_back(Preview { Preview::Image, true, offset + 1, size - 5 });
        }
    }

    std::vector<std::string> projectsList;

    int32_t projectCount;
    read(file_, projectCount);
    for(int32_t i = 0; i < projectCount; i++) {
        auto& str = projectsList.emplace_back();
        read(file_, str);
        decompressStr(str);
    }

    if(wxyVersion_ >= 4) {
        int32_t deleteCount;
        read(file_, deleteCount);
        for(int32_t i = 0; i < deleteCount; i++) {
            int32_t projectIndex;
            read(file_, projectIndex);
            if(projectIndex >= projectCount || projectIndex < 0) {
                throw std::runtime_error("delete file doesn't reference valid projcet!");
            }
            auto& skn = deleteList_.emplace_back();
            skn.project =  projectsList[static_cast<size_t>(i)];
            read(file_, skn.fileGamePath);
            decompressStr(skn.fileGamePath);
        }
    }

    int32_t fileCount;
    read(file_, fileCount);
    for(int32_t i = 0; i < fileCount; i++) {
        int32_t projectIndex;
        read(file_, projectIndex);
        if(projectIndex >= projectCount || projectIndex < 0) {
            throw std::runtime_error("delete file doesn't reference valid projcet!");
        }
        auto& skn = filesList_.emplace_back();
        skn.project = projectsList[static_cast<size_t>(projectIndex)];
        read(file_, skn.fileGamePath);
        read(file_, skn.uncompresedSize);
        read(file_, skn.compressedSize);
        if(wxyVersion_ != 1) {
            read(file_, skn.checksum);
        }
        skn.compressionMethod = skn.compressedSize == skn.uncompresedSize ? 0 : 2;
        decompressStr(skn.fileGamePath);
    }

    int32_t offset = (int32_t)file_.tell();
    for(int32_t i = 0; i < fileCount; i++) {
        auto& skn = filesList_[static_cast<size_t>(i)];
        skn.offset = offset;
        offset += skn.compressedSize;
        if(wxyVersion_ <= 4) {
            offset -= 1;
        } else {
            offset -= 2;
        }
    }
}

void WxyExtract::read_oink() {
    lcs_trace_func();
    std::array<char, 4> magic;
    read(file_, magic);
    if(magic != std::array{'O', 'I', 'N', 'K'}) {
        throw std::runtime_error("Not a .wxy file!");
    }

    struct {
        uint16_t name;
        uint16_t author;
        uint16_t version;
        uint16_t category;
        uint16_t subCategory;
    } cLens; // compressed lengths
    read(file_, cLens.name);
    read(file_, cLens.author);
    read(file_, cLens.version);
    read(file_, cLens.category);
    read(file_, cLens.subCategory);

    read(file_, name_, cLens.name);
    read(file_, author_, cLens.author);
    read(file_, version_, cLens.version);
    read(file_, category_, cLens.category);
    read(file_, subCategory_, cLens.subCategory);

    decryptStr2(name_);
    decryptStr2(author_);
    decryptStr2(version_);
    decryptStr2(category_);
    decryptStr2(subCategory_);

    decompressStr(name_);
    decompressStr(author_);
    decompressStr(version_);
    decompressStr(category_);
    decompressStr(subCategory_);

    int32_t previewContentCount;
    read(file_, previewContentCount);
    for(int32_t i = 0; i < previewContentCount; i++) {
        uint8_t type;
        read(file_, type);
        uint32_t size = 0;
        read(file_, size);
        uint32_t offset = (uint32_t)file_.tell();
        file_.seek(size, SEEK_CUR);
        previews_.push_back(Preview { (Preview::Type)type, true, offset, size });
    }

    uint16_t contentCount;
    read(file_, contentCount);

    std::vector<std::string> projectsList;
    for(int32_t i = 0; i < contentCount; i++) {
        uint16_t pLength;
        read(file_, pLength);
        auto& p = projectsList.emplace_back();
        read(file_, p, pLength);
    }

    for(int32_t c = 0; c < contentCount; c++) {
        int32_t fileCount;
        read(file_, fileCount);

        std::vector<std::pair<uint64_t, std::string>> fileNames;

        for(int32_t f = 0; f < fileCount; f++) {
            uint16_t nameLength;
            auto& kvp = fileNames.emplace_back();
            read(file_, nameLength);
            read(file_, kvp.second, nameLength);
            decryptStr(kvp.second);
            kvp.first = xxhashStr(kvp.second);
        }

        std::sort(fileNames.begin(), fileNames.end(), [](auto const& l, auto const& r){
            return l.first < r.first;
        });

        for(int32_t f = 0; f < fileCount; f++) {
            auto& skn = filesList_.emplace_back();
            read(file_, skn.checksum2);
            read(file_, skn.checksum);
            read(file_, skn.adler);
            read(file_, skn.uncompresedSize);
            read(file_, skn.compressedSize);
            read(file_, skn.compressionMethod);

            if(wxyVersion_ > 6) {
                skn.adler = skn.adler
                        ^ static_cast<uint32_t>(skn.uncompresedSize)
                        ^ static_cast<uint32_t>(skn.compressedSize)
                        ^ 0x2Au;
            }

            skn.project = projectsList[static_cast<size_t>(c)];
            skn.fileGamePath = fileNames[static_cast<size_t>(f)].second;
        }
    }
    int32_t offset = (int32_t)file_.tell();
    for(auto& skn: filesList_) {
        skn.offset = offset;
        offset += skn.compressedSize;
    }
}

void WxyExtract::build_paths() {
    lcs_trace_func();
    for(auto& file: filesList_) {
        fs::path path = file.fileGamePath;
        auto beg = path.begin();
        auto end = path.end();
        for (auto i = beg; i != end; i++) {
            std::string name = i->generic_string();
            std::transform(name.begin(), name.end(), name.begin(), ::tolower);
            if (name == "levels" || name == "data" || name == "assets") {
                file.pathFirst = name;
                beg = i;
                break;
            }
        }
        if (auto i = beg; i != end) {
            file.path = *i;
            i++;
            while (i != end) {
                file.path /= *i;
                i++;
            }
        }
    }
}

void WxyExtract::extract_files(fs::path const& dest, Progress& progress) const {
    lcs_trace_func();
    lcs_trace("dest: ", dest);
    size_t total = 0;
    size_t maxUncompressed = 0;
    size_t maxCompressed = 0;
    for(auto const& entry: filesList_) {
        maxCompressed = std::max(maxCompressed, (size_t)entry.compressedSize);
        maxUncompressed = std::max(maxUncompressed, (size_t)entry.uncompresedSize);
        total += (size_t)entry.compressedSize;
    }

    progress.startItem(path_, total);
    std::vector<char> uncompressedBuffer;
    std::vector<char> compressedBuffer;
    uncompressedBuffer.resize(maxUncompressed+6);
    compressedBuffer.resize(maxCompressed+6);

    for(auto const& entry: filesList_) {
        file_.seek(entry.offset, SEEK_SET);
        if(entry.compressionMethod == methodNone) {
            file_.read(uncompressedBuffer.data(), entry.uncompresedSize);
        } else if(entry.compressionMethod == methodZlib) {
            if(wxyVersion_ < 6) {
                uint16_t extra = wxyVersion_ <= 4 ? 1 : 2;
                file_.seek(-extra, SEEK_CUR);
                file_.read(compressedBuffer.data(), entry.compressedSize);
                compressedBuffer[0] = 0x78;
                if(extra > 1) {
                    compressedBuffer[1] = -0x64;
                }
            } else {
                file_.read(compressedBuffer.data(), entry.compressedSize);
            }
            mz_stream strm = {};
            lcs_assert(mz_inflateInit2(&strm, 15) == MZ_OK);
            strm.next_in = (unsigned char const*)compressedBuffer.data();
            strm.avail_in = (unsigned int)(entry.compressedSize);
            strm.next_out = (unsigned char*)uncompressedBuffer.data();
            strm.avail_out = (unsigned int)(entry.uncompresedSize);
            mz_inflate(&strm, MZ_FINISH);
            mz_inflateEnd(&strm);
        } else if(entry.compressionMethod == methodDeflate) {
            file_.read(compressedBuffer.data(), entry.compressedSize);
            mz_stream strm = {};
            lcs_assert(mz_inflateInit2(&strm, -15) == MZ_OK);
            strm.next_in = (unsigned char const*)compressedBuffer.data();
            strm.avail_in = (unsigned int)(entry.compressedSize);
            strm.next_out = (unsigned char*)uncompressedBuffer.data();
            strm.avail_out = (unsigned int)(entry.uncompresedSize);
            mz_inflate(&strm, MZ_FINISH);
            mz_inflateEnd(&strm);
        } else {
            throw_error("Unknow compression method!");
        }
        fs::path outpath = dest / entry.fileGamePath;
        fs::create_directories(outpath.parent_path());

        auto outfile = OutFile(outpath);
        outfile.write(uncompressedBuffer.data(), entry.uncompresedSize);
        progress.consumeData((std::size_t)(entry.compressedSize));
    }

    progress.finishItem();
}

void WxyExtract::extract_meta(fs::path const& dest, Progress& progress) const {
    lcs_trace_func();
    lcs_trace("dest: ", dest);
    fs::create_directories(dest);
    {
        json j = {
            { "Name", name_ },
            { "Author", author_ },
            { "Version", version_ },
            { "Description", "Converted from .wxy" },
        };
        auto jstr = j.dump(2);
        auto outfile = OutFile(dest / "info.json");
        outfile.write(jstr.data(), jstr.size());
    }
    size_t total = 0;
    size_t maxCompressed = 0;
    for(auto const& preview: previews_) {
        if(preview.type == Preview::Image) {
            total += preview.size;
            maxCompressed = std::max(maxCompressed, (size_t)preview.size);
        }
    }
    progress.startItem(dest, total);

    std::vector<char> compressedBuffer;
    std::vector<char> uncompressedBuffer;
    compressedBuffer.resize(maxCompressed);
    uncompressedBuffer.resize(64 * 1024 * 1024); // ¯\_(ツ)_/ works...

    size_t i = 0;
    bool foundFirst = false;
    for(auto const& preview: previews_) {
        if (preview.type != Preview::Image) {
            continue;
        }
        file_.seek(preview.offset, SEEK_SET);
        file_.read(compressedBuffer.data(), preview.size);
        mz_stream strm = {};
        lcs_assert(mz_inflateInit2(&strm, -15) == MZ_OK);
        strm.next_in = (unsigned char const*)compressedBuffer.data();
        strm.avail_in = (unsigned int)(preview.size);
        strm.next_out = (unsigned char*)uncompressedBuffer.data();
        strm.avail_out = (unsigned int)(uncompressedBuffer.size());
        mz_inflate(&strm, MZ_FINISH);
        mz_inflateEnd(&strm);

        auto extension = ScanExtension(uncompressedBuffer.data(), (size_t)(strm.total_out));
        if (!extension.empty()) {
            fs::path outpath = dest;
            if (!foundFirst && extension == "png") {
                outpath /= "image.png";
                foundFirst = true;
            } else {
                outpath /= "image" + std::to_string(i) + "." + extension;
                i++;
            }
            auto outfile = OutFile(outpath);
            outfile.write(uncompressedBuffer.data(), strm.total_out);
        }

        progress.consumeData((size_t)preview.size);
    }

    progress.finishItem();
}


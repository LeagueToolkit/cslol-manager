#include "wxyextract.hpp"
#include <miniz.h>
#include <type_traits>
#include <xxhash.h>
#include <numeric>
#include <algorithm>

using namespace LCS;

namespace {
    static inline constexpr uint8_t methodNone = 0;
    static inline constexpr uint8_t methodDeflate = 1;
    static inline constexpr uint8_t methodZlib = 2;

    static inline void decompressStr(std::string& str) {
        str.insert(str.begin(), 'x');
        char buffer[1024];
        mz_stream strm = {};
        if (mz_inflateInit2(&strm, 15) != Z_OK) {
            throw std::runtime_error("Failed to init uncompress stream!");
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

    static const char prefixDeploy[] = "deploy/";
    static const char prefixProjects[] = "projects/";

    static inline void decryptStr(std::string& str) {
        int32_t i = 0;
        int32_t nameSize = static_cast<int32_t>(str.size());
        for(auto& c : str) {
            auto& b = reinterpret_cast<uint8_t&>(c);
            b = b ^ static_cast<uint8_t>(i * 17 + (nameSize - i) * 42);
            i++;
        }
    }

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
    static inline void read(std::ifstream& file, T& value) {
        static_assert(std::is_arithmetic_v<T>);
        file.read((char*)&value, sizeof(T));
    }

    template<typename T, size_t S>
    static inline void read(std::ifstream& file, std::array<T, S>& value) {
        static_assert(std::is_arithmetic_v<T>);
        file.read((char*)&value, sizeof(T) * S);
    }

    static inline void read(std::ifstream& file, std::string& value, int32_t length) {
        value.clear();
        if(length > 0) {
            auto const start = file.tellg();
            file.seekg(0, std::ios::end);
            auto const end =file.tellg();
            file.seekg(start, std::ios::beg);
            if(length > (end - start)) {
                throw std::runtime_error("Sized string to big!");
            }
            value.resize((size_t)(length));
            file.read(value.data(), length);
        }
    }

    static inline void read(std::ifstream& file, std::string& value) {
        value.clear();
        int32_t length;
        read(file, length);
        read(file, value, length);
    }
}

WxyExtract::WxyExtract(fs::path const& path)
    : path_(fs::absolute(path)) {
    file_.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    file_.open(path_, std::ios::binary);

    std::array<char, 4> magic;
    read(file_, magic);
    if(magic != std::array{'W', 'X', 'Y', 'S'}) {
        throw std::runtime_error("Not a .wxy file!");
    }
    read(file_, wooxyFileVersion);
    if(wooxyFileVersion < 6) {
        read_old();
    } else {
        read_oink();
    }
}


void WxyExtract::extract_files(fs::path const& dest, Progress& progress) const {
    size_t total = 0;
    size_t maxUncompressed = 0;
    size_t maxCompressed = 0;
    for(auto const& entry: this->filesList) {
        maxCompressed = std::max(maxCompressed, (size_t)entry.compressedSize);
        maxUncompressed = std::max(maxUncompressed, (size_t)entry.uncompresedSize);
        total += (size_t)entry.compressedSize;
    }

    progress.startItem(path_, total);
    std::vector<char> uncompressedBuffer;
    std::vector<char> compressedBuffer;
    uncompressedBuffer.reserve(maxUncompressed+6);
    compressedBuffer.reserve(maxCompressed+6);

    for(auto const& entry: this->filesList) {
        file_.seekg((std::streamoff)entry.offset, std::ios::beg);
        if(entry.compressionMethod == methodNone) {
            file_.read(uncompressedBuffer.data(), (std::streamsize)entry.uncompresedSize);
        } else if(entry.compressionMethod == methodZlib) {
            if(this->wooxyFileVersion < 6) {
                uint16_t extra = this->wooxyFileVersion <= 4 ? 1 : 2;
                file_.seekg((std::streamoff)-extra, std::ios::cur);
                file_.read(compressedBuffer.data(), entry.compressedSize);
                compressedBuffer[0] = 0x78;
                if(extra > 1) {
                    compressedBuffer[1] = -0x64;
                }
            } else {
                file_.read(compressedBuffer.data(), entry.compressedSize);
            }
            mz_stream strm = {};
            if (mz_inflateInit2(&strm,15) != MZ_OK) {
                throw std::runtime_error("Failed to init uncompress stream!");
            }
            strm.next_in = (unsigned char const*)compressedBuffer.data();
            strm.avail_in = (unsigned int)(entry.compressedSize);
            strm.next_out = (unsigned char*)uncompressedBuffer.data();
            strm.avail_out = (unsigned int)(entry.uncompresedSize);
            mz_inflate(&strm, MZ_FINISH);
            mz_inflateEnd(&strm);
        } else if(entry.compressionMethod == methodDeflate) {
            file_.read(compressedBuffer.data(), entry.compressedSize);
            mz_stream strm = {};
            if (mz_inflateInit2(&strm,-15) != MZ_OK) {
                throw std::runtime_error("Failed to init uncompress stream!");
            }
            strm.next_in = (unsigned char const*)compressedBuffer.data();
            strm.avail_in = (unsigned int)(entry.compressedSize);
            strm.next_out = (unsigned char*)uncompressedBuffer.data();
            strm.avail_out = (unsigned int)(entry.uncompresedSize);
            mz_inflate(&strm, MZ_FINISH);
            mz_inflateEnd(&strm);
        } else {
            throw std::runtime_error("Unknow compression method!");
        }
        fs::path outpath = dest / entry.fileGamePath;
        fs::create_directories(outpath.parent_path());

        std::ofstream outfile;
        outfile.exceptions(std::ofstream::failbit | std::ofstream::badbit);
        outfile.open(outpath, std::ios::binary);
        outfile.write(uncompressedBuffer.data(), entry.uncompresedSize);
        progress.consumeData((size_t)(entry.compressedSize));
    }

    progress.finishItem();
}

void WxyExtract::read_old() {
    read(file_, this->skinName);
    read(file_, this->skinAuthor);
    read(file_, this->skinVersion);

    if(this->wooxyFileVersion >= 3) {
        read(file_, this->category);
        read(file_, this->subCategory);
        int32_t imageCount = 0;
        read(file_, imageCount);
        for(int32_t i = 1; i <= imageCount; i++) {
            int32_t imageSize = 0;
            read(file_, imageSize);
            file_.seekg(imageSize + 1, std::ios::cur);
        }
    }

    if(this->wooxyFileVersion < 3) {
        int32_t imageSize = 0;
        read(file_, imageSize);
        file_.seekg(imageSize + 1, std::ios::cur);
    }


    std::vector<std::string> projectsList;

    int32_t projectCount;
    read(file_, projectCount);
    for(int32_t i = 0; i < projectCount; i++) {
        auto& str = projectsList.emplace_back();
        read(file_, str);
        decompressStr(str);
    }

    if(this->wooxyFileVersion >= 4) {
        int32_t deleteCount;
        read(file_, deleteCount);
        for(int32_t i = 0; i < deleteCount; i++) {
            int32_t projectIndex;
            read(file_, projectIndex);
            if(projectIndex >= projectCount || projectIndex < 0) {
                throw std::runtime_error("delete file doesn't reference valid projcet!");
            }
            auto& skn = this->deleteList.emplace_back();
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
        auto& skn = this->filesList.emplace_back();
        skn.project = projectsList[static_cast<size_t>(projectIndex)];
        read(file_, skn.fileGamePath);
        read(file_, skn.uncompresedSize);
        read(file_, skn.compressedSize);
        if(this->wooxyFileVersion != 1) {
            read(file_, skn.checksum);
        }
        skn.compressionMethod = skn.compressedSize == skn.uncompresedSize ? 0 : 2;
        decompressStr(skn.fileGamePath);
    }

    int32_t offset = (int32_t)file_.tellg();
    for(int32_t i = 0; i < fileCount; i++) {
        auto& skn = this->filesList[static_cast<size_t>(i)];
        skn.offset = offset;
        offset += skn.compressedSize;
        if(this->wooxyFileVersion <= 4) {
            offset -= 1;
        } else {
            offset -= 2;
        }
    }
}

void WxyExtract::read_oink() {
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
    file_.seekg(cLens.name + cLens.author + cLens.version + cLens.category + cLens.subCategory, std::ios::cur);

    int32_t previewContentCount;
    read(file_, previewContentCount);
    for(int32_t i = 0; i < previewContentCount; i++) {
        uint8_t type;
        read(file_, type);
        if(type == 0) {
            int32_t size = 0;
            read(file_, size);
            file_.seekg(size, std::ios::cur);
        }
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
            if(this->wooxyFileVersion > 6) {
                decryptStr(kvp.second);
            }
            kvp.first = xxhashStr(kvp.second);
        }

        std::sort(fileNames.begin(), fileNames.end(), [](auto const& l, auto const& r){
            return l.first < r.first;
        });

        for(int32_t f = 0; f < fileCount; f++) {
            auto& skn = this->filesList.emplace_back();
            read(file_, skn.checksum2);
            read(file_, skn.checksum);
            read(file_, skn.adler);
            read(file_, skn.uncompresedSize);
            read(file_, skn.compressedSize);
            read(file_, skn.compressionMethod);

            if(this->wooxyFileVersion > 6) {
                skn.adler = skn.adler
                        ^ static_cast<uint32_t>(skn.uncompresedSize)
                        ^ static_cast<uint32_t>(skn.compressedSize)
                        ^ 0x2Au;
            }

            skn.project = projectsList[static_cast<size_t>(c)];
            skn.fileGamePath = fileNames[static_cast<size_t>(f)].second;
        }
    }
    int32_t offset = (int32_t)file_.tellg();
    for(auto& skn: this->filesList) {
        skn.offset = offset;
        offset += skn.compressedSize;
    }
}

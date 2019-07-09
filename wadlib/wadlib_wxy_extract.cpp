#include "wadlib.h"
#include "zlib.h"

namespace fs = std::filesystem;

void wxy_extract(fspath const& dst, fspath const& src, updatefn update, int32_t const buffercap) {
    WxySkin wxy{};
    File file(src, L"rb");
    file.read(wxy);

    int64_t total = 0;
    int64_t done = 0;
    std::vector<uint8_t> uncompressedBuffer;
    std::vector<uint8_t> compressedBuffer;
    size_t maxUncompressed = 0;
    size_t maxCompressed = 0;

    for(auto const& entry: wxy.filesList) {
        if(static_cast<size_t>(entry.uncompresedSize) > maxUncompressed) {
            maxUncompressed = static_cast<size_t>(entry.uncompresedSize);
        }
        if(static_cast<size_t>(entry.compressedSize) > maxCompressed) {
            maxCompressed = static_cast<size_t>(entry.compressedSize);
        }
        total += entry.uncompresedSize;
    }

    auto const nameStr = dst.generic_string();
    if(update) {
        update(nameStr, done, total);
    }

    for(auto const& entry: wxy.filesList) {
        auto dstfile_path = dst / entry.fileGamePath;
        std::error_code errorc;
        fs::create_directories(dstfile_path.parent_path(), errorc);
        if(errorc) {
            throw File::Error(errorc.message().c_str());
        }
        file.seek_beg(entry.offset);
        uncompressedBuffer.resize(static_cast<size_t>(entry.uncompresedSize));
        compressedBuffer.resize(static_cast<size_t>(entry.compressedSize));

        if(entry.compressionMethod == WxySkin::methodNone) {
            file.read(uncompressedBuffer);
        } else if(entry.compressionMethod == WxySkin::methodZlib) {
            file.read(compressedBuffer);
            z_stream strm = {};
            if (inflateInit2_(
                        &strm,
                        15,
                        ZLIB_VERSION,
                        static_cast<int>(sizeof(z_stream))) != Z_OK) {
                throw File::Error("Failed to init uncompress stream!");
            }
            strm.next_in = &compressedBuffer[0];
            strm.avail_in = static_cast<unsigned int>(compressedBuffer.size());
            strm.next_out = &uncompressedBuffer[0];
            strm.avail_out = static_cast<unsigned int>(uncompressedBuffer.size());
            inflate(&strm, Z_FINISH);
            inflateEnd(&strm);
        } else if(entry.compressionMethod == WxySkin::methodDeflate) {
            file.read(compressedBuffer);
            z_stream strm = {};
            if (inflateInit2_(
                        &strm,
                        -15,
                        ZLIB_VERSION,
                        static_cast<int>(sizeof(z_stream))) != Z_OK) {
                throw File::Error("Failed to init uncompress stream!");
            }
            strm.next_in = &compressedBuffer[0];
            strm.avail_in = static_cast<unsigned int>(compressedBuffer.size());
            strm.next_out = &uncompressedBuffer[0];
            strm.avail_out = static_cast<unsigned int>(uncompressedBuffer.size());
            inflate(&strm, Z_FINISH);
            inflateEnd(&strm);
        } else {
            throw File::Error("Unknow compression method!");
        }
        File dstfile(dstfile_path, L"wb");
        dstfile.write(uncompressedBuffer);
        if(update) {
            done += entry.uncompresedSize;
            update(nameStr, done, total);
        }
    }
}

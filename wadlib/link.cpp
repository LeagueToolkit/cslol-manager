#include "link.h"
#include "file.hpp"
#include <array>
#include <vector>


void read(const File &file, Link &link)
{
    uint32_t magic;
    std::array<uint8_t, 16> guid;
    uint32_t dataFlags;
    file.read(magic);
    file.read(guid);
    file.read(dataFlags);
    file.seek_beg(76);

    uint16_t shellList;
    if(dataFlags & 0x01) {
        file.read(shellList);
        if(shellList) {
            file.seek_cur(shellList);
        }
    }

    auto const startInfo = file.tell();
    uint32_t locSize;
    uint32_t locHeaderSize;
    uint32_t locFlags;
    int32_t locOffVolumeInfo;
    int32_t locOffLocalPath;
    int32_t locOffNetShare;
    int32_t locOffCommonPath;
    int32_t locOffULocalPath;
    int32_t locOffUCommonPath;
    if(dataFlags & 0x02) {
        file.read(locSize);
        file.read(locHeaderSize);
        file.read(locFlags);
        file.read(locOffVolumeInfo);
        file.read(locOffLocalPath);
        file.read(locOffNetShare);
        file.read(locOffCommonPath);
        if(locHeaderSize > 28) {
            file.read(locOffULocalPath);
        }
        if(locHeaderSize > 32) {
            file.read(locOffUCommonPath);
        }

        file.seek_beg(startInfo + locOffLocalPath);
        auto localPath = file.read_zstring();

        file.seek_beg(startInfo + locOffCommonPath);
        auto commonPath = file.read_zstring();
        link.path = commonPath + localPath;
    }
}

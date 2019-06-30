#include "wad.h"
#include "file.hpp"
#include <vector>

void read(File const& file, WadEntry& wad) {
    file.read(wad.xxhash);
    file.read(wad.dataOffset);
    file.read(wad.sizeCompressed);
    file.read(wad.sizeUncompressed);
    file.read(wad.type);
    file.read(wad.isDuplicate);
    file.read(wad.pad);
    file.read(wad.sha256);
}

void write(File const& file, WadEntry const & wad) {
    file.write(wad.xxhash);
    file.write(wad.dataOffset);
    file.write(wad.sizeCompressed);
    file.write(wad.sizeUncompressed);
    file.write(wad.type);
    file.write(wad.isDuplicate);
    file.write(wad.pad);
    file.write(wad.sha256);
}

void read(File const& file, WadHeader& header) {
    file.read(header.version);
    if(header.version != std::array{'R', 'W', '\x03', '\x00'}) {
        throw File::Error("Invalid .wad file!");
    }
    file.read(header.signature);
    file.read(header.checksum);
    file.read(header.filecount);
}

void write(File const& file, WadHeader const & header) {
    file.write(header.version);
    file.write(header.signature);
    file.write(header.checksum);
    file.write(header.filecount);
}



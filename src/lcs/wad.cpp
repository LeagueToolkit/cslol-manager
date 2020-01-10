#include <filesystem>
#include <fstream>
#include "wad.hpp"

using namespace LCS;

Wad::Wad(const std::filesystem::path& path, const std::string& name)
    : path_(fs::absolute(path)), size_(fs::file_size(path_)), name_(name)  {

    file_.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    file_.open(path_, std::ios::binary);
    file_.read((char*)&header_, sizeof(header_));
    if (header_.version != std::array{'R', 'W', '\x3', '\x0'}) {
        throw std::ifstream::failure("Not a wad file");
    }
    dataBegin_ = header_.filecount * sizeof(Entry) + sizeof(header_);
    dataEnd_ = size_;
    if (dataBegin_ > dataEnd_) {
        throw std::ifstream::failure("Failed to read TOC");
    }
    entries_.resize(header_.filecount);
    file_.read((char*)entries_.data(), header_.filecount * sizeof(Entry));
    for(auto const& entry: entries_) {
        if (entry.dataOffset > dataEnd_ || entry.dataOffset < dataBegin_) {
            throw std::ifstream::failure("Entry not in data region!");
        }
        if (entry.dataOffset + entry.sizeCompressed > dataEnd_) {
            throw std::ifstream::failure("Entry outside of data!");
        }
    }
}

#ifndef LCS_WAD_HPP
#define LCS_WAD_HPP
#include "common.hpp"
#include "hashtable.hpp"
#include <array>
#include <fstream>

namespace LCS {
    struct Wad {
        struct Entry {
            enum Type : uint8_t {
                Uncompressed,
                ZlibCompressed,
                FileRedirection,
                ZStandardCompressed
            };

            uint64_t xxhash;
            uint32_t dataOffset;
            uint32_t sizeCompressed;
            uint32_t sizeUncompressed;
            uint8_t type;
            uint8_t isDuplicate;
            std::array<uint8_t, 2> pad;
            std::array<uint8_t, 8> sha256;
        };
        static_assert (sizeof(Entry) == 32);

        struct Header {
            std::array<char, 4> version;
            std::array<uint8_t, 256> signature;
            std::array<uint8_t, 8> checksum;
            uint32_t filecount;
        };
        static_assert (sizeof(Header) == 4 + 256 + 8 + 4);

        // Throws std::runtime_error
        Wad(fs::path const& path, std::string const& name);
        inline Wad(fs::path path) : Wad(path, path.filename().generic_string()) {}
        Wad(Wad const&) = delete;
        Wad(Wad&&) = default;
        Wad& operator=(Wad const&) = delete;
        Wad& operator=(Wad&&) = delete;


        inline auto const& header() const& noexcept {
            return header_;
        }

        inline auto const& entries() const& noexcept {
            return entries_;
        }

        inline auto const& name() const& noexcept {
            return name_;
        }

        inline auto const& path() const& noexcept {
            return path_;
        }

        inline auto& file() const& noexcept {
            return file_;
        }

        inline auto dataBegin() const noexcept {
            return dataBegin_;
        }

        inline auto dataEnd() const noexcept {
            return dataEnd_;
        }

        inline auto dataSize() const noexcept {
            return dataEnd_ - dataBegin_;
        }

        inline auto size() const noexcept {
            return size_;
        }

        void extract(fs::path const& dest, HashTable const& hashtable, Progress& progress) const;
    private:
        fs::path path_;
        size_t size_;
        std::string name_;
        Header header_;
        std::vector<Entry> entries_;
        mutable std::ifstream file_;
        size_t dataBegin_ = 0;
        size_t dataEnd_ = 0;
    };
}

#endif // LCS_WAD_HPP

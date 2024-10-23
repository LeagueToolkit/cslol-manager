#pragma once
#include <array>
#include <lol/common.hpp>
#include <lol/hash/xxh64.hpp>
#include <lol/io/bytes.hpp>
#include <lol/wad/entry.hpp>

namespace lol::wad {
    // mixed-endian uint24_t with no alignment requirements
    struct UInt24ME {
        constexpr UInt24ME() = default;
        constexpr UInt24ME(uint32_t x) : hi_((uint8_t)(x >> 16)), lo_((uint8_t)x), mi_((uint8_t)(x >> 8)) {}
        constexpr operator uint32_t() const { return ((uint32_t)hi_ << 16) | ((uint32_t)mi_ << 8) | (uint32_t)lo_; }

    private:
        uint8_t hi_{};
        uint8_t lo_{};
        uint8_t mi_{};
    };

    struct TOC {
        struct Version {
            std::array<char, 2> magic;
            std::uint8_t major;
            std::uint8_t minor;

            static constexpr auto latest() noexcept -> Version { return {'R', 'W', 3, 4}; }

            inline bool is_wad() const noexcept { return magic == std::array{'R', 'W'}; }

            bool operator==(Version const& other) const noexcept {
                return magic == other.magic && major == other.major && minor == other.minor;
            }
            bool operator!=(Version const& other) const noexcept {
                return !(*this == other);
            }
        };

        using Signature = std::array<std::uint8_t, 16>;

        struct HeaderV1 {
            Version version;
            static constexpr Signature signature = {};
            static constexpr std::array<std::uint8_t, 8> checksum = {};
            std::uint16_t desc_offset = 12;
            std::uint16_t desc_size = 24;
            std::uint32_t desc_count;
        };

        struct HeaderV2 {
            Version version;
            static constexpr Signature signature = {};
            std::array<std::uint8_t, 84> signature_unused;
            std::array<std::uint8_t, 8> checksum;
            std::uint16_t desc_offset = 104;
            std::uint16_t desc_size = 24;
            std::uint32_t desc_count;
        };

        struct HeaderV3 {
            Version version;
            Signature signature = {};
            std::array<std::uint8_t, 240> signature_unused;
            std::array<std::uint8_t, 8> checksum;
            static constexpr std::uint16_t desc_offset = 272;
            static constexpr std::uint16_t desc_size = 32;
            std::uint32_t desc_count;
        };

        struct EntryV1 {
            std::uint64_t name;
            std::uint32_t offset;
            std::uint32_t size;
            std::uint32_t size_decompressed;
            EntryType type;
            uint8_t pad[3];
            static constexpr std::uint8_t subchunk_count = {};
            static constexpr bool is_duplicate = {};
            static constexpr std::uint16_t subchunk_index = {};
            static constexpr std::uint64_t checksum = {};
        };

        struct EntryV2 {
            std::uint64_t name;
            std::uint32_t offset;
            std::uint32_t size;
            std::uint32_t size_decompressed;
            EntryType type : 4;
            std::uint8_t subchunk_count : 4;
            std::uint8_t is_duplicate;
            std::uint16_t subchunk_index;
            static constexpr std::uint64_t checksum = {};
        };

        struct EntryV3 {
            std::uint64_t name;
            std::uint32_t offset;
            std::uint32_t size;
            std::uint32_t size_decompressed;
            EntryType type : 4;
            std::uint8_t subchunk_count : 4;
            std::uint8_t is_duplicate;
            std::uint16_t subchunk_index;
            std::uint64_t checksum_old;
            static constexpr std::uint64_t checksum = {};
        };

        struct EntryV3_1 {
            std::uint64_t name;
            std::uint32_t offset;
            std::uint32_t size;
            std::uint32_t size_decompressed;
            EntryType type : 4;
            std::uint8_t subchunk_count : 4;
            std::uint8_t is_duplicate;
            std::uint16_t subchunk_index;
            std::uint64_t checksum;
        };

        struct EntryV3_4 {
            std::uint64_t name;
            std::uint32_t offset;
            std::uint32_t size;
            std::uint32_t size_decompressed;
            EntryType type : 4;
            std::uint8_t subchunk_count : 4;
            UInt24ME subchunk_index;
            std::uint64_t checksum;
            static constexpr bool is_duplicate = {};
        };

        struct Entry {
            hash::Xxh64 name;
            EntryLoc loc;
        };

        using latest_header_t = HeaderV3;
        using latest_entry_t = EntryV3_4;

        Version version;
        Signature signature;
        std::vector<Entry> entries;

        auto read(io::Bytes src) noexcept -> char const*;
    };
}

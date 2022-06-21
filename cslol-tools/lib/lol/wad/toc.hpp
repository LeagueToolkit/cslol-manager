#pragma once
#include <array>
#include <lol/common.hpp>
#include <lol/hash/xxh64.hpp>
#include <lol/io/bytes.hpp>
#include <lol/wad/entry.hpp>

namespace lol::wad {
    struct TOC {
        struct Version {
            std::array<char, 2> magic;
            std::uint8_t major;
            std::uint8_t minor;

            static constexpr auto latest() noexcept -> Version { return {'R', 'W', 3, 3}; }

            inline bool is_wad() const noexcept { return magic == std::array{'R', 'W'}; }

            constexpr bool operator==(Version const&) const noexcept = default;
            constexpr auto operator<=>(Version const&) const noexcept = default;
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

        struct Entry {
            hash::Xxh64 name;
            EntryLoc loc;
        };

        using latest_header_t = HeaderV3;
        using latest_entry_t = EntryV3_1;

        Version version;
        Signature signature;
        std::vector<Entry> entries;

        auto read(io::Bytes src) noexcept -> char const*;
    };
}

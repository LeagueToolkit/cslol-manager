#pragma once
#include <lol/common.hpp>
#include <lol/hash/dict.hpp>
#include <lol/hash/xxh64.hpp>
#include <lol/io/bytes.hpp>
#include <memory>
#include <optional>

namespace lol::wad {
    enum class EntryType : unsigned char {
        Raw = 0,
        Link = 1,
        Gzip = 2,
        Zstd = 3,
        ZstdMulti = 4,
    };

    struct EntryLoc {
        EntryType type;
        std::uint8_t subchunk_count = {};
        std::uint32_t subchunk_index = {};
        std::uint64_t offset;
        std::uint64_t size;
        std::uint64_t size_decompressed;
        std::uint64_t checksum;
    };

    struct EntryData {
        EntryData() noexcept = default;

        EntryData(EntryData const& other) noexcept : impl_(other.impl_) {}

        EntryData(EntryData&& other) noexcept : impl_(std::exchange(other.impl_, Impl::empty())) {}

        EntryData& operator=(EntryData other) noexcept {
            std::swap(other.impl_, impl_);
            return *this;
        }

        static auto from_raw(io::Bytes data, std::uint64_t checksum) -> EntryData;

        static auto from_link(io::Bytes data, std::uint64_t checksum) -> EntryData;

        static auto from_gzip(io::Bytes data, std::size_t decompressed, std::uint64_t checksum) -> EntryData;

        static auto from_zstd(io::Bytes data, std::size_t decompressed, std::uint64_t checksum) -> EntryData;

        static auto from_zstd_multi(io::Bytes data,
                                    std::size_t decompressed,
                                    std::uint64_t checksum,
                                    std::uint8_t subchunk_count,
                                    std::uint32_t subchunk_index) -> EntryData;

        static auto from_file(fs::path const& path) -> EntryData;

        static auto from_loc(io::Bytes src, EntryLoc const& loc) -> EntryData;

        auto into_compressed() const -> EntryData;

        auto into_decompressed() const -> EntryData;

        auto into_optimal() const -> EntryData;

        auto checksum() const -> std::uint64_t;

        auto extension() const -> std::string_view;

        auto type() const noexcept -> EntryType { return impl_->type; }

        auto subchunk_count() const noexcept -> std::uint8_t { return impl_->subchunk_count; }

        auto subchunk_index() const noexcept -> std::uint32_t { return impl_->subchunk_index; }

        auto bytes() const noexcept -> io::Bytes { return impl_->bytes; }

        auto bytes_data() const noexcept -> char const* { return impl_->bytes.data(); }

        auto bytes_size() const noexcept -> std::size_t { return impl_->bytes.size(); }

        auto size_decompressed() const noexcept -> std::size_t { return impl_->size_decompressed; }

        auto mark_dirty() -> void { impl_->checksum = 0; }

        auto mark_optimal(bool value) -> void { impl_->is_optimal = value; }

        auto touch() -> void { impl_->bytes.reserve(0, 0); }

        auto write_to_file(fs::path const& path) const -> void;

        auto write_to_dir(hash::Xxh64 name, fs::path const& dir, hash::Dict const* dict) const -> void;

    private:
        struct Impl {
            EntryType type = EntryType::Raw;
            bool is_optimal = false;
            std::uint8_t subchunk_count = {};
            std::uint16_t subchunk_index = {};
            std::optional<std::string_view> extension = {};
            io::Bytes bytes = {};
            std::size_t size_decompressed = 0;
            mutable std::uint64_t checksum = 0;
            mutable std::weak_ptr<Impl> compressed = {};
            mutable std::weak_ptr<Impl> decompressed = {};

            static auto empty() noexcept -> std::shared_ptr<Impl> const&;
        };

        std::shared_ptr<Impl> impl_ = Impl::empty();

        explicit EntryData(std::shared_ptr<Impl> impl) noexcept : impl_(std::move(impl)) {}
    };
}

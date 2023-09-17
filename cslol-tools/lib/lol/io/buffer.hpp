#pragma once
#include <lol/common.hpp>
#include <system_error>

namespace lol::io {
    using std::dynamic_extent;

    inline constexpr std::size_t KiB = 1024;
    inline constexpr std::size_t MiB = KiB * 1024;
    inline constexpr std::size_t GiB = MiB * 1024;

    struct Bytes;
    struct Buffer {
        auto data() const noexcept -> char const* { return data_; }

        auto size() const noexcept -> std::size_t { return size_; }

        auto copy(std::size_t pos, std::size_t count) const -> Bytes;

        auto readsome(std::size_t pos, void* dst, std::size_t dst_count) const noexcept -> std::size_t;

        auto readsome_decompress_zstd(std::size_t pos,
                                      std::size_t count,
                                      void* dst,
                                      std::size_t dst_count) const noexcept -> std::size_t;

        auto read(std::size_t pos, void* dst, std::size_t dst_count) const -> std::size_t;

        auto reserve(std::size_t pos, std::size_t count) -> char*;

        auto resize(std::size_t count) -> char*;

        auto writesome(std::size_t pos, void const* src, std::size_t src_count) noexcept -> std::size_t;

        auto write(std::size_t pos, void const* src, std::size_t src_count) -> std::size_t;

        auto write_decompress_defl(std::size_t pos, std::size_t count, void const* src, std::size_t src_count) -> void;

        auto write_decompress_zlib(std::size_t pos, std::size_t count, void const* src, std::size_t src_count) -> void;

        auto write_decompress_gzip(std::size_t pos, std::size_t count, void const* src, std::size_t src_count) -> void;

        auto write_decompress_zstd(std::size_t pos, std::size_t count, void const* src, std::size_t src_count) -> void;

        auto write_decompress_zstd_hack(std::size_t pos, std::size_t count, void const* src, std::size_t src_count)
            -> void;

        auto write_compress_defl(std::size_t pos, void const* src, std::size_t src_count, int level = 6) -> std::size_t;

        auto write_compress_zlib(std::size_t pos, void const* src, std::size_t src_count, int level = 6) -> std::size_t;

        auto write_compress_gzip(std::size_t pos, void const* src, std::size_t src_count, int level = 6) -> std::size_t;

        auto write_compress_zstd(std::size_t pos, void const* src, std::size_t src_count, int level = 0) -> std::size_t;

    protected:
        virtual ~Buffer() noexcept = 0;

        virtual auto impl_copy(std::size_t pos, std::size_t count, Bytes& out) const noexcept -> std::error_code = 0;

        virtual auto impl_reserve(std::size_t count) noexcept -> std::error_code = 0;

        char* data_;
        std::size_t size_;
    };
}

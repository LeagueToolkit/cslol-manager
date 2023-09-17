#pragma once
#include <lol/common.hpp>
#include <lol/fs.hpp>
#include <lol/io/buffer.hpp>

namespace lol::io {
    struct Bytes final : Buffer {
    public:
        Bytes() noexcept = default;

        Bytes(Bytes const& other) noexcept {
            data_ = other.data_;
            size_ = other.size_;
            if ((impl_ = other.impl_)) {
                ++(impl_->ref_count);
            }
        }

        Bytes(Bytes&& other) noexcept {
            std::swap(impl_, other.impl_);
            std::swap(data_, other.data_);
            std::swap(size_, other.size_);
        }

        Bytes& operator=(Bytes other) noexcept {
            std::swap(impl_, other.impl_);
            std::swap(data_, other.data_);
            std::swap(size_, other.size_);
            return *this;
        }

        ~Bytes() noexcept {
            auto data = std::exchange(data_, {});
            auto size = std::exchange(size_, {});
            auto impl = std::exchange(impl_, {});
            if (impl && (impl->ref_count)-- == 0) {
                delete impl;
                (void)data;
                (void)size;
            }
        }

        static auto from_capacity(std::size_t cap) -> Bytes;

        static auto from_static(std::span<char const> data) noexcept -> Bytes;

        static auto from_vector(std::vector<char> vec) noexcept -> Bytes;

        static auto from_file(fs::path const& path) -> Bytes;

        auto copy_decompress_defl(std::size_t size_decompressed) const -> Bytes {
            auto result = Bytes();
            result.write_decompress_defl(0, size_decompressed, data_, size_);
            return result;
        }

        auto copy_decompress_zlib(std::size_t size_decompressed) const -> Bytes {
            auto result = Bytes();
            result.write_decompress_zlib(0, size_decompressed, data_, size_);
            return result;
        }

        auto copy_decompress_gzip(std::size_t size_decompressed) const -> Bytes {
            auto result = Bytes();
            result.write_decompress_gzip(0, size_decompressed, data_, size_);
            return result;
        }

        auto copy_decompress_zstd(std::size_t size_decompressed) const -> Bytes {
            auto result = Bytes();
            result.write_decompress_zstd(0, size_decompressed, data_, size_);
            return result;
        }

        auto copy_decompress_zstd_hack(std::size_t size_decompressed) const -> Bytes {
            auto result = Bytes();
            result.write_decompress_zstd_hack(0, size_decompressed, data_, size_);
            return result;
        }

        auto copy_compress_defl(int level = 6) const -> Bytes {
            auto result = Bytes();
            result.write_compress_defl(0, data_, size_, level);
            return result;
        }

        auto copy_compress_zlib(int level = 6) const -> Bytes {
            auto result = Bytes();
            result.write_compress_zlib(0, data_, size_, level);
            return result;
        }

        auto copy_compress_gzip(int level = 6) const -> Bytes {
            auto result = Bytes();
            result.write_compress_gzip(0, data_, size_, level);
            return result;
        }

        auto copy_compress_zstd(int level = 0) const -> Bytes {
            auto result = Bytes();
            result.write_compress_zstd(0, data_, size_, level);
            return result;
        }

        operator std::vector<char>() const&;

        operator std::vector<char>() &&;

    protected:
        auto impl_copy(std::size_t pos, std::size_t count, Bytes& out) const noexcept -> std::error_code override;

        auto impl_reserve(std::size_t count) noexcept -> std::error_code override;

    private:
        struct Impl final {
            std::size_t ref_count = 0;
            std::vector<char> vec;
            struct MMap {
                ~MMap() noexcept;
                std::intptr_t file;
                void* data;
                std::size_t size;
            } mmap = {};
        };
        Impl* impl_ = {};
    };
}

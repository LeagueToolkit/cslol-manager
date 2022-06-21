#pragma once
#include <lol/common.hpp>
#include <lol/fs.hpp>
#include <lol/io/buffer.hpp>

namespace lol::io {
    struct File final : Buffer {
        File() noexcept = default;

        File(File const& other) noexcept = delete;

        File(File&& other) noexcept {
            std::swap(file_, other.file_);
            std::swap(data_, other.data_);
            std::swap(size_, other.size_);
            std::swap(disk_size_, other.disk_size_);
            std::swap(mapped_size_, other.mapped_size_);
        }

        File& operator=(File other) noexcept {
            std::swap(file_, other.file_);
            std::swap(data_, other.data_);
            std::swap(size_, other.size_);
            std::swap(disk_size_, other.disk_size_);
            std::swap(mapped_size_, other.mapped_size_);
            return *this;
        }

        ~File() noexcept;

        static auto create(fs::path const& path) -> File;

    protected:
        auto impl_copy(std::size_t pos, std::size_t count, Bytes& out) const noexcept -> std::error_code override;

        auto impl_reserve(std::size_t count) noexcept -> std::error_code override;

    private:
        std::intptr_t file_ = {};
        std::size_t disk_size_ = {};
        std::size_t mapped_size_ = {};
    };
}

#pragma once
#include <filesystem>
#include <system_error>

namespace lol::io::sys {
    template <typename T>
    struct [[nodiscard]] Result {
        std::error_code error = {};
        T value = {};
    };

    extern auto file_open(std::filesystem::path const& path, bool write) noexcept -> Result<std::intptr_t>;

    extern auto file_size(std::intptr_t file) noexcept -> Result<std::size_t>;

    extern auto file_resize(std::intptr_t file, std::size_t count) noexcept -> Result<std::size_t>;

    extern auto file_close(std::intptr_t file) noexcept -> void;

    extern auto file_mmap(std::intptr_t file, std::size_t count, bool write) noexcept -> Result<void*>;

    extern auto file_munmap(void* data, std::size_t count) noexcept -> void;
}

#include <lol/io/sys.hpp>

using namespace lol;
using namespace lol::io;

#define handle_ok(handle) ((std::intptr_t)handle != 0 && (std::intptr_t)handle != -1)

#ifdef _WIN32
#    ifndef WIN32_LEAN_AND_MEAN
#        define WIN32_LEAN_AND_MEAN
#    endif
#    ifndef NOMINMAX
#        define NOMINMAX
#    endif
#    include <windows.h>
#    define last_error() std::error_code((int)GetLastError(), std::system_category())

auto sys::file_open(std::filesystem::path const& path, bool write) noexcept -> Result<std::intptr_t> {
    if (write) {
        std::error_code ec = {};
        std::filesystem::create_directories(path.parent_path(), ec);
        if (ec) [[unlikely]] {
            return {ec};
        }
    }
    auto const file = ::CreateFile(path.string().c_str(),
                                   write ? GENERIC_READ | GENERIC_WRITE : GENERIC_READ,
                                   write ? 0 : FILE_SHARE_READ,
                                   0,
                                   write ? OPEN_ALWAYS : OPEN_EXISTING,
                                   FILE_ATTRIBUTE_NORMAL,
                                   0);
    if (!handle_ok(file)) [[unlikely]] {
        return {last_error()};
    }
    return {{}, (std::intptr_t)file};
}

auto sys::file_size(std::intptr_t file) noexcept -> Result<std::size_t> {
    LARGE_INTEGER result = {};
    auto const status = ::GetFileSizeEx((HANDLE)file, &result);
    if (status == FALSE) [[unlikely]] {
        return {last_error()};
    }
    return {{}, (std::size_t)result.QuadPart};
}

auto sys::file_resize(std::intptr_t file, std::size_t count) noexcept -> Result<std::size_t> {
    FILE_END_OF_FILE_INFO info = {.EndOfFile = {.QuadPart = (LONGLONG)count}};
    auto const status = ::SetFileInformationByHandle((HANDLE)file, FileEndOfFileInfo, &info, sizeof(info));
    if (status == FALSE) [[unlikely]] {
        return {last_error()};
    }
    return {{}, count};
}

auto sys::file_close(std::intptr_t file) noexcept -> void {
    if (file) {
        CloseHandle((HANDLE)file);
    }
}

auto sys::file_mmap(std::intptr_t file, std::size_t count, bool write) noexcept -> Result<void*> {
    if (count == 0) {
        return {{}, {}};
    }
    auto mapping = ::CreateFileMapping((HANDLE)file, 0, write ? PAGE_READWRITE : PAGE_READONLY, count >> 32, count, 0);
    if (!handle_ok(mapping)) [[unlikely]] {
        return {last_error()};
    }
    auto data = ::MapViewOfFile(mapping, write ? FILE_MAP_READ | FILE_MAP_WRITE : FILE_MAP_READ, 0, 0, count);
    if (!handle_ok(data)) [[unlikely]] {
        auto ec = last_error();
        CloseHandle(mapping);
        return {ec, {}};
    }
    CloseHandle(mapping);
    return {{}, data};
}

auto sys::file_munmap(void* data, std::size_t count) noexcept -> void {
    if (data) {
        ::UnmapViewOfFile(data);
    }
}
#endif

#ifndef _WIN32
#    include <fcntl.h>
#    include <sys/mman.h>
#    include <sys/param.h>
#    include <sys/stat.h>
#    include <unistd.h>
#    include <sys/resource.h>
#    define last_error() std::error_code((int)errno, std::system_category())

static struct ResourceLimit {
    public: ResourceLimit() {
        rlimit resourceLimit;
        getrlimit(RLIMIT_NOFILE, &resourceLimit);
        resourceLimit.rlim_cur = resourceLimit.rlim_max == RLIM_INFINITY ? 65000 : resourceLimit.rlim_max - 1;
        setrlimit(RLIMIT_NOFILE, &resourceLimit);
    }
} resourceLimit_ = {};

auto sys::file_open(std::filesystem::path const& path, bool write) noexcept -> Result<std::intptr_t> {
    if (write) {
        std::error_code ec = {};
        std::filesystem::create_directories(path.parent_path(), ec);
        if (ec) [[unlikely]] {
            return {ec};
        }
    }
    int fd = write ? ::open(path.string().c_str(), O_RDWR | O_CREAT, 0644) : ::open(path.string().c_str(), O_RDONLY);
    if (!handle_ok(fd)) [[unlikely]] {
        return {last_error()};
    }
    return {{}, (std::intptr_t)fd};
}

auto sys::file_size(std::intptr_t file) noexcept -> Result<std::size_t> {
    struct ::stat result = {};
    auto const status = ::fstat((int)file, &result);
    if (status == -1) [[unlikely]] {
        return {last_error()};
    }
    return {{}, (std::size_t)result.st_size};
}

auto sys::file_resize(std::intptr_t file, std::size_t count) noexcept -> Result<std::size_t> {
    auto const status = ::ftruncate((int)file, (off_t)count);
    if (status == -1) [[unlikely]] {
        return {last_error()};
    }
    return {{}, count};
}

auto sys::file_close(std::intptr_t file) noexcept -> void {
    if (file) {
        ::close((int)file);
    }
}

auto sys::file_mmap(std::intptr_t file, std::size_t count, bool write) noexcept -> Result<void*> {
    if (count == 0) {
        return {{}, {}};
    }
    void* data = ::mmap(0, count, write ? PROT_READ | PROT_WRITE : PROT_READ, MAP_SHARED, (int)file, 0);
    if (!handle_ok(data)) [[unlikely]] {
        return {last_error()};
    }
    return {{}, data};
}

auto sys::file_munmap(void* data, std::size_t count) noexcept -> void {
    if (data) {
        ::munmap(data, count);
    }
}
#endif

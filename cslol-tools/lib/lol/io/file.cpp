#include <lol/error.hpp>
#include <lol/io/bytes.hpp>
#include <lol/io/file.hpp>
#include <lol/io/sys.hpp>

using namespace lol;
using namespace lol::io;

File::~File() noexcept {
    auto const file = std::exchange(file_, {});
    auto const data = std::exchange(data_, {});
    auto const size = std::exchange(size_, {});
    auto const disk_size = std::exchange(disk_size_, {});
    auto const mapped_size = std::exchange(mapped_size_, {});
    if (data) {
        sys::file_munmap(data, mapped_size);
    }
    if (file) {
        if (size != disk_size) {
            (void)sys::file_resize(file, size);
        }
        sys::file_close(file);
    }
}

auto io::File::create(fs::path const& path) -> File {
    lol_trace_func(lol_trace_var("{}", path));

    auto result = File{};

    auto const file = sys::file_open(path, true);
    lol_throw_if(file.error);
    result.file_ = file.value;

    auto const file_size = sys::file_size(file.value);
    lol_throw_if(file_size.error);
    result.size_ = file_size.value;
    result.disk_size_ = file_size.value;
    result.mapped_size_ = file_size.value;

    auto const mmap_data = sys::file_mmap(file.value, file_size.value, true);
    lol_throw_if(mmap_data.error);
    result.data_ = (char*)mmap_data.value;

    return result;
}

auto io::File::impl_reserve(std::size_t count) noexcept -> std::error_code {
    if (count > mapped_size_) {
        if (count > 64 * GiB) [[unlikely]] {
            return std::make_error_code(std::errc::value_too_large);
        }
        if (count > disk_size_) {
            if (count > GiB) {
                count = ((count + (GiB - 1)) / GiB) * GiB;
            } else {
                count = ((count + (MiB - 1)) / MiB) * MiB;
            }
            if (auto error = sys::file_resize(file_, count).error) [[unlikely]] {
                return error;
            } else {
                disk_size_ = count;
            }
        }
        if (auto [error, data] = sys::file_mmap(file_, disk_size_, true); error) [[unlikely]] {
            return error;
        } else {
            sys::file_munmap(data_, mapped_size_);
            data_ = (char*)data;
            mapped_size_ = disk_size_;
        }
    }
    return {};
}

auto io::File::impl_copy(std::size_t pos, std::size_t count, Bytes& out) const noexcept -> std::error_code {
    try {
        out = Bytes::from_vector(std::vector<char>(data_ + pos, data_ + pos + count));
        return {};
    } catch (std::system_error const& error) {
        return error.code();
    }
}

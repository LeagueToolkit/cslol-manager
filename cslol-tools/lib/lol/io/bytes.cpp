#include <lol/error.hpp>
#include <lol/io/bytes.hpp>
#include <lol/io/sys.hpp>

using namespace lol;
using namespace lol::io;

Bytes::Impl::MMap::~MMap() noexcept {
    sys::file_munmap(data, size);
    sys::file_close(file);
}

auto Bytes::from_capacity(std::size_t count) -> Bytes {
    lol_trace_func(lol_trace_var("{:#x}", count));
    auto result = Bytes();
    result.impl_ = new Impl{};
    result.impl_->vec.resize(count);
    return result;
}

auto Bytes::from_static(std::span<char const> data) noexcept -> Bytes {
    auto result = Bytes{};
    result.data_ = const_cast<char*>(data.data());
    result.size_ = data.size();
    return result;
}

auto Bytes::from_vector(std::vector<char> vec) noexcept -> Bytes {
    try {
        vec.shrink_to_fit();
    } catch (...) {
        // NOTE: this is fine
    }
    auto result = Bytes();
    result.impl_ = new Impl{};
    result.impl_->vec = std::move(vec);
    result.data_ = result.impl_->vec.data();
    result.size_ = result.impl_->vec.size();
    return result;
}

auto Bytes::from_file(fs::path const& path) -> Bytes {
    lol_trace_func(lol_trace_var("{}", path));
    auto result = Bytes();
    result.impl_ = new Impl{};

    auto const file = sys::file_open(path, false);
    lol_throw_if(file.error);
    result.impl_->mmap.file = file.value;

    auto const file_size = sys::file_size(file.value);
    lol_throw_if(file_size.error);
    result.impl_->mmap.size = file_size.value;

    auto const mmap_data = sys::file_mmap(file.value, file_size.value, false);
    lol_throw_if(mmap_data.error);
    result.impl_->mmap.data = mmap_data.value;
    result.data_ = (char*)mmap_data.value;
    result.size_ = file_size.value;

    return result;
}

auto Bytes::impl_copy(std::size_t pos, std::size_t count, Bytes& out) const noexcept -> std::error_code {
    if (count == 0) return {};
    out.data_ = pos + data_;
    out.size_ = count;
    if ((out.impl_ = impl_)) {
        ++(impl_->ref_count);
    }
    return {};
}

auto Bytes::impl_reserve(std::size_t count) noexcept -> std::error_code {
    if (count > 64 * GiB) [[unlikely]] {
        return std::make_error_code(std::errc::value_too_large);
    }
    try {
        if (!impl_ || impl_->ref_count || impl_->vec.data() != data_) {
            auto result = Bytes();
            result.impl_ = new Impl{};
            result.impl_->vec.resize(std::max(count, size_));
            result.data_ = result.impl_->vec.data();
            result.size_ = size_;
            std::memcpy(result.data_, data_, size_);
            std::swap(result.data_, data_);
            std::swap(result.size_, size_);
            std::swap(result.impl_, impl_);
            return {};
        }
        if (auto& vec = impl_->vec; count > vec.size()) {
            vec.resize(std::max(count, vec.capacity()));
            data_ = vec.data();
        }
        return {};
    } catch (std::system_error const& error) {
        return error.code();
    }
}

Bytes::operator std::vector<char>() const& { return std::vector<char>(data_, data_ + size_); }

Bytes::operator std::vector<char>() && {
    auto result = std::vector<char>();
    if (!impl_ || impl_->ref_count || impl_->vec.data() != data_) {
        result = std::vector<char>(data_, data_ + size_);
    } else {
        impl_->vec.resize(size_);
        result = std::move(impl_->vec);
    }
    data_ = {};
    size_ = {};
    delete std::exchange(impl_, {});
    return result;
}

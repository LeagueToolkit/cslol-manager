#include <libdeflate.h>
#include <zstd.h>

#include <lol/error.hpp>
#include <lol/io/buffer.hpp>
#include <lol/io/bytes.hpp>

using namespace lol;
using namespace lol::io;

static std::size_t find_zstd_magic(std::span<char const> src) {
    static constexpr char const zstd_magic[4] = {0x28, (char)0xB5, 0x2F, (char)0xFD};
    auto magic = std::search(src.data(), src.data() + src.size(), zstd_magic, zstd_magic + sizeof(zstd_magic));
    if (magic == src.data() + src.size()) {
        return 0;
    }
    return (std::size_t)(magic - src.data());
}

Buffer::~Buffer() noexcept = default;

auto Buffer::copy(std::size_t pos, std::size_t count) const -> Bytes {
    lol_trace_func(lol_trace_var("{:#x}", size_), lol_trace_var("{:#x}", pos), lol_trace_var("{:#x}", count));
    lol_throw_if(pos + count < pos);
    lol_throw_if(size_ < pos);
    lol_throw_if(size_ - pos < count);
    auto result = Bytes{};
    lol_throw_if(impl_copy(pos, count, result));
    return result;
}

auto Buffer::readsome_decompress_zstd(std::size_t pos,
                                      std::size_t count,
                                      void* dst,
                                      std::size_t dst_count) const noexcept -> std::size_t {
    [[unlikely]] if (pos > size_ || size_ - pos < count) return 0;

    auto frame_start = find_zstd_magic({data_ + pos, count});
    if (frame_start >= dst_count) {
        std::memcpy(dst, data_ + pos, dst_count);
        return dst_count;
    }
    if (frame_start) {
        std::memcpy(dst, data_ + pos, frame_start);
        pos += frame_start;
        count -= frame_start;
        dst = (char*)dst + frame_start;
        dst_count -= frame_start;
    }

    thread_local auto ctx = std::shared_ptr<ZSTD_DStream>(ZSTD_createDStream(), ZSTD_freeDStream);
    [[unlikely]] if (!ctx.get()) return 0;
    [[unlikely]] if (ZSTD_isError(ZSTD_initDStream(ctx.get()))) return 0;
    [[unlikely]] if (pos > size_ || size_ - pos < count) return 0;
    auto input = ZSTD_inBuffer{data_ + pos, count, 0};
    auto output = ZSTD_outBuffer{dst, dst_count, 0};
    ZSTD_decompressStream(ctx.get(), &output, &input);
    return frame_start + output.pos;
}

auto Buffer::readsome(std::size_t pos, void* dst, std::size_t count) const noexcept -> std::size_t {
    pos = std::min(size_, pos);
    count = std::min(count, size_ - pos);
    std::memcpy(dst, data_ + pos, count);
    return count;
}

auto Buffer::read(std::size_t pos, void* dst, std::size_t count) const -> std::size_t {
    lol_trace_func(lol_trace_var("{:#x}", size_),
                   lol_trace_var("{:#x}", pos),
                   lol_trace_var("{:p}", dst),
                   lol_trace_var("{:#x}", count));
    if (count == 0) return {};
    lol_throw_if(pos + count < pos);
    lol_throw_if(size_ < pos);
    lol_throw_if(size_ - pos < count);
    std::memcpy(dst, data_, count);
    return count;
}

auto Buffer::reserve(std::size_t pos, std::size_t count) -> char* {
    lol_trace_func(lol_trace_var("{:#x}", size_), lol_trace_var("{:#x}", pos), lol_trace_var("{:#x}", count));
    auto const endpos = pos + count;
    lol_throw_if(endpos < pos);
    lol_throw_if(impl_reserve(endpos));
    return data_;
}

auto Buffer::resize(std::size_t count) -> char* {
    lol_trace_func(lol_trace_var("{:#x}", size_), lol_trace_var("{:#x}", count));
    lol_throw_if(impl_reserve(count));
    size_ = count;
    return data_;
}

auto Buffer::writesome(std::size_t pos, void const* src, std::size_t src_count) noexcept -> std::size_t {
    lol_trace_func(lol_trace_var("{:#x}", size_),
                   lol_trace_var("{:#x}", pos),
                   lol_trace_var("{:p}", src),
                   lol_trace_var("{:#x}", src_count));
    if (src_count == 0) return {};
    auto const endpos = pos + src_count;
    [[unlikely]] if (endpos < pos) return {};
    [[unlikely]] if (auto error = impl_reserve(endpos)) return {};
    std::memcpy(data_ + pos, src, src_count);
    size_ = std::max(size_, endpos);
    return src_count;
}

auto Buffer::write(std::size_t pos, void const* src, std::size_t src_count) -> std::size_t {
    lol_trace_func(lol_trace_var("{:#x}", size_),
                   lol_trace_var("{:#x}", pos),
                   lol_trace_var("{:p}", src),
                   lol_trace_var("{:#x}", src_count));
    if (src_count == 0) return {};
    auto const endpos = pos + src_count;
    lol_throw_if(endpos < pos);
    lol_throw_if(impl_reserve(endpos));
    std::memcpy(data_ + pos, src, src_count);
    size_ = std::max(size_, endpos);
    return src_count;
}

auto Buffer::write_decompress_defl(std::size_t pos, std::size_t count, void const* src, std::size_t src_count) -> void {
    lol_trace_func(lol_trace_var("{:#x}", size_),
                   lol_trace_var("{:#x}", pos),
                   lol_trace_var("{:#x}", count),
                   lol_trace_var("{:p}", src),
                   lol_trace_var("{:#x}", src_count));
    auto ctx = std::shared_ptr<libdeflate_decompressor>(libdeflate_alloc_decompressor(), libdeflate_free_decompressor);
    auto const endpos = pos + count;
    lol_throw_if(endpos < pos);
    lol_throw_if(impl_reserve(endpos));
    auto result = libdeflate_deflate_decompress(ctx.get(), src, src_count, data_ + pos, count, nullptr);
    lol_throw_if(result != LIBDEFLATE_SUCCESS);
    size_ = std::max(size_, endpos);
}

auto Buffer::write_decompress_zlib(std::size_t pos, std::size_t count, void const* src, std::size_t src_count) -> void {
    lol_trace_func(lol_trace_var("{:#x}", size_),
                   lol_trace_var("{:#x}", pos),
                   lol_trace_var("{:#x}", count),
                   lol_trace_var("{:p}", src),
                   lol_trace_var("{:#x}", src_count));
    auto ctx = std::shared_ptr<libdeflate_decompressor>(libdeflate_alloc_decompressor(), libdeflate_free_decompressor);
    auto const endpos = pos + count;
    lol_throw_if(endpos < pos);
    lol_throw_if(impl_reserve(endpos));
    auto result = libdeflate_zlib_decompress(ctx.get(), src, src_count, data_ + pos, count, nullptr);
    lol_throw_if(result != LIBDEFLATE_SUCCESS);
    size_ = std::max(size_, endpos);
}

auto Buffer::write_decompress_gzip(std::size_t pos, std::size_t count, void const* src, std::size_t src_count) -> void {
    lol_trace_func(lol_trace_var("{:#x}", size_),
                   lol_trace_var("{:#x}", pos),
                   lol_trace_var("{:#x}", count),
                   lol_trace_var("{:p}", src),
                   lol_trace_var("{:#x}", src_count));
    auto ctx = std::shared_ptr<libdeflate_decompressor>(libdeflate_alloc_decompressor(), libdeflate_free_decompressor);
    auto const endpos = pos + count;
    lol_throw_if(endpos < pos);
    lol_throw_if(impl_reserve(endpos));
    auto result = libdeflate_gzip_decompress(ctx.get(), src, src_count, data_ + pos, count, nullptr);
    lol_throw_if(result != LIBDEFLATE_SUCCESS);
    size_ = std::max(size_, endpos);
}

auto Buffer::write_decompress_zstd(std::size_t pos, std::size_t count, void const* src, std::size_t src_count) -> void {
    lol_trace_func(lol_trace_var("{:#x}", size_),
                   lol_trace_var("{:#x}", pos),
                   lol_trace_var("{:#x}", count),
                   lol_trace_var("{:p}", src),
                   lol_trace_var("{:#x}", src_count));
    auto const maxendpos = pos + count;
    lol_throw_if(maxendpos < pos);
    lol_throw_if(impl_reserve(maxendpos));
    auto const result = ZSTD_decompress(data_ + pos, count, src, src_count);
    lol_throw_if_msg(ZSTD_isError(result), "{}", ZSTD_getErrorName(result));
    lol_throw_if(result != count);
    size_ = std::max(size_, pos + result);
}

auto Buffer::write_decompress_zstd_hack(std::size_t pos, std::size_t count, void const* src, std::size_t src_count)
    -> void {
    lol_trace_func(lol_trace_var("{:#x}", size_),
                   lol_trace_var("{:#x}", pos),
                   lol_trace_var("{:#x}", count),
                   lol_trace_var("{:p}", src),
                   lol_trace_var("{:#x}", src_count));
    auto const maxendpos = pos + count;
    lol_throw_if(maxendpos < pos);
    lol_throw_if(impl_reserve(maxendpos));
    auto frame_start = find_zstd_magic({(char const*)src, src_count});
    if (frame_start) {
        lol_throw_if(frame_start > count);
        this->write(pos, src, frame_start);
        pos += frame_start;
        count -= frame_start;
        src = (char const*)src + frame_start;
        src_count -= frame_start;
    }
    if (count == 0 && src_count == 0) return;
    write_decompress_zstd(pos, count, src, src_count);
}

auto Buffer::write_compress_defl(std::size_t pos, void const* src, std::size_t src_count, int level) -> std::size_t {
    lol_trace_func(lol_trace_var("{:#x}", size_),
                   lol_trace_var("{:#x}", pos),
                   lol_trace_var("{:p}", src),
                   lol_trace_var("{:#x}", src_count),
                   lol_trace_var("{:}", level));
    if (level < 0 || level > 12) level = 6;
    auto ctx = std::shared_ptr<libdeflate_compressor>(libdeflate_alloc_compressor(level), libdeflate_free_compressor);
    auto const bound = libdeflate_deflate_compress_bound(ctx.get(), src_count);
    auto const maxendpos = pos + bound;
    lol_throw_if(maxendpos < pos);
    lol_throw_if(impl_reserve(maxendpos));
    auto result = libdeflate_deflate_compress(ctx.get(), src, src_count, data_ + pos, bound);
    lol_throw_if(result == 0);
    size_ = std::max(size_, pos + result);
    return result;
}

auto Buffer::write_compress_zlib(std::size_t pos, void const* src, std::size_t src_count, int level) -> std::size_t {
    lol_trace_func(lol_trace_var("{:#x}", size_),
                   lol_trace_var("{:#x}", pos),
                   lol_trace_var("{:p}", src),
                   lol_trace_var("{:#x}", src_count),
                   lol_trace_var("{:}", level));
    if (level < 0 || level > 12) level = 6;
    auto ctx = std::shared_ptr<libdeflate_compressor>(libdeflate_alloc_compressor(level), libdeflate_free_compressor);
    auto const bound = libdeflate_zlib_compress_bound(ctx.get(), src_count);
    auto const maxendpos = pos + bound;
    lol_throw_if(maxendpos < pos);
    lol_throw_if(impl_reserve(maxendpos));
    auto result = libdeflate_zlib_compress(ctx.get(), src, src_count, data_ + pos, bound);
    lol_throw_if(result == 0);
    size_ = std::max(size_, pos + result);
    return result;
}

auto Buffer::write_compress_gzip(std::size_t pos, void const* src, std::size_t src_count, int level) -> std::size_t {
    lol_trace_func(lol_trace_var("{:#x}", size_),
                   lol_trace_var("{:#x}", pos),
                   lol_trace_var("{:p}", src),
                   lol_trace_var("{:#x}", src_count),
                   lol_trace_var("{:}", level));
    if (level < 0 || level > 12) level = 6;
    auto ctx = std::shared_ptr<libdeflate_compressor>(libdeflate_alloc_compressor(level), libdeflate_free_compressor);
    auto const bound = libdeflate_gzip_compress_bound(ctx.get(), src_count);
    auto const maxendpos = pos + bound;
    lol_throw_if(maxendpos < pos);
    lol_throw_if(impl_reserve(maxendpos));
    auto result = libdeflate_gzip_compress(ctx.get(), src, src_count, data_ + pos, bound);
    lol_throw_if(result == 0);
    size_ = std::max(size_, pos + result);
    return result;
}

auto Buffer::write_compress_zstd(std::size_t pos, void const* src, std::size_t src_count, int level) -> std::size_t {
    lol_trace_func(lol_trace_var("{:#x}", size_),
                   lol_trace_var("{:#x}", pos),
                   lol_trace_var("{:p}", src),
                   lol_trace_var("{:#x}", src_count),
                   lol_trace_var("{:}", level));
    auto const bound = ZSTD_compressBound(src_count);
    auto const maxendpos = pos + bound;
    lol_throw_if(maxendpos < pos);
    lol_throw_if(impl_reserve(maxendpos));
    auto result = ZSTD_compress(data_ + pos, bound, src, src_count, level);
    lol_throw_if_msg(ZSTD_isError(result), ": {}", ZSTD_getErrorName(result));
    size_ = std::max(size_, pos + result);
    return result;
}

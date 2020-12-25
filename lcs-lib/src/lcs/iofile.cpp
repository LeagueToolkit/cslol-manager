#include "iofile.hpp"
#include "error.hpp"

using namespace LCS;

File::File(fs::path const& path, std::string const& mode)
    : path_(path), mode_(mode), handle_(nullptr)
{
    lcs_trace_func();
    lcs_trace("path_: ", path_, "mode_: ", mode_);
#ifdef WIN32
#define _CRT_SECURE_NO_WARNINGS
    std::wstring wmode(mode_.begin(), mode_.end());
    handle_ = FileHandle { _wfopen(path.c_str(), wmode.c_str()) };
#else
    handle_ =  FileHandle { fopen(path.c_str(), mode_.c_str()) };
#endif
    lcs_assert(!!handle_);
}

void File::write(void const* data, std::size_t size) {
    lcs_trace_func();
    lcs_trace("path_: ", path_, "mode_: ", mode_, "size: ", size);
    lcs_assert(fwrite(data, 1, size, handle_.get()) == size);
}

void File::read(void* data, std::size_t size) {
    lcs_trace_func();
    lcs_trace("path_: ", path_, "mode_: ", mode_, "size: ", size);
    lcs_trace("cur: ", tell());
    lcs_assert(fread(data, 1, size, handle_.get()) == size);
}

void File::seek(std::int64_t pos, int origin) {
    lcs_trace_func();
    lcs_trace("path_: ", path_, "mode_: ", mode_, "pos: ", pos, "origin: ", origin);
    lcs_trace("cur: ", tell());
#ifdef WIN32
    lcs_assert(_fseeki64(handle_.get(), pos, origin) == 0);
#else
    static_assert(sizeof(void*) == 8, "Non-windows platforms must be 64bit!");
    lcs_assert(fseek(handle_.get(), pos, origin) == 0);
#endif
}

std::int64_t File::tell() const {
    lcs_trace_func();
    lcs_trace("path_: ", path_, "mode_: ", mode_);
#ifdef WIN32
    auto const result = _ftelli64(handle_.get());
#else
    static_assert(sizeof(void*) == 8, "Non-windows platforms must be 64bit!");
    auto const result = ftell(handle_.get());
#endif
    lcs_trace("pos: ", result);
    lcs_assert(result >= 0);
    return result;
}

std::int64_t File::size() {
    lcs_trace_func();
    lcs_trace("path_: ", path_, "mode_: ", mode_);
    auto const cur = tell();
    seek(0, SEEK_END);
    auto const result = tell();
    seek(cur, SEEK_SET);
    return result;
}

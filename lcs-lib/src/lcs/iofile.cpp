#ifdef WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif
#include "iofile.hpp"
#include "error.hpp"

using namespace LCS;

File::File(fs::path const& path, bool readonly)
    : path_(path), readonly_(readonly), handle_(nullptr)
{
    lcs_trace_func(
                lcs_trace_var(this->path_),
                lcs_trace_var(this->readonly_)
                );
    if (readonly) {
        lcs_assert(fs::exists(path));
        lcs_assert(fs::is_regular_file(path));
    } else {
        if (fs::exists(path)) {
            lcs_assert(fs::is_regular_file(path));
        } else {
            auto const parent = path.parent_path();
            lcs_assert(fs::exists(parent));
            lcs_assert(fs::is_directory(parent));
        }
    }
#ifdef WIN32
    handle_ = FileHandle { _wfopen(path.c_str(), readonly_ ? L"rb" : L"wb") };
#else
    handle_ =  FileHandle { fopen(path.c_str(), readonly_ ? "rb" : "wb") };
#endif
    lcs_assert_msg("Failed to open file, MAKE SURE:\n"
                   "\t1. its not opened by something else already (for example: another modding tool)!\n"
                   "\t2. league is NOT patching (restart both league and LCS)!\n"
                   "\t3. you have sufficient priviliges (for example: run as Administrator)", !!handle_);
}

void File::write(void const* data, std::size_t size) {
    lcs_trace_func(
                lcs_trace_var(this->path_),
                lcs_trace_var(this->readonly_),
                lcs_trace_var(size)
                );
    lcs_assert(fwrite(data, 1, size, handle_.get()) == size);
}

void File::read(void* data, std::size_t size) {
    lcs_trace_func(
                lcs_trace_var(this->path_),
                lcs_trace_var(this->readonly_),
                lcs_trace_var(size),
                lcs_trace_var(tell())
                );
    lcs_assert(fread(data, 1, size, handle_.get()) == size);
}

void File::seek(std::int64_t pos, int origin) {
    lcs_trace_func(
                lcs_trace_var(this->path_),
                lcs_trace_var(this->readonly_),
                lcs_trace_var(pos),
                lcs_trace_var(origin),
                lcs_trace_var(tell())
                );
#ifdef WIN32
    lcs_assert(_fseeki64(handle_.get(), pos, origin) == 0);
#else
    static_assert(sizeof(void*) == 8, "Non-windows platforms must be 64bit!");
    lcs_assert(fseek(handle_.get(), pos, origin) == 0);
#endif
}

std::int64_t File::tell() const {
    lcs_trace_func(
                lcs_trace_var(this->path_),
                lcs_trace_var(this->readonly_)
                );
#ifdef WIN32
    auto const result = _ftelli64(handle_.get());
#else
    static_assert(sizeof(void*) == 8, "Non-windows platforms must be 64bit!");
    auto const result = ftell(handle_.get());
#endif
    lcs_assert(result >= 0);
    return result;
}

std::int64_t File::size() {
    lcs_trace_func(
                lcs_trace_var(this->path_),
                lcs_trace_var(this->readonly_)
                );
    auto const cur = tell();
    seek(0, SEEK_END);
    auto const result = tell();
    seek(cur, SEEK_SET);
    return result;
}

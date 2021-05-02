#ifdef WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif
#include "iofile.hpp"
#include "error.hpp"
#include <string.h>

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
    auto error = _wfopen_s((FILE**)&handle_, path.c_str(), readonly_ ? L"rb" : L"wb");
#else
    auto error = fopen_s((FILE**)&handle_, path.c_str(), readonly_ ? "rb" : "wb");
#endif
    if (error != 0 || !handle_) {
        std::string msg = "Failed to open file: ";
        if (auto error_details = strerror(error)) {
            msg += error_details;
        }
        msg += "\nMAKE SURE:\n"
               "\t1. its not opened by something else already (for example: another modding tool)!\n"
               "\t2. league is NOT patching (restart both league and LCS)!\n"
               "\t3. you have sufficient priviliges (for example: run as Administrator)";
        throw_error(msg.c_str());
    }
}

File::~File() {
    if (handle_) {
        lcs_assert_msg("Failed to close a file?!?", fclose((FILE*)handle_) == 0);
        handle_ = nullptr;
    }
}


void File::write(void const* data, std::size_t size) {
    lcs_trace_func(
                lcs_trace_var(this->path_),
                lcs_trace_var(this->readonly_),
                lcs_trace_var(size)
                );
    lcs_assert(fwrite(data, 1, size, (FILE*)handle_) == size);
}

void File::read(void* data, std::size_t size) {
    lcs_trace_func(
                lcs_trace_var(this->path_),
                lcs_trace_var(this->readonly_),
                lcs_trace_var(size),
                lcs_trace_var(tell())
                );
    lcs_assert(fread(data, 1, size, (FILE*)handle_) == size);
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
    lcs_assert(_fseeki64((FILE*)handle_, pos, origin) == 0);
#else
    static_assert(sizeof(void*) == 8, "Non-windows platforms must be 64bit!");
    lcs_assert(fseek((FILE*)handle_, pos, origin) == 0);
#endif
}

std::int64_t File::tell() const {
    lcs_trace_func(
                lcs_trace_var(this->path_),
                lcs_trace_var(this->readonly_)
                );
#ifdef WIN32
    auto const result = _ftelli64((FILE*)handle_);
#else
    static_assert(sizeof(void*) == 8, "Non-windows platforms must be 64bit!");
    auto const result = ftell((FILE*)handle_);
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

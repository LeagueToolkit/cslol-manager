#include <miniz.h>

#include <lol/error.hpp>
#include <lol/io/bytes.hpp>
#include <lol/io/file.hpp>
#include <lol/utility/zip.hpp>
#include <memory>

using namespace lol;
using namespace lol::utility;

#define lol_throw_if_zip(func, ...) \
    lol_throw_if_msg(!(func(__VA_ARGS__)), #func ": {}", mz_zip_get_error_string(mz_zip_get_last_error(zip.get())))

auto utility::zip(fs::path const& src, fs::path const& dst) -> void {
    lol_trace_func(lol_trace_var("{}", src), lol_trace_var("{}", dst));
    auto zip = std::shared_ptr<mz_zip_archive>(new mz_zip_archive{}, [](mz_zip_archive* p) {
        mz_zip_writer_end(p);
        delete p;
    });
    lol_throw_if(zip.get() == nullptr);
    auto dst_file = io::File::create(dst);
    dst_file.reserve(0, 1 * io::GiB);
    zip->m_pIO_opaque = &dst_file;
    zip->m_pWrite = [](void* opaq, mz_uint64 pos, void const* src, size_t n) -> size_t {
        return opaq ? ((io::File*)opaq)->writesome(pos, src, n) : 0;
    };
    lol_throw_if_zip(mz_zip_writer_init_v2, zip.get(), 0, 0);
    for (auto const& dirent : fs::recursive_directory_iterator(src)) {
        if (!dirent.is_regular_file()) continue;
        auto file_path = dirent.path();
        auto src_file = io::Bytes::from_file(file_path);
        auto file_name = fs::relative(file_path, src).generic_string();
        lol_throw_if_zip(mz_zip_writer_add_mem, zip.get(), file_name.c_str(), src_file.data(), src_file.size(), 0);
    }
    lol_throw_if_zip(mz_zip_writer_finalize_archive, zip.get());
}

auto utility::unzip(fs::path const& src, fs::path const& dst) -> void {
    lol_trace_func(lol_trace_var("{}", src), lol_trace_var("{}", dst));
    auto zip = std::shared_ptr<mz_zip_archive>(new mz_zip_archive{}, [](mz_zip_archive* p) {
        mz_zip_reader_end(p);
        delete p;
    });
    lol_throw_if(zip.get() == nullptr);
    auto src_file = io::Bytes::from_file(src);
    lol_throw_if_zip(mz_zip_reader_init_mem, zip.get(), src_file.data(), src_file.size(), 0);
    for (std::uint32_t index = 0; index < zip->m_total_files; ++index) {
        mz_zip_archive_file_stat stat = {};
        lol_throw_if_zip(mz_zip_reader_file_stat, zip.get(), index, &stat);
        if (stat.m_is_directory || !stat.m_is_supported) continue;
        auto file_path = fs::path((char const*)stat.m_filename).lexically_normal();
        for (auto const& component : file_path) {
            lol_throw_if(component == "..");
        }
        auto dst_file = io::File::create(dst / file_path);
        auto dst_data = dst_file.resize(stat.m_uncomp_size);
        lol_throw_if_zip(mz_zip_reader_extract_to_mem_no_alloc, zip.get(), index, dst_data, dst_file.size(), 0, 0, 0);
    }
}
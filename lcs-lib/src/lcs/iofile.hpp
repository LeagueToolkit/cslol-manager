#ifndef LCS_IOFILE_HPP
#define LCS_IOFILE_HPP
#include <cstdio>
#include <memory>

#include "common.hpp"

namespace LCS {
    struct File {
    private:
        fs::path path_;
        bool readonly_;
        void* handle_;
    public:
        File(fs::path const& path, bool write);
        File(File const&) = delete;
        File(File&&) = delete;
        File& operator=(File const&) = delete;
        File& operator=(File&&) = delete;
        ~File();

        void write(void const* data, std::size_t size);
        void read(void* data, std::size_t size);
        void seek(std::int64_t pos, int origin);
        std::int64_t tell() const;
        std::int64_t size();
    };

    struct InFile {
    private:
        File file_;
    public:
        inline InFile(fs::path const& path) : file_(path, true) {}

        inline void read(void* data, std::size_t size) {
            file_.read(data, size);
        }

        inline void seek(std::int64_t pos, int origin) {
            file_.seek(pos, origin);
        }

        inline std::int64_t tell() const {
            return file_.tell();
        }

        inline std::int64_t size() {
            return file_.size();
        }
    };

    struct OutFile {
    private:
        File file_;
    public:
        inline OutFile(fs::path const& path) : file_(path, false) {}

        inline void write(void const* data, std::size_t size) {
            file_.write(data, size);
        }

        inline void seek(std::int64_t pos, int origin) {
            file_.seek(pos, origin);
        }

        inline std::int64_t tell() const {
            return file_.tell();
        }

        inline std::int64_t size() {
            return file_.size();
        }
    };
}

#endif // LCS_IOFILE_HPP

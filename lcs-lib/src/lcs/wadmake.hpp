#ifndef WADMAKE_HPP
#define WADMAKE_HPP
#include "common.hpp"
#include "wad.hpp"
#include "wadindex.hpp"
#include <optional>
#include <vector>
#include <map>

namespace LCS {
    struct WadMakeBase {
        virtual ~WadMakeBase() noexcept = 0;
        virtual void write(fs::path const& path, Progress& progress) const = 0;
        virtual size_t size() const noexcept = 0;
        virtual std::string const& name() const& noexcept = 0;
        virtual fs::path const& path() const& noexcept = 0;
        virtual std::optional<std::string> identify(WadIndex const& index) const noexcept = 0;
    };

    struct WadMakeCopy : WadMakeBase {
        WadMakeCopy(fs::path const& path);

        void write(fs::path const& path, Progress& progress) const override;

        inline size_t size() const noexcept override {
            return size_;
        }

        inline std::string const& name() const& noexcept override {
            return name_;
        }

        inline fs::path const& path() const& noexcept override {
            return path_;
        }

        inline auto const& entries() const& noexcept {
            return entries_;
        }

        inline std::optional<std::string> identify(WadIndex const& index) const noexcept override {
            if (auto wad = index.findOriginal(name_, entries_)) {
                return wad->name();
            }
            return {};
        }
    private:
        fs::path path_;
        std::string name_;
        std::vector<Wad::Entry> entries_;
        size_t size_ = 0;
    };

    struct WadMake : WadMakeBase {
        WadMake(fs::path const& path);

        void write(fs::path const& path, Progress& progress) const override;

        inline size_t size() const noexcept override {
            return size_;
        }

        inline std::string const& name() const& noexcept override {
            return name_;
        }

        inline fs::path const& path() const& noexcept override {
            return path_;
        }

        inline auto const& entries() const& noexcept {
            return entries_;
        }

        inline std::optional<std::string> identify(WadIndex const& index) const noexcept override {
            if (auto wad = index.findOriginal(name_, entries_)) {
                return wad->name();
            }
            return {};
        }
    private:
        fs::path path_;
        std::string name_;
        std::map<uint64_t, fs::path> entries_;
        size_t size_ = 0;
    };

    struct WadMakeUnZip : WadMakeBase {
        struct FileEntry {
            fs::path path;
            unsigned int index;
            size_t size;
        };
        WadMakeUnZip(fs::path const& source, void* archive);

        void add(fs::path const& path, unsigned int zipEntry, size_t size);

        void write(fs::path const& path, Progress& progress) const override;

        inline size_t size() const noexcept override;

        inline std::string const& name() const& noexcept override {
            return name_;
        }

        inline fs::path const& path() const& noexcept override {
            return path_;
        }

        inline auto const& entries() const& noexcept {
            return entries_;
        }

        inline std::optional<std::string> identify(WadIndex const& index) const noexcept override {
            if (auto wad = index.findOriginal(name_, entries_)) {
                return wad->name();
            }
            return {};
        }
    private:
        fs::path path_;
        std::string name_;
        void* archive_;
        std::map<uint64_t, FileEntry> entries_;
        mutable size_t size_ = 0;
        mutable bool sizeCalculated_;
    };
}

#endif // WADMAKE_HPP

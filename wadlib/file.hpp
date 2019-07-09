#pragma once
#include <cinttypes>
#include <cstddef>
#include <exception>
#include <type_traits>
#include <vector>
#include <cstdio>
#include <string_view>
#include <array>
#include <filesystem>

class File
{
private:
    FILE* f = nullptr;
public:
    class Error : public std::exception {
    public:
        using std::exception::exception;
    };

    File(File const&) = delete;
    inline File(File&& other) : f(other.f) { other.f = nullptr; }
    File& operator=(File const&) = delete;
    File& operator=(File&&) = delete;

    File(std::filesystem::path const& path, wchar_t const* mode);
    ~File();

    template<typename...Args>
    void scanf(char const* format, Args&...args) {
        if(fscanf_s(f, format, &args...) != sizeof...(args)) {
            throw Error("scanf failed!");
        }
    }

    int peekc() const;

    bool is_eof() const;

    void rread(void * data, size_t count, size_t elemsize) const;

    void rwrite(void const* data, size_t count, size_t elemsize) const;

    std::string readline() const;

    inline int32_t size() const {
        auto const old = tell();
        seek_end(0);
        auto const res = tell();
        seek_beg(old);
        return res;
    }

    int32_t tell() const;

    void seek_beg(int32_t pos) const;

    void seek_cur(int32_t pos) const;

    void seek_end(int32_t pos) const;

    void read_zstring(std::string& str) const;

    void read_sized_string(std::string& str) const;

    template<typename T>
    inline void rread(T* data, size_t count = 1) const {
        return rread(data, count, sizeof(T));
    }

    template<typename T>
    inline void rwrite(T const* data, size_t count = 1) const {
        return rwrite(data, count, sizeof(T));
    }

    template<typename T>
    inline void read(T& data) const;

    template<typename T>
    inline void write(T const& data) const;
};

namespace FileImpl {
    template<typename T>
    inline std::enable_if_t<std::is_arithmetic_v<T>, void>
    read(File const& file, T& data) {
        file.rread(&data);
    }

    template<typename T>
    inline std::enable_if_t<std::is_arithmetic_v<T>, void>
    write(File const& file, T const& data) {
        file.rwrite(&data);
    }

    template<typename T>
    inline std::enable_if_t<!std::is_arithmetic_v<T>, void>
    read(File const& file, std::vector<T>& data) {
        for(auto& elem : data) {
            read(file, elem);
        }
    }

    template<typename T>
    inline std::enable_if_t<!std::is_arithmetic_v<T>, void>
    write(File const& file, std::vector<T> const& data) {
        for(auto const& elem : data) {
            write(file, elem);
        }
    }

    template<typename T>
    inline std::enable_if_t<std::is_arithmetic_v<T>, void>
    read(File const& file, std::vector<T>& data) {
        return file.rread(&data[0], data.size());
    }

    template<typename T>
    inline std::enable_if_t<std::is_arithmetic_v<T>, void>
    write(File const& file, std::vector<T> const& data) {
        return file.rwrite(&data[0], data.size());
    }

    template<typename T, size_t S>
    inline std::enable_if_t<!std::is_arithmetic_v<T>, void>
    read(File const& file, std::array<T, S>& data) {
        for(auto& elem : data) {
            read(file, elem);
        }
    }

    template<typename T, size_t S>
    inline std::enable_if_t<!std::is_arithmetic_v<T>, void>
    write(File const& file, std::array<T, S> const& data) {
        for(auto const& elem : data) {
            write(file, elem);
        }
    }

    template<typename T, size_t S>
    inline std::enable_if_t<std::is_arithmetic_v<T>, void>
    read(File const& file, std::array<T, S>& data) {
        return file.rread(&data[0], S);
    }

    template<typename T, size_t S>
    inline std::enable_if_t<std::is_arithmetic_v<T>, void>
    write(File const& file, std::array<T, S> const& data) {
        return file.rwrite(&data[0], S);
    }

    template<typename T, size_t S>
    inline std::enable_if_t<!std::is_arithmetic_v<T>, void>
    read(File const& file, T (&data)[S]) {
        for(auto& elem : data) {
            read(file, elem);
        }
    }

    template<typename T, size_t S>
    inline std::enable_if_t<!std::is_arithmetic_v<T>, void>
    write(File const& file, T const (&data)[S]) {
        for(auto const& elem : data) {
            write(file, elem);
        }
    }

    template<typename T, size_t S>
    inline std::enable_if_t<std::is_arithmetic_v<T>, void>
    read(File const& file, T (&data)[S]) {
        return file.rread(&data[0], S);
    }

    template<typename T, size_t S>
    inline std::enable_if_t<std::is_arithmetic_v<T>, void>
    write(File const& file, T const (&data)[S]) {
        return file.rwrite(&data[0], S);
    }

    inline void
    read(File const& file, std::string& data) {
        return file.rread(&data[0], data.size());
    }

    inline void
    write(File const& file, std::string const& data) {
        return file.rwrite(&data[0], data.size());
    }
};

template<typename T>
inline void File::read(T& data) const {
    using FileImpl::read;
    return read(*this, data);
}

template<typename T>
inline void File::write(T const& data) const {
    using FileImpl::write;
    return write(*this, data);
}

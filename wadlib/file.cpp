#include "file.hpp"
#include <cstdio>


File::File(const std::filesystem::path &path, const wchar_t *mode)
{
    if(_wfopen_s(&f, path.c_str(), mode)) {
        throw Error("Failed to open the file!");
    }
}

File::~File()
{
    if(f) {
        fclose(f);
    }
}

int File::peekc() const
{
    int c = fgetc(f);
    ungetc(c, f);
    return c;
}


bool File::is_eof() const
{
    return peekc() == EOF;
}

void File::rread(void *data, size_t count, size_t elemsize) const
{
    if(fread(data, elemsize, count, f) != count){
        throw Error("Failed to read from file!");
    }
}

void File::rwrite(const void *data, size_t count, size_t elemsize) const
{
    if(fwrite(data, elemsize, count, f) != count) {
        throw Error("Failed to write to file!");
    }
}


std::string File::readline() const
{
    size_t size = 0;
    int32_t nlsize = 0;
    {
        auto start = tell();
        for(;;) {
            int const c = fgetc(f);
            if(c == '\r') {
                nlsize++;
            } else if (c == '\n') {
                nlsize++;
                break;
            } else if (c == EOF) {
                break;
            } else {
                size++;
            }
        }
        seek_beg(start);
    }
    std::string results;
    results.resize(size);
    read(results);
    seek_cur(nlsize);
    return results;
}

int32_t File::tell() const
{
    if(auto const r = ftell(f);
            r < 0) {
        throw Error("Failed to tell file position!");
    } else {
        return static_cast<int32_t>(r);
    }
}

void File::seek_beg(int32_t pos) const
{
    if(auto const r = fseek(f, static_cast<long int>(pos), SEEK_SET);
            r < 0) {
        throw Error("Failed to seek from begining!");
    }
}

void File::seek_cur(int32_t pos) const
{
    if(auto const r = fseek(f, static_cast<long int>(pos), SEEK_CUR);
            r < 0) {
        throw Error("Failed to seek from current position!");
    }
}

void File::seek_end(int32_t pos) const
{
    if(auto const r = fseek(f, static_cast<long int>(pos), SEEK_END);
            r < 0) {
        throw Error("Failed to seek from end!");
    }
}

void File::read_sized_string(std::string &str) const
{
    int32_t length;
    read(length);
    if(length < 1) {
        str.clear();
        return;
    }
    auto const start = tell();
    seek_end(0);
    auto const end = tell();
    seek_beg(start);
    if(length > (end - start)) {
        throw File::Error("Sized string to big!");
    }
    str.resize(static_cast<size_t>(length));
    rread(&str[0], static_cast<size_t>(length));
}

void File::read_zstring(std::string& str) const
{
    size_t size = 0;
    auto start = tell();
    for(;;) {
        auto c = getc(f);
        if(c == EOF) {
            throw Error("EOF while reading zstring!");
        }
        if(c == '\0') {
            break;
        }
        size++;
    }
    seek_beg(start);
    str.resize(size);
    rread(&str[0], size + 1);
}

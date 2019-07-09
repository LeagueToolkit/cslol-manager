#include "wadlib.h"
#include "xxhash.h"
#include "file.hpp"
#include "link.h"

namespace fs = std::filesystem;
using fspath = fs::path;

uint32_t xx32_lower(std::string str) noexcept
{
    for(auto& c: str) { c = static_cast<char>(::tolower(c)); }
    return XXH32(str.c_str(), str.size(), 0);
}

uint64_t xx64_lower(std::string str) noexcept
{
    for(auto& c: str) { c = static_cast<char>(::tolower(c)); }
    return XXH64(str.c_str(), str.size(), 0);
}

std::string read_link(const std::string &path) noexcept {
    try {
        Link lnk = {};
        File file(path, L"rb");
        file.read(lnk);
        return fspath(lnk.path).lexically_normal().generic_string();
    } catch(File::Error const&) {}
    return "";
}

std::string find_league(std::string path) noexcept {
    fspath lol;
    if(path.size() < 3) {
        return "";
    }
    if(path[0] == '"') {
        path = path.substr(1, path.size() - 2);
    }
    lol = path;
    if(lol.filename().extension() == ".lnk") {
        try {
            Link lnk = {};
            File file(lol, L"rb");
            file.read(lnk);
            lol = lnk.path;
        } catch(File::Error const&) {}
    }
    if(lol.filename() == "LeagueClient.exe"
            || lol.filename() == "LeagueClientUx.exe"
            || lol.filename() == "LeagueClientUxRender.exe"
            || lol.filename() == "lol.launcher.exe"
            || lol.filename() == "lol.launcher.admin.exe") {
        lol = lol.parent_path() / "Game" / "League of Legends.exe";
    }
    if(lol.filename() == "League of Legends.exe") {
        lol = lol.parent_path();
    }
    if(std::filesystem::exists(lol / "League of Legends.exe")) {
        return lol.lexically_normal().generic_string();
    }
    return "";
}

std::string read_line(const std::string &from) noexcept {
    try {
        File config(from, L"rb");
        return config.readline();
    } catch(File::Error const&) {
        return "";
    }
}

bool write_line(const std::string &to, const std::string &line) noexcept {
    try {
        fs::create_directories(fspath(to).parent_path());
        File config(to, L"wb");
        config.write(line);
        config.write('\n');
        return true;
    } catch(File::Error const&) {
        return false;
    }
}

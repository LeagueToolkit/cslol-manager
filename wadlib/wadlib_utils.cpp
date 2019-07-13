#include "wadlib.h"
#include "xxhash.h"
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

void UpdatePrinter::operator()(const std::string &name, int64_t done, int64_t total) noexcept
{
    if(old != name) {
        old = name;
        printf("%s\n", name.c_str());
    }
    if(done == 0 && total == 0) {
        printf("Skiping!\n");
    } else {
        printf("\r%4.2fMB/%4.2fMB", done / 1024.0 / 1024.0, total / 1024.0 / 1024.0);
        if(done == total) {
            printf("\n");
        }
    }
}

static std::string file_read_line(fspath const& from) noexcept {
    try {
        File config(from, L"rb");
        return config.readline();
    } catch(File::Error const&) {
        return "";
    }
}

static bool file_write_line(fspath const &to, const std::string &line) noexcept {
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

static std::string find_league(std::string path) noexcept {
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

fspath get_league_path(fspath cmdpath, fspath configfile) {
    if(!cmdpath.empty()) {
        cmdpath = find_league(cmdpath.generic_string());
        if(fs::exists(cmdpath / "League of Legends.exe")) {
            if(!configfile.empty()) {
                file_write_line(configfile, cmdpath.generic_string());
            }
            return cmdpath;
        }
    }

    cmdpath = file_read_line(configfile);
    if(fs::exists(cmdpath / "League of Legends.exe")) {
        return cmdpath;
    }

    cmdpath = open_file_browse(browse_filter_lol_exe, "lol.exe", "Find League of Legends!");
    if(!cmdpath.empty()) {
        cmdpath = find_league(cmdpath.generic_string());
        if(fs::exists(cmdpath / "League of Legends.exe")) {
            if(!configfile.empty()) {
                file_write_line(configfile, cmdpath.generic_string());
            }
            return cmdpath;
        }
    }
    return {};
}

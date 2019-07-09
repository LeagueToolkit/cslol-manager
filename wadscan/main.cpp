#include <string>
#include <filesystem>
#include <iostream>
#include "wadlib.h"
#include "file.hpp"

using namespace std;
namespace fs = std::filesystem;

int main(int argc, char**argv) {
    if(argc < 1) {
        puts("wadscan.exe <path>");
        puts("Scans the folder recursively for .wad files and dumps TOC.");
        return 1;
    }
    fspath dir = argv[1];
    if(!fs::is_directory(dir) || !fs::exists(dir)) {
        puts("Path must be a directory!");
        return 1;
    }
    for(auto const& ent: fs::recursive_directory_iterator(dir)) {
        if(ent.is_directory() || !ent.is_regular_file()) {
            continue;
        }
        auto path = ent.path();
        if(path.extension() != ".client" && path.extension() != ".wad") {
            continue;
        }
        try {
            auto const rel = fs::relative(path, dir).lexically_normal().generic_string();
            WadFile file(path);
            for(auto const& [xx, entry]: file.entries) {
                printf("%llx:%s:%u\n", xx, rel.c_str(), entry.type);
            }
        } catch(std::exception const&) {}
    }
    return 0;
}

#include "wadlib.h"

bool load_hashdb(HashMap &hashes, fspath const& name) {
    try {
        File file(name, L"rb");
        uint32_t count = 0;
        file.read(count);
        std::array<char, 4> sig = {};
        file.read(sig);
        if(sig != std::array<char, 4>{'x', 'x', '6', '4'}) {
            return false;
        }
        file.seek_cur(256 - 8);
        hashes.reserve(hashes.size() + static_cast<size_t>(count));
        for(uint32_t i = 0; i < count; i++) {
            uint64_t xx;
            HashEntry entry;
            file.read(xx);
            file.read(entry.name);
            entry.name[sizeof(entry.name) - 1] = '\0';
            hashes[xx] = entry;
        }
    } catch(File::Error const&) {
        return false;
    }
    return true;
}

bool save_hashdb(const HashMap &hashmap, fspath const &name) {
    try{
        File file(name, L"wb");
        uint32_t count = static_cast<uint32_t>(hashmap.size());
        file.write(count);
        std::array<char, 4> sig = {'x','x','6','4'};
        file.write(sig);
        file.seek_cur(256 - 8);
        for(auto const& [xx, entry]: hashmap) {
            file.write(xx);
            file.write(entry.name);
        }
        return true;
    } catch(File::Error const&) {
        return false;
    }
}

bool import_hashes(HashMap& hashmap, fspath const& name) {
    try {
        File file(name, L"rb");
        std::string line;
        fspath path;
        for(; !file.is_eof() ;) {
            HashEntry entry{};
            line = file.readline();
            if(line.empty()) {
                continue;
            }
            uint64_t const h = xx64_lower(line);
            if(line.size() >= sizeof(entry.name)) {
                continue;
            }
            fspath const path = line;
            bool bad = false;
            for(auto& component: path) {
                auto const component_str = component.lexically_normal().generic_string();
                if(component_str.size() >= 127) {
                    bad = true;
                }
            }
            if(bad) {
                continue;
            }
            std::copy(line.begin(), line.end(), entry.name);
            hashmap[h] = entry;
        }
        return true;
    } catch(File::Error const&) {
        return false;
    }
}

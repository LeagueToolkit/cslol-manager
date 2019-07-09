#include "wadlib.h"
namespace fs = std::filesystem;


void wad_mods_scan_one(
        wad_mods &mods,
        fspath const& mod_path,
        conflictfn) {
    if(!fs::exists(mod_path) || !fs::is_directory(mod_path)) {
        return;
    }
    for(auto wad_ent : fs::recursive_directory_iterator(mod_path)) {
        if(wad_ent.path().extension() != ".client") {
            continue;
        }
        if(wad_ent.is_directory()) {
            continue;
        }
        auto str = fs::relative(wad_ent, mod_path).lexically_normal().generic_string();
        for(auto& c: str) { c = static_cast<char>(::tolower(c)); }
        mods[str].emplace(mod_path.filename().generic_string(), wad_ent.path());
    }
}

void wad_mods_scan_recursive(
        wad_mods &mods,
        fspath const& modspath,
        conflictfn) {
    if(!fs::exists(modspath) || !fs::is_directory(modspath)) {
        return;
    }
    for(auto const& mod_ent : fs::directory_iterator(modspath)) {
        if(!fs::is_directory(mod_ent)) {
            continue;
        }
        auto const mod_path = mod_ent.path();
        if(mod_path.extension() == ".disabled") {
            continue;
        }
        wad_mods_scan_one(mods, mod_path);
    }
}

void wad_mods_install(
        wad_mods const&mods,
        fspath const& orgdir,
        fspath const& modsdir,
        updatefn update,
        int32_t const buffercap) {
    for(auto& [dst_raw, wads] : mods) {
        auto org_raw = orgdir / dst_raw;
        if(!fs::exists(org_raw)) {
            continue;
        }
        auto org = fs::directory_entry(org_raw).path();
        auto dst = modsdir / fs::relative(org, orgdir);
        wad_merge(dst, org, wads, update, buffercap);
    }

    for(auto wad_ent : fs::recursive_directory_iterator(modsdir)) {
        if(wad_ent.path().extension() != ".client" || wad_ent.path().extension() == ".wad") {
            continue;
        }
        if(wad_ent.is_directory()) {
            continue;
        }
        auto str = fs::relative(wad_ent, modsdir).lexically_normal().generic_string();
        for(auto& c: str) { c = static_cast<char>(::tolower(c)); }
        if(auto f = mods.find(str); f == mods.end() || f->second.empty()) {
            fs::remove(wad_ent);
        }
    }
}

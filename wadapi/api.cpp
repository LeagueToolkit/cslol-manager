#include "installer.h"
#include <filesystem>
#define WADAPI_IMPL 1
#include "api.h"

namespace fs = std::filesystem;

int wadapi_wad_make(
        const char *dst_path, uint32_t dst_path_size,
        const char *src_path, uint32_t src_path_size,
        update_fn c_update, void *c_update_data,
        int32_t bufferMax, char *err_out, uint32_t err_out_max, uint32_t *err_out_size)
{
    if(!dst_path || !src_path || !dst_path_size || !src_path_size) {
        return -2;
    }
    if(bufferMax < 4096) {
        bufferMax = 1024 * 1024;
    }
    try {
        fspath const dst(std::string_view(dst_path, dst_path_size));
        fspath const src(std::string_view(src_path, src_path_size));
        if(!fs::exists(src) || !fs::is_directory(src)) {
            return -2;
        }
        if(fs::exists(dst) && fs::is_directory(dst)) {
            return -2;
        }
        updatefn update = nullptr;
        if(c_update) {
            update = [c_update, c_update_data](auto const& n, auto d, auto t) {
                c_update(n.c_str(), static_cast<uint32_t>(n.size()), d, t, c_update_data);
            };
        }
        wad_make(dst, src, update, bufferMax);
        return 0;
    } catch(std::exception const& err) {
        if(err_out && err_out_max) {
            auto what = err.what();
            if(strcpy_s(err_out, err_out_max, what)) {
                return -3;
            }
            if(err_out_size) {
                *err_out_size = ::strlen(what);
            }
        }
        return -1;
    }
}

wad_mods *wadapi_wad_mods_create() {
    return new wad_mods;
}

void wadapi_wad_mods_free(wad_mods *wad_mods) {
    delete wad_mods;
}

void wadapi_wad_mods_clear(wad_mods *wad_mods) {
    if(wad_mods) {
        wad_mods->clear();
    }
}

int wadapi_wad_modscan_one(wad_mods* wad_mods,
        const char *src_path, uint32_t src_path_size,
        void* reserved1, void* reserverd2,
        char* err_out, uint32_t err_out_max, uint32_t* err_out_size) {
    if(reserved1 || reserverd2) {
        return -2;
    }
    if(!wad_mods || !src_path || !src_path_size) {
        return -2;
    }
    try {
        fspath path(std::string_view(src_path, src_path_size));
        wad_mods_scan_one(*wad_mods, path);
        return 0;
    } catch(std::exception const& err) {
        if(err_out && err_out_max) {
            auto what = err.what();
            if(strcpy_s(err_out, err_out_max, what)) {
                return -3;
            }
            if(err_out_size) {
                *err_out_size = ::strlen(what);
            }
        }
        return -1;
    }
}

int wadapi_wad_modscan_recursive(
        wad_mods* wad_mods,
        const char *src_path, uint32_t src_path_size,
        void* reserved1, void* reserverd2,
        char* err_out, uint32_t err_out_max, uint32_t* err_out_size) {
    if(reserved1 || reserverd2) {
        return -2;
    }
    if(!wad_mods || !src_path || !src_path_size) {
        return -2;
    }
    try {
        fspath path(std::string_view(src_path, src_path_size));
        wad_mods_scan_recursive(*wad_mods, path);
        return 0;
    } catch(std::exception const& err) {
        if(err_out && err_out_max) {
            auto what = err.what();
            if(strcpy_s(err_out, err_out_max, what)) {
                return -3;
            }
            if(err_out_size) {
                *err_out_size = ::strlen(what);
            }
        }
        return -1;
    }
}

int wadapi_wad_mods_install(const wad_mods *wad_mods,
        const char *orgdir_path, uint32_t orgdir_path_size,
        const char *moddir_path, uint32_t moddir_path_size,
        update_fn c_update, void* c_update_data,
        int32_t bufferMax,
        char* err_out, uint32_t err_out_max, uint32_t* err_out_size) {
    if(!wad_mods || !orgdir_path || !moddir_path || !orgdir_path_size || !moddir_path_size) {
        return -2;
    }
    updatefn update = nullptr;
    if(c_update) {
        update = [c_update, c_update_data](auto const& n, auto d, auto t) {
            c_update(n.c_str(), static_cast<uint32_t>(n.size()), d, t, c_update_data);
        };
    }
    fspath orgdir(std::string_view(orgdir_path, orgdir_path_size));
    fspath moddir(std::string_view(moddir_path, moddir_path_size));
    try {
        ::wad_mods_install(*wad_mods, orgdir, moddir, update, bufferMax);
        return 0;
    } catch(std::exception const& err) {
        if(err_out && err_out_max) {
            auto what = err.what();
            if(strcpy_s(err_out, err_out_max, what)) {
                return -3;
            }
            if(err_out_size) {
                *err_out_size = ::strlen(what);
            }
        }
        return -1;
    }
}


/*
    typedef int(*conflict_fn)(
        uint64_t xxhash,
        char const* old_wad_path, uint32_t old_wad_path_size,
        char const* new_wad_path, uint32_t new_wad_path_size,
        void* fn_data);

*/

#ifndef API_H
#define API_H

#include <inttypes.h>
#ifdef WADAPI_IMPL
#define WADAPI_EXPORTS __declspec(dllexport)
#else
#define WADAPI_EXPORTS __declspec(dllimport)
#ifdef __cplusplus
#include <string>
#include <vector>
#include <unordered_map>
#include "installer.h"
#else
struct wad_mods;
struct wad_files;
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus
    typedef int(*update_fn)(
        char const* dst, uint32_t dst_size,
        int64_t bytes_done, int64_t bytes_total, void* fn_data);

    /*
     * Descriptions:
     * Makes a .wad from folder
     *
     * Arguments:
     * dst_path, dst_path_size: IN string buffer containing the path to destination wad
     * dst_path, dst_path_size: IN string buffer containing the path to source folder
     * c_update, c_update_data: OPTIONAL, update function to be called on each buffer copy
     * bufferMax: OPTIONAL, copy buffer in bytes
     * err_out, err_out_max: OUT string buffer to copy error message in
     * err_out_size: OPTIONAL OUT size of error message string
     *
     * Returns (int):
     *  (0) OK
     *  (-1) Something went wrong
     *  (-2) Bad arguments
     *  (-3) Error message buffer to small
    */
    WADAPI_EXPORTS int wadapi_wad_make(
            char const* dst_path, uint32_t dst_path_size,
            char const* src_path, uint32_t src_path_size,
            update_fn c_update, void* c_update_data,
            int32_t bufferMax,
            char* err_out, uint32_t err_out_max, uint32_t* err_out_size);

    /*
     * Description:
     * Creates wad_mods structure.
     * The structure is used by ALL: wadapi_wad_mods_* functions
     *
     * Arguments:
     *
     * Returns (wad_mods*):
     *
    */
    WADAPI_EXPORTS wad_mods* wadapi_wad_mods_create();

    /*
     * Description:
     * Free wad_mods structure
     *
     * Arguments:
     * wad_mods: a wad_mods structure to be freed
     *
     * Returns (void):
     *
    */
    WADAPI_EXPORTS void wadapi_wad_mods_free(wad_mods* wad_mods);

    /*
     * Description:
     * Clears wad_mods structure
     *
     * Arguments:
     * wad_mods: a wad_mods structure to be cleared
     *
     * Returns (void):
     *
    */
    WADAPI_EXPORTS void wadapi_wad_mods_clear(wad_mods* wad_mods);

    /*
     * Description:
     * Scans single mod from its root and adds the files to wad_mods structure
     *
     * Arguments:
     * wad_mods: a wad_mods structure to be updated
     * src_path, src_path_size: IN string buffer containing single mod root folder path
     * reserved1, reserved2: reserved fields, must be set to null
     * err_out, err_out_max: OUT string buffer to copy error message in
     * err_out_size: OPTIONAL OUT size of error message string
     *
     * Returns (int):
     *  (0) OK
     *  (-1) Something went wrong
     *  (-2) Bad arguments
     *  (-3) Error message buffer to small
     *
    */
    WADAPI_EXPORTS int wadapi_wad_modscan_one(
            wad_mods* wad_mods,
            char const* src_path, uint32_t src_path_size,
            void* reserved1, void* reserved2,
            char* err_out, uint32_t err_out_max, uint32_t* err_out_size);

    /*
     * Description:
     * Scans collection of mods and fills wad_mods structure
     *
     * Arguments:
     * wad_mods: a wad_mods structure to be updated
     * src_path, src_path_size: IN string buffer containing mods collection folder path
     * reserved1, reserved2: reserved fields, must be set to null
     * err_out, err_out_max: OUT string buffer to copy error message in
     * err_out_size: OPTIONAL OUT size of error message string
     *
     * Returns (int):
     *  (0) OK
     *  (-1) Something went wrong
     *  (-2) Bad arguments
     *  (-3) Error message buffer to small
     *
    */
    WADAPI_EXPORTS int wadapi_wad_modscan_recursive(
            wad_mods* wad_mods,
            char const* src_path, uint32_t src_path_size,
            void* reserved1, void* reserved2,
            char* err_out, uint32_t err_out_max, uint32_t* err_out_size);

    /*
     * Description:
     * Installs .wad mods
     *
     * Arguments:
     * wad_mods: a wad_mods structure to install mods from
     * orgdir_path, orgdir_path_size: IN string buffer containing path to league game folder
     * moddir_path, moddir_path_size: IN string buffer containing path to final moded .wad's folder
     * c_update, c_update_data: OPTIONAL, update function to be called on each buffer copy
     * bufferMax: OPTIONAL, copy buffer in bytes
     * err_out, err_out_max: OUT string buffer to copy error message in
     * err_out_size: OPTIONAL OUT size of error message string
     *
     * Returns (int):
     *  (0) OK
     *  (-1) Something went wrong
     *  (-2) Bad arguments
     *  (-3) Error message buffer to small
     *
    */
    WADAPI_EXPORTS int wadapi_wad_mods_install(
            wad_mods const * wad_mods,
            char const* orgdir_path, uint32_t orgdir_path_size,
            char const* moddir_path, uint32_t moddir_path_size,
            update_fn c_update, void* c_update_data,
            int32_t bufferMax,
            char* err_out, uint32_t err_out_max, uint32_t* err_out_size);
#ifdef __cplusplus
}
#endif //__cplusplus

#endif // API_H

#pragma once
#include <cstdint>
#include <cstddef>
#include <stdexcept>
#include <cstdio>
#include <cstdlib>
#include "patscanner.hpp"
#include "process.hpp"

namespace ModOverlay {
    struct CodePayload {
        uint8_t Verify[0x10] = {
            0xB8, 0x01, 0x00, 0x00, 0x00, 0xC3, 0x90, 0x90,
            0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
        };

        uint8_t PrefixFn[0x30] = {
            0x57, 0x56, 0x8b, 0x54, 0x24, 0x0c, 0x8b, 0x74,
            0x24, 0x14, 0x89, 0xd7, 0xac, 0xaa, 0x84, 0xc0,
            0x75, 0xfa, 0x8b, 0x74, 0x24, 0x10, 0x83, 0xef,
            0x01, 0xac, 0xaa, 0x84, 0xc0, 0x75, 0xfa, 0x5e,
            0x89, 0xd0, 0x5f, 0xc3, 0x90, 0x90, 0x90, 0x90,
            0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
        };

        uint8_t Open[0x50] = {
            0x56, 0x53, 0x81, 0xec, 0x14, 0x02, 0x00, 0x00,
            0x8b, 0x41, 0x04, 0x8b, 0x58, 0x08, 0x8b, 0x03,
            0x8b, 0x30, 0x8d, 0x41, 0x0c, 0x89, 0x44, 0x24,
            0x08, 0x8b, 0x84, 0x24, 0x20, 0x02, 0x00, 0x00,
            0x89, 0x44, 0x24, 0x04, 0x8d, 0x44, 0x24, 0x10,
            0x89, 0x04, 0x24, 0xff, 0x51, 0x08, 0x8b, 0x94,
            0x24, 0x24, 0x02, 0x00, 0x00, 0x89, 0xd9, 0x89,
            0x04, 0x24, 0x89, 0x54, 0x24, 0x04, 0xff, 0xd6,
            0x83, 0xec, 0x08, 0x81, 0xc4, 0x14, 0x02, 0x00,
            0x00, 0x5b, 0x5e, 0xc2, 0x08, 0x00, 0x90, 0x90,
        };

        uint8_t CheckAccess[0x50] = {
            0x56, 0x53, 0x81, 0xec, 0x14, 0x02, 0x00, 0x00,
            0x8b, 0x41, 0x04, 0x8b, 0x58, 0x08, 0x8b, 0x03,
            0x8b, 0x70, 0x04, 0x8d, 0x41, 0x0c, 0x89, 0x44,
            0x24, 0x08, 0x8b, 0x84, 0x24, 0x20, 0x02, 0x00,
            0x00, 0x89, 0x44, 0x24, 0x04, 0x8d, 0x44, 0x24,
            0x10, 0x89, 0x04, 0x24, 0xff, 0x51, 0x08, 0x8b,
            0x94, 0x24, 0x24, 0x02, 0x00, 0x00, 0x89, 0xd9,
            0x89, 0x04, 0x24, 0x89, 0x54, 0x24, 0x04, 0xff,
            0xd6, 0x83, 0xec, 0x08, 0x81, 0xc4, 0x14, 0x02,
            0x00, 0x00, 0x5b, 0x5e, 0xc2, 0x08, 0x00, 0x90,
        };

        uint8_t CreateIterator[0x10] = {
            0x31, 0xc0, 0xc2, 0x08, 0x00, 0x90, 0x90, 0x90,
            0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
        };

        uint8_t VectorDeleter[0x10] = {
            0x89, 0xc8, 0xc2, 0x04, 0x00, 0x90, 0x90, 0x90,
            0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
        };

        uint8_t Destructor[0x10] = {
            0xc3, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
            0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
        };

        uint8_t Deleter[0x10] = {
            0xc3, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
            0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
        };

        uint8_t IsRads[0x10] = {
            0x31, 0xc0, 0xc3, 0x90, 0x90, 0x90, 0x90, 0x90,
            0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
        };

        char Info[0x10] = {
            'C' ,'u' ,'s' ,'t' ,'o' ,'m' ,' ' ,'L' ,
            'o' ,'l' ,' ' ,'s' , 'k','i' ,'n' ,'\0',
        };
    };


    struct EVP_PKEY_METHOD {
        int pkey_id;
        int flags;
        uintptr_t init;
        uintptr_t copy;
        uintptr_t cleanup;
        uintptr_t paramgen_init;
        uintptr_t paramgen;
        uintptr_t keygen_init;
        uintptr_t keygen;
        uintptr_t sign_init;
        uintptr_t sign;
        uintptr_t verify_init;
        uint8_t* verify;
        uintptr_t verify_recover_init;
        uintptr_t verify_recover;
        uintptr_t signctx_init;
        uintptr_t signctx;
        uintptr_t verifyctx_init;
        uintptr_t verifyctx;
        uintptr_t encrypt_init;
        uintptr_t encrypt;
        uintptr_t decrypt_init;
        uintptr_t decrypt;
        uintptr_t derive_init;
        uintptr_t derive;
        uintptr_t ctrl;
        uintptr_t ctrl_str;
        uintptr_t digestsign;
        uintptr_t digestverify;
        uintptr_t check;
        uintptr_t public_check;
        uintptr_t param_check;
        uintptr_t digest_custom;
    };

    struct FileProvider {
        struct Vtable {
            uint8_t* Open;
            uint8_t* CheckAccess;
            uint8_t* CreateIterator;
#ifndef __APPLE__
            uint8_t* VectorDeleter;
#else
            uint8_t* Destructor;
            uint8_t* Deleter;
#endif
            uint8_t* IsRads;
        } *vtable;
        struct List {
            FileProvider* arr[4];
            uint32_t size;
        } *list;
        uint8_t* prefixFn;
        std::array<char, 256> prefix;
        char Info[0x10] = {
            'C' ,'u' ,'s' ,'t' ,'o' ,'m' ,' ' ,'L' ,
            'o' ,'l' ,' ' ,'s' , 'k','i' ,'n' ,'\0',
        };
    };

    inline constexpr auto pat_pmeth = Pattern<
        0x68, Any, Any, Any, Any,       // push    offset method_compare
        0x6A, 0x04,                     // push    4
        0x6A, 0x12,                     // push    12h
        0x8D, 0x44, 0x24, Any,          // lea     eax, [esp+0x1C]
        0x68, Cap<Any>, Any, Any, Any,  // push    offset pkey_methods
        0x50,                           // push    eax
        0xE8, Any, Any, Any, Any,       // call    OBJ_bsearch_
        0x83, 0xC4, 0x14,               // add     esp, 14h
        0x85, 0xC0                      // test    eax, eax
    >();

    inline constexpr auto pat_fp = Pattern<
        0x56,                           // push    esi
        0x8B, 0x74, 0x24, 0x08,         // mov     dword ptr esi, [esp+4+4]
        0xB8, Cap<Any>, Any, Any, Any,  // mov     eax, offset fileproviders_list
        0x33, 0xC9,                     // xor     ecx, ecx
        0x0F, 0x1F, 0x40, 0x00          // nop     dword ptr [eax+00h]
    >();


    inline constexpr char const schema[] = "lolskinmod-overlay v0 0x%08X 0x%08X 0x%08X\n";

    struct Config {
        uint32_t checksum = 0;
        uintptr_t off_fp  = 0;
        uintptr_t off_pmeth  = 0;
        std::array<char, 256> prefix = { "MOD/" };

        inline bool good(Process const& process) const noexcept {
            return checksum == process.Checksum() && off_fp && off_pmeth;
        }

        inline void print() const noexcept {
            printf(schema, checksum, off_fp, off_pmeth);
        }

        inline void save() const noexcept {
            if(FILE* file = nullptr; !fopen_s(&file, "lolskinmod.txt", "w") && file) {
                fprintf_s(file, schema, checksum, off_fp, off_pmeth);
                fclose(file);
            }
        }

        inline void load() noexcept {
            if(FILE* file = nullptr; !fopen_s(&file, "lolskinmod.txt", "r") && file) {
                if(fscanf_s(file, schema, &checksum, &off_fp, &off_pmeth) != 3) {
                    checksum = 0;
                    off_fp = 0;
                    off_pmeth = 0;
                }
                fclose(file);
            }
        }

        inline bool rescan(Process const& process) noexcept {
            process.WaitWindow("League of Legends (TM) Client", 50);
            auto data = process.Dump();
            auto res_pmeth = pat_pmeth(data.data(), data.size());
            auto res_fp = pat_fp(data.data(), data.size());

            if(!res_pmeth[0] || !res_fp[0]) {
                return false;
            }

            off_pmeth = process.Debase(*reinterpret_cast<uintptr_t const*>(res_pmeth[1]));
            off_fp = process.Debase(*reinterpret_cast<uintptr_t const*>(res_fp[1]));
            checksum = process.Checksum();
            return true;
        }

        inline void patch(Process const& process) const {
            auto rem_code = process.Allocate<CodePayload>();
            process.Write(rem_code, {});
            process.MarkExecutable(rem_code);

            auto org_pmeth_arr = process.Rebase<EVP_PKEY_METHOD*>(off_pmeth);
            auto rem_pmeth = process.Allocate<EVP_PKEY_METHOD>();
            auto org_pmeth_first = process.WaitNonZero(org_pmeth_arr);
            auto mod_pmeth = EVP_PKEY_METHOD {};
            process.Read(org_pmeth_first, mod_pmeth);
            mod_pmeth.verify = rem_code->Verify;
            process.Write(rem_pmeth, mod_pmeth);
            process.Write(org_pmeth_arr, rem_pmeth);

            auto org_fplist = process.Rebase<FileProvider::List>(off_fp);
            auto rem_fp = process.Allocate<FileProvider>();
            auto rem_fpvtbl = process.Allocate<FileProvider::Vtable>();
            process.Write(rem_fp, {
                              .vtable = rem_fpvtbl,
                              .list = org_fplist,
                              .prefixFn = rem_code->PrefixFn,
                              .prefix = prefix,
                          });
            process.Write(rem_fpvtbl, {
                              .Open = rem_code->Open,
                              .CheckAccess = rem_code->CheckAccess,
                              .CreateIterator = rem_code->CreateIterator,
#ifndef __APPLE__
                              .VectorDeleter = rem_code->VectorDeleter,
#else
                              .Destructor = rem_code->Destructor,
                              .Deleter = rem_code->Deleter,
#endif
                              .IsRads = rem_code->IsRads,
                          });


            auto mod_fplist = FileProvider::List {};
            process.WaitNonZero(org_fplist->arr);
            process.Read(org_fplist, mod_fplist);
            process.Write(org_fplist, {
                              .arr = {
                                  rem_fp,
                                  mod_fplist.arr[0],
                                  mod_fplist.arr[1],
                                  mod_fplist.arr[2],
                              },
                              .size = mod_fplist.size + 1,
                          });
        }
    };
}

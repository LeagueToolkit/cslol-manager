#include "modoverlay.hpp"
#include <array>
#include <cstdio>
#include <cstdlib>

using namespace LCS;
namespace {
    inline constexpr uint16_t Any = 0x0100u;

    // capture offset of given byte
    template <uint16_t C = Any>
    inline constexpr uint16_t Cap = C | 0x0200u;

    // returns function that searches for the given signature
    // result of search is array with first offset + any offsets captured
    template <uint16_t... ops>
    inline constexpr auto Pattern() {
        return [](uint8_t const *start, size_t size) constexpr noexcept {
            std::array<uint8_t const *, ((ops & 0x0200 ? 1 : 0) + ... + 1)> result = {};
            uint8_t const *const end = start + size + sizeof...(ops);
            for (uint8_t const *i = start; i != end; i++) {
                uint8_t const *c = i;
                if (((*c++ == (ops & 0xFF) || (ops & 0x100)) && ...)) {
                    uint8_t const *o = i;
                    size_t r = 0;
                    result[r++] = o;
                    ((ops & 0x200 ? result[r++] = o++ : o++), ...);
                    return result;
                }
            }
            return result;
        };
    }

    static inline constexpr auto PAT_PMETH =
        Pattern<0x68, Any, Any, Any, Any,      // push    offset method_compare
                0x6A, 0x04,                    // push    4
                0x6A, 0x12,                    // push    12h
                0x8D, 0x44, 0x24, Any,         // lea     eax, [esp+0x1C]
                0x68, Cap<Any>, Any, Any, Any, // push    offset pkey_methods
                0x50,                          // push    eax
                0xE8, Any, Any, Any, Any,      // call    OBJ_bsearch_
                0x83, 0xC4, 0x14,              // add     esp, 14h
                0x85, 0xC0                     // test    eax, eax
                >();

    static inline constexpr auto PAT_FP =
        Pattern<0x56,                          // push    esi
                0x8B, 0x74, 0x24, 0x08,        // mov     dword ptr esi, [esp+4+4]
                0xB8, Cap<Any>, Any, Any, Any, // mov     eax, offset fileproviders_list
                0x33, 0xC9,                    // xor     ecx, ecx
                0x0F, 0x1F, 0x40, 0x00         // nop     dword ptr [eax+00h]
                >();
}

void ModOverlay::save(char const *filename) const noexcept {
    if (FILE *file = fopen(filename, "w"); file) {
        fprintf(file, SCHEMA, checksum, off_fp, off_pmeth);
        fclose(file);
    }
}

void ModOverlay::load(char const *filename) noexcept {
    if (FILE *file = fopen(filename, "r"); file) {
        if (fscanf(file, SCHEMA, &checksum, &off_fp, &off_pmeth) != 3) {
            checksum = 0;
            off_fp = 0;
            off_pmeth = 0;
        }
        fclose(file);
    }
}

std::string ModOverlay::to_string() const noexcept {
    char buffer[sizeof(SCHEMA) * 2];
    int size = sprintf(buffer, SCHEMA, checksum, off_fp, off_pmeth);
    return std::string(buffer, (size_t)size);
}

void ModOverlay::from_string(std::string const &buffer) noexcept {
    if (sscanf(buffer.c_str(), SCHEMA, &checksum, &off_fp, &off_pmeth) != 3) {
        checksum = 0;
        off_fp = 0;
        off_pmeth = 0;
    }
}

bool ModOverlay::check(LCS::Process const &process) const {
    return checksum && off_fp && off_pmeth && process.Checksum() == checksum;
}

void ModOverlay::scan(LCS::Process const &process) {
    auto data = process.Dump();
    auto res_pmeth = PAT_PMETH(data.data(), data.size());
    auto res_fp = PAT_FP(data.data(), data.size());
    if (!res_pmeth[0] || !res_fp[0]) {
        throw std::runtime_error("Failed to find offsets");
    }
    off_pmeth = process.Debase(*(PtrStorage const *)(res_pmeth[1]));
    off_fp = process.Debase(*(PtrStorage const *)(res_fp[1]));
    checksum = process.Checksum();
}

void ModOverlay::wait_patchable(const Process &process, uint32_t timeout) {
    process.WaitNonZero(process.Rebase(off_pmeth), 1, timeout);
}

namespace {
    struct CodePayload {
        uint8_t Verify[0x10] = {
            0xB8, 0x01, 0x00, 0x00, 0x00, 0xC3, 0x90, 0x90,
            0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
        };

        uint8_t PrefixFn[0x30] = {
            0x57, 0x56, 0x8b, 0x54, 0x24, 0x0c, 0x8b, 0x74, 0x24, 0x14, 0x89, 0xd7,
            0xac, 0xaa, 0x84, 0xc0, 0x75, 0xfa, 0x8b, 0x74, 0x24, 0x10, 0x83, 0xef,
            0x01, 0xac, 0xaa, 0x84, 0xc0, 0x75, 0xfa, 0x5e, 0x89, 0xd0, 0x5f, 0xc3,
            0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
        };

        uint8_t Open[0x50] = {
            0x56, 0x53, 0x81, 0xec, 0x14, 0x02, 0x00, 0x00, 0x8b, 0x41, 0x04, 0x8b, 0x58, 0x08,
            0x8b, 0x03, 0x8b, 0x30, 0x8d, 0x41, 0x0c, 0x89, 0x44, 0x24, 0x08, 0x8b, 0x84, 0x24,
            0x20, 0x02, 0x00, 0x00, 0x89, 0x44, 0x24, 0x04, 0x8d, 0x44, 0x24, 0x10, 0x89, 0x04,
            0x24, 0xff, 0x51, 0x08, 0x8b, 0x94, 0x24, 0x24, 0x02, 0x00, 0x00, 0x89, 0xd9, 0x89,
            0x04, 0x24, 0x89, 0x54, 0x24, 0x04, 0xff, 0xd6, 0x83, 0xec, 0x08, 0x81, 0xc4, 0x14,
            0x02, 0x00, 0x00, 0x5b, 0x5e, 0xc2, 0x08, 0x00, 0x90, 0x90,
        };

        uint8_t CheckAccess[0x50] = {
            0x56, 0x53, 0x81, 0xec, 0x14, 0x02, 0x00, 0x00, 0x8b, 0x41, 0x04, 0x8b, 0x58, 0x08,
            0x8b, 0x03, 0x8b, 0x70, 0x04, 0x8d, 0x41, 0x0c, 0x89, 0x44, 0x24, 0x08, 0x8b, 0x84,
            0x24, 0x20, 0x02, 0x00, 0x00, 0x89, 0x44, 0x24, 0x04, 0x8d, 0x44, 0x24, 0x10, 0x89,
            0x04, 0x24, 0xff, 0x51, 0x08, 0x8b, 0x94, 0x24, 0x24, 0x02, 0x00, 0x00, 0x89, 0xd9,
            0x89, 0x04, 0x24, 0x89, 0x54, 0x24, 0x04, 0xff, 0xd6, 0x83, 0xec, 0x08, 0x81, 0xc4,
            0x14, 0x02, 0x00, 0x00, 0x5b, 0x5e, 0xc2, 0x08, 0x00, 0x90,
        };

        uint8_t CreateIterator[0x10] = {
            0x31, 0xc0, 0xc2, 0x08, 0x00, 0x90, 0x90, 0x90,
            0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
        };

        uint8_t VectorDeleter[0x10] = {
            0x89, 0xc8, 0xc2, 0x04, 0x00, 0x90, 0x90, 0x90,
            0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
        };

        uint8_t IsRads[0x10] = {
            0x31, 0xc0, 0xc3, 0x90, 0x90, 0x90, 0x90, 0x90,
            0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
        };

        char Info[0x10] = {"Custom LoL skin"};
    };

    struct EVP_PKEY_METHOD {
        int pkey_id;
        int flags;
        Ptr<void> init;
        Ptr<void> copy;
        Ptr<void> cleanup;
        Ptr<void> paramgen_init;
        Ptr<void> paramgen;
        Ptr<void> keygen_init;
        Ptr<void> keygen;
        Ptr<void> sign_init;
        Ptr<void> sign;
        Ptr<void> verify_init;
        Ptr<uint8_t> verify;
        Ptr<void> verify_recover_init;
        Ptr<void> verify_recover;
        Ptr<void> signctx_init;
        Ptr<void> signctx;
        Ptr<void> verifyctx_init;
        Ptr<void> verifyctx;
        Ptr<void> encrypt_init;
        Ptr<void> encrypt;
        Ptr<void> decrypt_init;
        Ptr<void> decrypt;
        Ptr<void> derive_init;
        Ptr<void> derive;
        Ptr<void> ctrl;
        Ptr<void> ctrl_str;
        Ptr<void> digestsign;
        Ptr<void> digestverify;
        Ptr<void> check;
        Ptr<void> public_check;
        Ptr<void> param_check;
        Ptr<void> digest_custom;
    };

    struct FileProvider {
        struct Vtable {
            Ptr<uint8_t> Open;
            Ptr<uint8_t> CheckAccess;
            Ptr<uint8_t> CreateIterator;
            Ptr<uint8_t> VectorDeleter;
            Ptr<uint8_t> IsRads;
        };
        struct List {
            std::array<Ptr<FileProvider>, 4> arr;
            uint32_t size;
        };
        Ptr<Vtable> vtable;
        Ptr<List> list;
        Ptr<uint8_t> prefixFn;
        std::array<char, 256> prefix;
        char Info[0x10] = {"Custom LoL skin"};
    };
}

void ModOverlay::patch(LCS::Process const &process, std::string_view prefix) const {
    if (!check(process)) {
        throw std::runtime_error("Config invalid to patch this executable!");
    }

    // Copy prefix into array and fix missing trailing /
    std::array<char, 256> prefix_data = {};
    if (!prefix.empty()) {
        strncat(prefix_data.data(), prefix.data(), prefix.size());
        if (prefix.back() != '\\' && prefix.back() != '/') {
            strncat(prefix_data.data(), "/", 1);
        }
    } else {
        prefix_data = {"MOD/"};
    }

    // Declare remote pointer types
    Ptr<Ptr<EVP_PKEY_METHOD>> org_pmeth_arr;
    Ptr<EVP_PKEY_METHOD> org_pmeth;
    Ptr<FileProvider::List> org_fp_arr;
    Ptr<CodePayload> mod_code;
    Ptr<EVP_PKEY_METHOD> mod_pmeth;
    Ptr<FileProvider::Vtable> mod_fp_vtable;
    Ptr<FileProvider> mod_fp;

    // Rebase offsets
    org_pmeth_arr = process.Rebase(off_pmeth);
    org_fp_arr = process.Rebase(off_fp);

    // Allocate
    mod_code = process.Allocate<CodePayload>();
    mod_pmeth = process.Allocate<EVP_PKEY_METHOD>();
    mod_fp_vtable = process.Allocate<FileProvider::Vtable>();
    mod_fp = process.Allocate<FileProvider>();

    // Write code payload
    process.Write(mod_code, CodePayload{});
    process.MarkExecutable(mod_code);

    // Write FileProvider
    process.Write(mod_fp, FileProvider{
                              mod_fp_vtable,
                              org_fp_arr,
                              &mod_code->PrefixFn[0],
                              prefix_data,
                          });
    process.Write(mod_fp_vtable, FileProvider::Vtable{
                                     mod_code->Open,
                                     mod_code->CheckAccess,
                                     mod_code->CreateIterator,
                                     mod_code->VectorDeleter,
                                     mod_code->IsRads,
                                 });

    // Read original PMETH, patch, and write
    process.Read(org_pmeth_arr, org_pmeth);
    EVP_PKEY_METHOD copy_pmeth;
    process.Read(org_pmeth, copy_pmeth);
    copy_pmeth.verify = &mod_code->Verify[0];
    process.Write(mod_pmeth, copy_pmeth);
    process.Write(org_pmeth_arr, mod_pmeth);

    // Read original FileProvider List, append, and write
    FileProvider::List copy_fp_arr;
    process.Read(org_fp_arr, copy_fp_arr);
    process.Write(org_fp_arr, FileProvider::List{
                                  std::array{
                                      mod_fp,
                                      copy_fp_arr.arr[0],
                                      copy_fp_arr.arr[1],
                                      copy_fp_arr.arr[2],
                                  },
                                  copy_fp_arr.size + 1,
                              });
}

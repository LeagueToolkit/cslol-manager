#pragma once
#include <cstdint>
#include <cstddef>
#include "patscanner.hpp"

static struct Shellcode {
    /*
    uint32_t __thiscall Open(FileProvider* this,
                             char const* src, uint32_t mode) {
        char buffer[260];
        buffer[0] = 'M';
        buffer[1] = 'O';
        buffer[2] = 'D';
        buffer[3] = '/';
        for(char * dst = &buffer[4]; *dst++ = *src++;); // strcpy
        auto const org = this->org->list[2];
        return org->vtable->Open(org, buffer, mode);
    }
    */
    uint8_t const Open[0x50] = {
        0x55,0x8B,0xEC,0x81,0xEC,0x04,0x01,0x00,
        0x00,0x56,0x8B,0x75,0x08,0x8D,0x95,0x00,
        0xFF,0xFF,0xFF,0xC7,0x85,0xFC,0xFE,0xFF,
        0xFF,0x4D,0x4F,0x44,0x2F,0x8A,0x06,0x46,
        0x88,0x02,0x42,0x84,0xC0,0x75,0xF6,0x8B,
        0x41,0x04,0x8D,0x95,0xFC,0xFE,0xFF,0xFF,
        0xFF,0x75,0x0C,0x52,0x8B,0x48,0x08,0x8B,
        0x01,0x8B,0x00,0xFF,0xD0,0x5E,0xC9,0xC2,
        0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    };

    /*
    uint32_t __thiscall CheckAccess(FileProvider* this,
                                    char const* src, uint32_t mode) {
        char buffer[260];
        buffer[0] = 'M';
        buffer[1] = 'O';
        buffer[2] = 'D';
        buffer[3] = '/';
        for(char * dst = &buffer[4]; *dst++ = *src++;); // strcpy
        auto const org = this->org->list[2];
        return org->vtable->CheckAccess(org, buffer, mode);
    }
    */
    uint8_t const CheckAccess[0x50] = {
        0x55,0x8B,0xEC,0x81,0xEC,0x04,0x01,0x00,
        0x00,0x56,0x8B,0x75,0x08,0x8D,0x95,0x00,
        0xFF,0xFF,0xFF,0xC7,0x85,0xFC,0xFE,0xFF,
        0xFF,0x4D,0x4F,0x44,0x2F,0x8A,0x06,0x46,
        0x88,0x02,0x42,0x84,0xC0,0x75,0xF6,0x8B,
        0x41,0x04,0x8D,0x95,0xFC,0xFE,0xFF,0xFF,
        0xFF,0x75,0x0C,0x52,0x8B,0x48,0x08,0x8B,
        0x01,0x8B,0x40,0x04,0xFF,0xD0,0x5E,0xC9,
        0xC2,0x08,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    };

    /*
    uint32_t __thiscall CreateIterator(FileProvider* this,
                                       char const*, uint32_t) {
        return 0;
    }
    */
    uint8_t const CreateIterator[0x10] = {
        0x33,0xC0,0xC2,0x08,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    };

    /*
    uint32_t __thiscall VectorDeleter(FileProvider*, uint32_t) {
        return 0;
    }
    */
    uint8_t const VectorDeleter[0x10] = {
        0x33,0xC0,0xC2,0x04,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    };

    /*
    uint32_t __thiscall VectorDeleter(FileProvider*) {
        return 0;
    }
    */
    uint8_t const Destructor[0x10] = {
        0x32,0xC0,0xC3,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    };

    /*
    void* __cdecl Verify() {
        return 0;
    }
    */
    uint8_t const Verify[0x10] = {
        0xB8,0x01,0x00,0x00,0x00,0xC3,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    };
    char const Info[0x10] = {
        'C' ,'u' ,'s' ,'t' ,'o' ,'m' ,' ' ,'L' ,
        'o' ,'l' ,' ' ,'s' , 'k','i' ,'n' ,'\0',
    };
} const shellcode = {};


struct EVP_PKEY_METHOD {
    uint32_t pkey_id;
    uint32_t flags;
    uint32_t init;
    uint32_t copy;
    uint32_t cleanup;
    uint32_t paramgen_init;
    uint32_t paramgen;
    uint32_t keygen_init;
    uint32_t keygen;
    uint32_t sign_init;
    uint32_t sign;
    uint32_t verify_init;
    uint8_t const* verify;
    uint32_t verify_recover_init;
    uint32_t verify_recover;
    uint32_t signctx_init;
    uint32_t signctx;
    uint32_t verifyctx_init;
    uint32_t verifyctx;
    uint32_t encrypt_init;
    uint32_t encrypt;
    uint32_t decrypt_init;
    uint32_t decrypt;
    uint32_t derive_init;
    uint32_t derive;
    uint32_t ctrl;
    uint32_t ctrl_str;
    uint32_t digestsign;
    uint32_t digestverify;
    uint32_t check;
    uint32_t public_check;
    uint32_t param_check;
    uint32_t digest_custom;
};

struct FileProvider {
    struct Vtable {
        uint8_t const* Open;
        uint8_t const* CheckAccess;
        uint8_t const* CreateIterator;
        uint8_t const* VectorDeleter;
        uint8_t const* Destructor;
    } *vtable;
    struct List {
        FileProvider* arr[4];
        uint32_t size;
    } *list;
    char const Info[0x10] = {
        'C' ,'u' ,'s' ,'t' ,'o' ,'m' ,' ' ,'L' ,
        'o' ,'l' ,' ' ,'s' , 'k','i' ,'n' ,'\0',
    };
};

struct Data {
    FileProvider fp;
    FileProvider::Vtable fpvtbl;
    EVP_PKEY_METHOD pmeth;
};

static char const * const classnames[] = {
    "League of Legends (TM) Client",
    // TODO: do chinese, jp, KR clients have english class names as well ?
};

inline constexpr auto pmeth_pat = Pattern<
    0x68, Any, Any, Any, Any,   // push    offset method_compare
    0x6A, 0x04,                 // push    4
    0x6A, 0x12,                 // push    12h
    0x8D, 0x44, 0x24, Any,      // lea     eax, [esp+0x1C]
    0x68, Cap<4>,               // push    offset pkey_methods
    0x50,                       // push    eax
    0xE8, Any, Any, Any, Any,   // call    OBJ_bsearch_
    0x83, 0xC4, 0x14,           // add     esp, 14h
    0x85, 0xC0                  // test    eax, eax
>{};

inline constexpr auto fp_pat = Pattern<
        0x56,                   // push    esi
        0x8B, 0x74, 0x24, 0x08, // mov     dword ptr esi, [esp+4+4]
        0xB8, Cap<4>,           // mov     eax, offset fileproviders_list
        0x33, 0xC9,             // xor     ecx, ecx
        0x0F, 0x1F, 0x40, 0x00  // nop     dword ptr [eax+00h]
>{};


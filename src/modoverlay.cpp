#define _CRT_SECURE_NO_WARNINGS
#include "modoverlay.hpp"
#include <array>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <span>

using namespace LCS;

char const ModOverlay::SCHEMA[] = "lcs-overlay v2 0x%08X 0x%08X 0x%08X 0x%08X";
char const ModOverlay::INFO[] = "lcs-overlay v2 checksum off_rsa_meth off_open off_open_ref";

namespace {
    constexpr char const* Find(std::span<const char> data, std::string_view what) noexcept {
        auto result = std::search(data.data(), data.data() + data.size(),
                                  what.data(), what.data() + what.size());
        if (result != data.data() + data.size()) {
            return result;
        }
        return nullptr;
    }

    constexpr char const* FindAfter(std::span<const char> data, std::string_view what) noexcept {
        auto result = Find(data, what);
        if (result) {
            return result + what.size();
        }
        return nullptr;
    }

    inline constexpr uint16_t Any = 0x0100u;

    // capture offset of given byte
    template <uint16_t C = Any>
    inline constexpr uint16_t Cap = C | 0x0200u;

    // returns function that searches for the given signature
    // result of search is array with first offset + any offsets captured
    template <uint16_t... ops>
    inline constexpr auto Pattern() {
        return [](std::span<const char> data) constexpr noexcept {
            std::array<char const *, ((ops & 0x0200 ? 1 : 0) + ... + 1)> result = {};
            if (data.size() >= sizeof...(ops)) {
                auto const end = data.data() + (data.size() - sizeof...(ops));
                for (auto *i = data.data(); i != end; i++) {
                    auto c = i;
                    if ((((uint8_t)*c++ == (ops & 0xFF) || (ops & 0x100)) && ...)) {
                        auto *o = i;
                        size_t r = 0;
                        result[r++] = o;
                        ((ops & 0x200 ? result[r++] = o++ : o++), ...);
                        return result;
                    }
                }
            }
            return result;
        };
    }

    constexpr auto FindOpen = Pattern<
            0x6A, 0x03,
            0x68, 0x00, 0x00, 0x00, 0xC0,
            0x68, Any, Any, Any, Any,
            0xFF, 0x15, Cap<Any>, Any, Any, Any
    >();
}

void ModOverlay::save(char const *filename) const noexcept {
    if (FILE *file = fopen(filename, "w"); file) {
        fprintf(file, SCHEMA, checksum, off_rsa_meth, off_open, off_open_ref);
        fclose(file);
    }
}

void ModOverlay::load(char const *filename) noexcept {
    if (FILE *file = fopen(filename, "r"); file) {
        if (fscanf(file, SCHEMA, &checksum, &off_rsa_meth, &off_open, &off_open_ref) != 4) {
            checksum = 0;
            off_rsa_meth = 0;
            off_open = 0;
            off_open_ref = 0;
        }
        fclose(file);
    }
}

std::string ModOverlay::to_string() const noexcept {
    char buffer[sizeof(SCHEMA) * 2] = {};
    int size = sprintf(buffer, SCHEMA, checksum, off_rsa_meth, off_open, off_open_ref);
    return std::string(buffer, (size_t)size);
}

void ModOverlay::from_string(std::string const &buffer) noexcept {
    if (sscanf(buffer.c_str(), SCHEMA, &checksum, &off_rsa_meth, &off_open, &off_open_ref) != 4) {
        checksum = 0;
        off_rsa_meth = 0;
        off_open = 0;
        off_open_ref = 0;
    }
}

bool ModOverlay::check(LCS::Process const &process) const {
    return checksum && off_rsa_meth && off_open && off_open_ref && process.Checksum() == checksum;
}

void ModOverlay::scan(LCS::Process const &process) {
    auto data = process.Dump();

    // Find rsa method name ptr
    auto rsa_meth_name_offset = Find(data, "OpenSSL PKCS#1 RSA");
    if (!rsa_meth_name_offset) {
        throw std::runtime_error("Failed to find RSA method name!");
    }
    auto rsa_meth_name_ptr = process.Rebase((PtrStorage)(rsa_meth_name_offset - data.data()));

    // Convert pointer to bytes
    char rsa_meth_name_ptr_bytes[sizeof(PtrStorage)];
    std::memcpy(&rsa_meth_name_ptr_bytes, &rsa_meth_name_ptr, sizeof(PtrStorage));

    // Find rsa method table
    auto res_rsa_meth = Find(data, {rsa_meth_name_ptr_bytes, sizeof(rsa_meth_name_ptr_bytes)});
    if (!res_rsa_meth) {
        throw std::runtime_error("Failed to find RSA method offset!");
    }
    off_rsa_meth = (PtrStorage)(res_rsa_meth - data.data());

    // Find call to fopen
    auto open_call_offset = FindOpen(data);
    if (!open_call_offset[0]) {
        throw std::runtime_error("Failed to find fopen call!");
    }

    // Save where we found our pointer
    off_open_ref = (PtrStorage)(open_call_offset[1] - data.data());

    // Convert bytes to pointer
    PtrStorage open_ptr;
    std::memcpy(&open_ptr, open_call_offset[1], sizeof(open_ptr));
    off_open = process.Debase(open_ptr);


    checksum = process.Checksum();
}

namespace {
    struct alignas(64) Trampoline {
        std::array<uint8_t, 8> mov = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0xB8 };
        Ptr<uint8_t> where = {};
        std::array<uint8_t, 8> jmp = { 0xff, 0xE0, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };
    };

    struct CodePayload {
        uint8_t verify[0x8] = {
            0xB8, 0x01, 0x00, 0x00, 0x00, 0xC3, 0x90, 0x90,
        };
        uint8_t open[0x78] = {
            0xC8, 0x00, 0x10, 0x00, 0x53, 0x57, 0x56, 0xE8,
            0x00, 0x00, 0x00, 0x00, 0x5B, 0x83, 0xE3, 0x80,
            0x81, 0xC3, 0x80, 0x00, 0x00, 0x00, 0x8D, 0xBD,
            0x00, 0xF0, 0xFF, 0xFF, 0x8D, 0x73, 0x40, 0xAC,
            0xAA, 0x84, 0xC0, 0x75, 0xFA, 0x4F, 0x8B, 0x75,
            0x08, 0xAC, 0xAA, 0x84, 0xC0, 0x75, 0xFA, 0xFF,
            0x75, 0x20, 0xFF, 0x75, 0x1C, 0xFF, 0x75, 0x18,
            0xFF, 0x75, 0x14, 0xFF, 0x75, 0x10, 0xFF, 0x75,
            0x0C, 0x8D, 0x85, 0x00, 0xF0, 0xFF, 0xFF, 0x50,
            0x8D, 0x03, 0xFF, 0xD0, 0x83, 0xF8, 0xFF, 0x75,
            0x19, 0xFF, 0x75, 0x20, 0xFF, 0x75, 0x1C, 0xFF,
            0x75, 0x18, 0xFF, 0x75, 0x14, 0xFF, 0x75, 0x10,
            0xFF, 0x75, 0x0C, 0xFF, 0x75, 0x08, 0x8D, 0x03,
            0xFF, 0xD0, 0x5E, 0x5F, 0x5B, 0xC9, 0xC2, 0x1C,
            0x00
        };
        Trampoline original = {};
        std::array<char, 0x100> prefix = { "MOD\\" };
    };

    struct RSA_METHOD {
        Ptr<void> name;
        Ptr<void> rsa_pub_enc;
        Ptr<void> rsa_pub_dec;
        Ptr<void> rsa_priv_enc;
        Ptr<void> rsa_priv_dec;
        Ptr<void> rsa_mod_exp;
        Ptr<void> bn_mod_exp;
        Ptr<void> init;
        Ptr<void> finish;
        int flags;
        Ptr<void> app_data;
        Ptr<void> rsa_sign;
        Ptr<uint8_t> rsa_verify;
        Ptr<void> rsa_keygen;
        Ptr<void> rsa_multi_prime_keygen;
    };
}

void ModOverlay::wait_patchable(const Process &process, uint32_t timeout) {
    auto ptr_open = process.Rebase(off_open);
    auto ptr_load = process.Rebase(off_open_ref);
    process.WaitPtrEq(ptr_load, ptr_open, 1, timeout);
}

void ModOverlay::patch(LCS::Process const &process, std::string prefix) const {
    if (!check(process)) {
        throw std::runtime_error("Config invalid to patch this executable!");
    }
    if (prefix.empty()) {
        prefix = "MOD/";
    }
    if (prefix.back() != '\\' && prefix.back() != '/') {
        prefix.push_back('/');
    }
    if (prefix.size() >= sizeof(CodePayload::prefix)) {
        throw std::runtime_error("Prefix too big!");
    }

    auto payload = CodePayload {};
    auto trampoline = Trampoline{};

    // Prepare pointers
    auto rsa_meth = Ptr<RSA_METHOD>(process.Rebase(off_rsa_meth));
    auto ptr_open = Ptr<Ptr<Trampoline>>(process.Rebase(off_open));
    auto mod_code = process.Allocate<CodePayload>();
    auto org_open = Ptr<Trampoline>{};
    process.Read(ptr_open, org_open);

    process.Read(org_open, payload.original);
    std::copy_n(prefix.data(), prefix.size(), payload.prefix.data());
    trampoline.where = mod_code->open;

    // Write payload
    process.Write(mod_code, payload);
    process.MarkExecutable(mod_code);

    // Write hooks
    process.Write(Ptr(&rsa_meth->rsa_verify), Ptr(mod_code->verify));
    process.Write(org_open, trampoline);
}

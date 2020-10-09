#define _CRT_SECURE_NO_WARNINGS
#include "modoverlay.hpp"
#include <array>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <span>

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

    constexpr auto FindRet = Pattern<
            0xE8, Any, Any, Any, Any,
            Any,
            0xE8, Any, Any, Any, Any,
            Cap<0x83>, 0xC4, 0x08,
            0x83, 0xFE, 0x01,
            0x75, 0x04,
            0xB0, 0x01
    >();

    constexpr auto FindFree = Pattern<
            0xA1, Cap<Any>, Any, Any, Any,
            0x85, 0xC0,
            0x74, 0x09,
            0x3D, Any, Any, Any, Any,
            0x74, 0x02,
            0xFF, 0xE0,
            Cap<0xFF>, 0x74, 0x24, 0x04,
            0xE8, Any, Any, Any, Any
            >();

    inline PtrStorage bytes2ptr(char const* bytes) {
        PtrStorage result;
        std::memcpy(&result, bytes, sizeof(result));
        return result;
    }
}

void ModOverlay::save(char const *filename) const noexcept {
    if (FILE *file = fopen(filename, "wb"); file) {
        auto str = to_string();
        fwrite(str.data(), 1, str.size(), file);
        fclose(file);
    }
}

void ModOverlay::load(char const *filename) noexcept {
    if (FILE *file = fopen(filename, "rb"); file) {
        auto buffer = std::string();
        fseek(file, 0, SEEK_END);
        auto end = ftell(file);
        fseek(file, 0, SEEK_SET);
        buffer.resize((size_t)end);
        fread(buffer.data(), 1, buffer.size(), file);
        from_string(buffer);
    }
}

char const ModOverlay::SCHEMA[] = "lcs-overlay v3 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X";
char const ModOverlay::INFO[] = "lcs-overlay v3 checksum off_rsa_meth off_open off_open_ref off_free_ptr off_free_fn";

std::string ModOverlay::to_string() const noexcept {
    char buffer[sizeof(SCHEMA) * 2] = {};
    int size = sprintf(buffer, SCHEMA, checksum, off_open, off_open_ref, off_ret, off_free_ptr, off_free_fn);
    return std::string(buffer, (size_t)size);
}

void ModOverlay::from_string(std::string const &buffer) noexcept {
    if (sscanf(buffer.c_str(), SCHEMA, &checksum, &off_open, &off_open_ref, &off_ret, &off_free_ptr, &off_free_fn) != 6) {
        checksum = 0;
        off_open = 0;
        off_open_ref = 0;
        off_ret = 0;
        off_free_ptr = 0;
        off_free_fn = 0;
    }
}

bool ModOverlay::check(LCS::Process const &process) const {
    return checksum && off_open && off_open_ref && off_ret && off_free_ptr && off_free_fn && process.Checksum() == checksum;
}

void ModOverlay::scan(LCS::Process const &process) {
    auto data = process.Dump();

    // Find call to fopen
    auto open_call_offset = FindOpen(data);
    if (!open_call_offset[0]) {
        throw std::runtime_error("Failed to find fopen call!");
    }

    off_open_ref = (PtrStorage)(open_call_offset[1] - data.data());
    off_open = process.Debase(bytes2ptr(open_call_offset[1]));

    // Find ret
    auto ret_offset = FindRet(data);
    if (!ret_offset[0]) {
        throw std::runtime_error("Failed to find ret offset!");
    }
    off_ret = (PtrStorage)(ret_offset[1] - data.data());

    // Find EVP_PKEY_METHOD array
    auto free_ptr_offset = FindFree(data);
    if (!free_ptr_offset[0]) {
        throw std::runtime_error("Failed to find pmeth arr offset!");
    }

    off_free_ptr = process.Debase(bytes2ptr(free_ptr_offset[1]));
    off_free_fn = (PtrStorage)(free_ptr_offset[2] - data.data());

    checksum = process.Checksum();
}

namespace {
    struct alignas(64) Trampoline {
        std::array<uint8_t, 8> mov = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0xB8 };
        Ptr<uint8_t> where = {};
        std::array<uint8_t, 8> jmp = { 0xff, 0xE0, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };
    };

    struct CodePayload {
        uint8_t free[0x80] = {
            0xC8, 0x00, 0x00, 0x00, 0x53, 0x56, 0xE8, 0x00,
            0x00, 0x00, 0x00, 0x5B, 0x81, 0xE3, 0x00, 0xF0,
            0xFF, 0xFF, 0x81, 0xC3, 0x00, 0x01, 0x00, 0x00,
            0x8B, 0x73, 0x44, 0x89, 0xE8, 0x83, 0xC0, 0x2C,
            0x83, 0xE8, 0x04, 0x39, 0xE8, 0x74, 0x0D, 0x3B,
            0x30, 0x75, 0xF5, 0x8D, 0x40, 0xFC, 0xC7, 0x00,
            0x01, 0x00, 0x00, 0x00, 0x8B, 0x43, 0x40, 0x5E,
            0x5B, 0xC9, 0xFF, 0xE0
        };
        uint8_t open[0x80] = {
            0xC8, 0x00, 0x10, 0x00, 0x53, 0x57, 0x56, 0xE8,
            0x00, 0x00, 0x00, 0x00, 0x5B, 0x81, 0xE3, 0x00,
            0xF0, 0xFF, 0xFF, 0x81, 0xC3, 0x00, 0x01, 0x00,
            0x00, 0x8D, 0xBD, 0x00, 0xF0, 0xFF, 0xFF, 0x8D,
            0x73, 0x48, 0xAC, 0xAA, 0x84, 0xC0, 0x75, 0xFA,
            0x4F, 0x8B, 0x75, 0x08, 0xAC, 0xAA, 0x84, 0xC0,
            0x75, 0xFA, 0xFF, 0x75, 0x20, 0xFF, 0x75, 0x1C,
            0xFF, 0x75, 0x18, 0xFF, 0x75, 0x14, 0xFF, 0x75,
            0x10, 0xFF, 0x75, 0x0C, 0x8D, 0x85, 0x00, 0xF0,
            0xFF, 0xFF, 0x50, 0x8D, 0x03, 0xFF, 0xD0, 0x83,
            0xF8, 0xFF, 0x75, 0x19, 0xFF, 0x75, 0x20, 0xFF,
            0x75, 0x1C, 0xFF, 0x75, 0x18, 0xFF, 0x75, 0x14,
            0xFF, 0x75, 0x10, 0xFF, 0x75, 0x0C, 0xFF, 0x75,
            0x08, 0x8D, 0x03, 0xFF, 0xD0, 0x5E, 0x5F, 0x5B,
            0xC9, 0xC2, 0x1C, 0x00
        };
        Trampoline original = {};
        Ptr<uint8_t> org_free = {};
        PtrStorage ret = {};
        std::array<char, 0x100> prefix = { "MOD\\" };
    };
}

void ModOverlay::wait_patchable(const Process &process, uint32_t timeout) {
    auto ptr_open = process.Rebase(off_open);
    auto ptr_open_ref = process.Rebase(off_open_ref);
    process.WaitPtrEq(ptr_open_ref, PtrStorage(ptr_open), 1, timeout);
    auto ptr_free = process.Rebase(off_free_ptr);
    process.WaitPtrNotEq(ptr_free, PtrStorage(0), 1, timeout);
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
    auto mod_code = process.Allocate<CodePayload>();
    auto ptr_open = Ptr<Ptr<Trampoline>>(process.Rebase(off_open));
    auto org_open = Ptr<Trampoline>{};
    process.Read(ptr_open, org_open);
    auto ret = process.Rebase(off_ret);
    auto ptr_free = Ptr<Ptr<uint8_t>>(process.Rebase(off_free_ptr));
    auto org_free = Ptr<uint8_t>(process.Rebase(off_free_fn));

    // Prepare payload
    process.Read(org_open, payload.original);
    payload.org_free = org_free;
    payload.ret = ret;
    std::copy_n(prefix.data(), prefix.size(), payload.prefix.data());

    // Prepare trampoline
    trampoline.where = mod_code->open;

    // Write payload
    process.Write(mod_code, payload);
    process.MarkExecutable(mod_code);

    // Write hooks
    process.Write(ptr_free, Ptr(mod_code->free));
    process.Write(org_open, trampoline);
}

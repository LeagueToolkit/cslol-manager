#include "modoverlay.hpp"
#include <array>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <span>

using namespace LCS;
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
}

void ModOverlay::save(char const *filename) const noexcept {
    if (FILE *file = fopen(filename, "w"); file) {
        fprintf(file, SCHEMA, checksum, off_rsa_meth, off_unused);
        fclose(file);
    }
}

void ModOverlay::load(char const *filename) noexcept {
    if (FILE *file = fopen(filename, "r"); file) {
        if (fscanf(file, SCHEMA, &checksum, &off_rsa_meth, &off_unused) != 3) {
            checksum = 0;
            off_rsa_meth = 0;
            off_unused = 0;
        }
        fclose(file);
    }
}

std::string ModOverlay::to_string() const noexcept {
    char buffer[sizeof(SCHEMA) * 2] = {};
    int size = sprintf(buffer, SCHEMA, checksum, off_rsa_meth, off_unused);
    return std::string(buffer, (size_t)size);
}

void ModOverlay::from_string(std::string const &buffer) noexcept {
    if (sscanf(buffer.c_str(), SCHEMA, &checksum, &off_rsa_meth, &off_unused) != 3) {
        checksum = 0;
        off_rsa_meth = 0;
        off_unused = 0;
    }
}

bool ModOverlay::check(LCS::Process const &process) const {
    return checksum && off_rsa_meth && process.Checksum() == checksum;
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
    off_unused = 0;
    checksum = process.Checksum();
}

namespace {
    struct CodePayload {
        uint8_t Verify[0x10] = {
            0xB8, 0x01, 0x00, 0x00, 0x00, 0xC3, 0x90, 0x90,
            0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
        };
        char Info[0x10] = {"Custom LoL skin"};
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
    process.WaitNonZero(process.Rebase(off_rsa_meth), 1, timeout);
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

    // Declare remote pointer types
    auto org_rsa_meth = Ptr<RSA_METHOD>(process.Rebase(off_rsa_meth));
    auto mod_code = process.Allocate<CodePayload>();
    process.Write(mod_code, CodePayload{});
    process.MarkExecutable(mod_code);
    process.Write(Ptr(&org_rsa_meth->rsa_verify), Ptr(&mod_code->Verify[0]));

    // TODO: hook CreateFileW and OpenFile
}

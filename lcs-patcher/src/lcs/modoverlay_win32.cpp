#define _CRT_SECURE_NO_WARNINGS
#include "modoverlay.hpp"
#include "process.hpp"
#include "ppp.hpp"
#include <array>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <span>
#include <filesystem>

using namespace LCS;

constexpr auto const find_open = &ppp::any<
        "6A 03 68 00 00 00 00 C0 68 ?? ?? ?? ?? FF 15 o[u[?? ?? ?? ??]]"_pattern,
        "6A 00 56 55 50 FF 15 o[u[?? ?? ?? ??]] 8B F8"_pattern>;
constexpr auto const find_ret = &ppp::any<
        "57 8B CB E8 ?? ?? ?? ?? o[84] C0 75 12"_pattern>;
constexpr auto const find_wopen = &ppp::any<
        "6A 00 6A ?? 68 ?? ?? 12 00 ?? FF 15 u[?? ?? ?? ??]"_pattern>;
constexpr auto const find_free = &ppp::any<
        "A1 u[?? ?? ?? ??] 85 C0 74 09 3D ?? ?? ?? ?? 74 02 FF E0 o[FF 74 24 04] E8 ?? ?? ??"_pattern>;

char const ModOverlay::SCHEMA[] = "lcs-overlay v5 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X";
char const ModOverlay::INFO[] = "lcs-overlay v5 checksum off_open off_open_ref off_wopen off_ret off_free_ptr off_free_fn";

struct ModOverlay::Config {
    uint32_t checksum = {};
    PtrStorage off_open = {};
    PtrStorage off_open_ref = {};
    PtrStorage off_wopen = {};
    PtrStorage off_ret = {};
    PtrStorage off_free_ptr = {};
    PtrStorage off_free_fn = {};

    std::string to_string() const noexcept {
        char buffer[sizeof(SCHEMA) * 4] = {};
        int size = sprintf(buffer, SCHEMA, checksum, off_open, off_open_ref, off_wopen, off_ret, off_free_ptr, off_free_fn);
        return std::string(buffer, (size_t)size);
    }

    void from_string(std::string const &buffer) noexcept {
        if (sscanf(buffer.c_str(), SCHEMA, &checksum, &off_open, &off_open_ref, &off_wopen, &off_ret, &off_free_ptr, &off_free_fn) != 7) {
            checksum = 0;
            off_open = 0;
            off_open_ref = 0;
            off_wopen = 0;
            off_ret = 0;
            off_free_ptr = 0;
            off_free_fn = 0;
        }
    }

    bool check(LCS::Process const &process) const {
        return checksum && off_open && off_open_ref && off_wopen && off_ret && off_free_ptr && off_free_fn && process.Checksum() == checksum;
    }

    void scan(LCS::Process const &process) {
        auto const base = process.Base();
        auto const data = process.Dump();

        auto const open_match = find_open(data, base);
        if (!open_match) {
            throw std::runtime_error("Failed to find fopen!");
        }
        auto const wopen_match = find_wopen(data, base);
        if (!wopen_match) {
            throw std::runtime_error("Failed to find wfopen!");
        }
        auto const ret_match = find_ret(data, base);
        if (!ret_match) {
            throw std::runtime_error("Failed to find ret!");
        }
        auto const free_match = find_free(data, base);
        if (!free_match) {
            throw std::runtime_error("Failed to find free!");
        }

        off_open_ref = process.Debase((PtrStorage)std::get<1>(*open_match));
        off_open = process.Debase((PtrStorage)std::get<2>(*open_match));
        off_wopen = process.Debase((PtrStorage)std::get<1>(*wopen_match));
        off_ret = process.Debase((PtrStorage)std::get<1>(*ret_match));
        off_free_ptr = process.Debase((PtrStorage)std::get<1>(*free_match));
        off_free_fn = process.Debase((PtrStorage)std::get<2>(*free_match));

        checksum = process.Checksum();
    }

    struct ImportTrampoline {
        uint8_t data[64] = {};
        static ImportTrampoline make(Ptr<uint8_t> where) {
            ImportTrampoline result = {
                { 0xB8u, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xE0, }
            };
            memcpy(result.data + 1, &where.storage, sizeof(where.storage));
            return result;
        }
    };

    struct CodePayload {
        // Pointers to be consumed by shellcode
        Ptr<uint8_t> org_open_ptr = {};
        Ptr<char16_t> prefix_open_ptr = {};
        Ptr<uint8_t> wopen_ptr = {};
        Ptr<uint8_t> org_free_ptr = {};
        PtrStorage find_ret_addr = {};
        PtrStorage hook_ret_addr = {};

        // Actual data and shellcode storage
        uint8_t hook_open_data[0x80] = {
            0xc8, 0x00, 0x10, 0x00, 0x53, 0x57, 0x56, 0xe8,
            0x00, 0x00, 0x00, 0x00, 0x5b, 0x81, 0xe3, 0x00,
            0xf0, 0xff, 0xff, 0x8d, 0xbd, 0x00, 0xf0, 0xff,
            0xff, 0x8b, 0x73, 0x04, 0x66, 0xad, 0x66, 0xab,
            0x66, 0x85, 0xc0, 0x75, 0xf7, 0x83, 0xef, 0x02,
            0x8b, 0x75, 0x08, 0xb4, 0x00, 0xac, 0x3c, 0x2f,
            0x75, 0x02, 0xb0, 0x5c, 0x66, 0xab, 0x84, 0xc0,
            0x75, 0xf3, 0x8d, 0x85, 0x00, 0xf0, 0xff, 0xff,
            0xff, 0x75, 0x20, 0xff, 0x75, 0x1c, 0xff, 0x75,
            0x18, 0xff, 0x75, 0x14, 0xff, 0x75, 0x10, 0xff,
            0x75, 0x0c, 0x50, 0xff, 0x53, 0x08, 0x83, 0xf8,
            0xff, 0x75, 0x17, 0xff, 0x75, 0x20, 0xff, 0x75,
            0x1c, 0xff, 0x75, 0x18, 0xff, 0x75, 0x14, 0xff,
            0x75, 0x10, 0xff, 0x75, 0x0c, 0xff, 0x75, 0x08,
            0xff, 0x13, 0x5e, 0x5f, 0x5b, 0xc9, 0xc2, 0x1c,
            0x00
        };
        uint8_t hook_free_data[0x80] = {
            0xc8, 0x00, 0x00, 0x00, 0x53, 0x56, 0xe8, 0x00,
            0x00, 0x00, 0x00, 0x5b, 0x81, 0xe3, 0x00, 0xf0,
            0xff, 0xff, 0x8b, 0x73, 0x10, 0x89, 0xe8, 0x05,
            0x80, 0x01, 0x00, 0x00, 0x83, 0xe8, 0x04, 0x39,
            0xe8, 0x74, 0x09, 0x3b, 0x30, 0x75, 0xf5, 0x8b,
            0x73, 0x14, 0x89, 0x30, 0x8b, 0x43, 0x0c, 0x5e,
            0x5b, 0xc9, 0xff, 0xe0
        };
        ImportTrampoline org_open_data = {};
        char16_t prefix_open_data[0x400] = {};
    };

    bool is_patchable(const Process &process) {
        try {
            auto const ptr_open_ref = process.Rebase<PtrStorage>(off_open_ref);
            if (process.Read(ptr_open_ref) != process.Rebase(off_open)) {
                return false;
            }
            auto const ptr_free = process.Rebase<PtrStorage>(off_free_ptr);
            if (process.Read(ptr_free) == 0) {
                return false;
            }
            auto const ptr_wopen = process.Rebase<PtrStorage>(off_wopen);
            if (process.Read(ptr_wopen) == 0) {
                return false;
            }
        } catch(std::runtime_error const&) {
            return false;
        }
        return true;
    }

    void patch(LCS::Process const &process, std::u16string const& prefix_str) const {
        if (!check(process)) {
            throw std::runtime_error("Config invalid to patch this executable!");
        }
        // Prepare pointers
        auto mod_code = process.Allocate<CodePayload>();
        auto ptr_open = Ptr<Ptr<ImportTrampoline>>(process.Rebase(off_open));
        auto org_open = Ptr<ImportTrampoline>{};
        process.Read(ptr_open, org_open);
        auto ptr_wopen = Ptr<Ptr<uint8_t>>(process.Rebase(off_wopen));
        auto wopen = Ptr<uint8_t>{};
        process.Read(ptr_wopen, wopen);
        auto find_ret_addr = process.Rebase(off_ret);
        auto ptr_free = Ptr<Ptr<uint8_t>>(process.Rebase(off_free_ptr));
        auto org_free_ptr = Ptr<uint8_t>(process.Rebase(off_free_fn));

        // Prepare payload
        auto payload = CodePayload {};
        payload.org_open_ptr = Ptr(mod_code->org_open_data.data);
        payload.prefix_open_ptr = Ptr(mod_code->prefix_open_data);
        payload.wopen_ptr = wopen;
        payload.org_free_ptr = org_free_ptr;
        payload.find_ret_addr = find_ret_addr;
        payload.hook_ret_addr = find_ret_addr + 0x16;

        process.Read(org_open, payload.org_open_data);
        std::copy_n(prefix_str.data(), prefix_str.size(), payload.prefix_open_data);

        // Write payload
        process.Write(mod_code, payload);
        process.MarkExecutable(mod_code);

        // Write hooks
        process.Write(ptr_free, Ptr(mod_code->hook_free_data));
        process.Write(org_open, ImportTrampoline::make(mod_code->hook_open_data));
    }
};

ModOverlay::ModOverlay() : config_(std::make_unique<Config>()) {}

ModOverlay::~ModOverlay() = default;

std::string ModOverlay::to_string() const noexcept {
    config_->to_string();
}

void ModOverlay::from_string(std::string const & str) noexcept {
    config_->from_string(str);
}

void ModOverlay::run(std::function<bool(Message)> update, std::filesystem::path const& profilePath) {
    std::u16string prefix_str = profilePath.generic_u16string();
    for (auto& c: prefix_str) {
        if (c == u'/') {
            c = u'\\';
        }
    }
    prefix_str = u"\\\\?\\" + prefix_str;
    if (!prefix_str.ends_with(u"\\")) {
        prefix_str.push_back(u'\\');
    }
    if ((prefix_str.size() + 1) * sizeof(char16_t) >= sizeof(Config::CodePayload::prefix_open_data)) {
        throw std::runtime_error("Prefix too big!");
    }
    if (!update(M_DONE)) return;
    for (;;) {
        auto process = Process::Find("/LeagueofLegends");
        if (!process) {
            if (!update(M_NONE)) return;
            LCS::SleepMS(125);
            continue;
        }
        if (!update(M_FOUND)) return;
        if (!config_->check(*process)) {
            if (!update(M_WAIT_INIT)) return;
            for (std::uint32_t timeout = 3 * 60 * 1000; timeout; timeout -= 5) {
                if (process->WaitInitialized(5)) {
                    break;
                }
                if (!update(M_NONE)) return;
            }
            if (!update(M_SCAN)) return;
            config_->scan(*process);
            if (!update(M_NEED_SAVE)) return;
        } else {
            if (!update(M_WAIT_PATCHABLE)) return;
            for (std::uint32_t timeout = 3 * 60 * 1000; timeout; timeout -= 5) {
                if (config_->is_patchable()) {
                    break;
                }
                if (!update(M_NONE)) return;
            }
        }
        if (!update(M_PATCH)) return;
        config_->patch(*process, prefix_str);
        for (std::uint32_t timeout = 3 * 60 * 60 * 1000; timeout; timeout -= 250) {
            if (process->WaitExit(250)) {
                break;
            }
            if (!update(M_NONE)) return;
        }
        if (!update(M_DONE)) return;
    }
}


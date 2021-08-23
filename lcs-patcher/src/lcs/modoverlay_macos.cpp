#include "modoverlay.hpp"
#include "macho.hpp"
#include "process.hpp"
#include <cstring>

using namespace LCS;

char const ModOverlay::SCHEMA[] = "lcs-overlay-osx v5 0x%016llX 0x%016llX";
char const ModOverlay::INFO[] = "lcs-overlay-osx v5 off_rsa_verify off_fopen";

struct ModOverlay::Config {
    MachO macho = {};
    std::uint64_t off_rsa_verify = {};
    std::uint64_t off_fopen = {};

    std::string to_string() const noexcept {
        char buffer[sizeof(SCHEMA) * 4] = {};
        int size = sprintf(buffer, SCHEMA, off_rsa_verify, off_fopen);
        return std::string(buffer, (size_t)size);
    }

    void from_string(std::string const &buffer) noexcept {
        off_rsa_verify = 0;
        off_fopen = 0;
    }

    void scan(LCS::Process const &process) {
        auto data = process.Dump();
        macho.parse_data((MachO::data_t)data.data(), data.size());
        if (!(off_rsa_verify = macho.find_export("_int_rsa_verify"))) {
            throw std::runtime_error("Failed to find int_rsa_verify");
        }
        if (!(off_fopen = macho.find_import_ptr("_fopen"))) {
            throw std::runtime_error("Failed to find fopen");
        }
    }

    void wait_patchable(LCS::Process const &process, uint32_t delay, uint32_t timeout) {
        auto ptr = process.Rebase<PtrStorage>(off_fopen);
        for (; timeout > delay;) {
            PtrStorage data = 0;
            process.Read(ptr, data);
            if ((data > 0x7f0000000000)) {
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            timeout -= delay;
        }
    }

    struct Payload {
        unsigned char ret_true[16] = {
            0xb8, 01, 00, 00, 00, 0xc2, 00, 00,
        };
        unsigned char fopen_hook[0x80] = {
            0x55, 0x48, 0x89, 0xe5, 0x41, 0x54, 0x41, 0x55,
            0x48, 0x81, 0xec, 0x00, 0x04, 0x00, 0x00, 0x49,
            0x89, 0xfc, 0x49, 0x89, 0xf5, 0x4d, 0x85, 0xe4,
            0x74, 0x47, 0x4d, 0x85, 0xed, 0x74, 0x42, 0x41,
            0x80, 0x7d, 0x00, 0x72, 0x75, 0x3b, 0x41, 0x80,
            0x7d, 0x01, 0x62, 0x75, 0x34, 0x41, 0x80, 0x7d,
            0x02, 0x00, 0x75, 0x2d, 0x48, 0x89, 0xe7, 0x48,
            0x8d, 0x35, 0x4a, 0x00, 0x00, 0x00, 0xac, 0xaa,
            0x84, 0xc0, 0x75, 0xfa, 0x48, 0xff, 0xcf, 0x4c,
            0x89, 0xe6, 0xac, 0xaa, 0x84, 0xc0, 0x75, 0xfa,
            0x48, 0x89, 0xe7, 0x4c, 0x89, 0xee, 0xff, 0x15,
            0x24, 0x00, 0x00, 0x00, 0x48, 0x85, 0xc0, 0x75,
            0x0c, 0x4c, 0x89, 0xe7, 0x4c, 0x89, 0xee, 0xff,
            0x15, 0x13, 0x00, 0x00, 0x00, 0x48, 0x81, 0xc4,
            0x00, 0x04, 0x00, 0x00, 0x41, 0x5d, 0x41, 0x5c,
            0x5d, 0xc3, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
        };
        PtrStorage org_fopen = {};
        char prefix[0x200] = {};
    };

    void patch(LCS::Process const &process, std::string prefix) {
        if (!prefix.ends_with('/')) {
            prefix.push_back('/');
        }
        auto payload = Payload {};
        memcpy(payload.prefix, prefix.c_str(), prefix.size() + 1);
        auto const ptr_fopen = process.Rebase<PtrStorage>(off_fopen);
        auto const ptr_rsa_verify = process.Rebase<Payload>(off_rsa_verify);
        // Read org fopen
        process.Read(ptr_fopen, payload.org_fopen);
        // Write payload
        process.MarkWritable(ptr_rsa_verify);
        process.Write(ptr_rsa_verify, payload);
        process.MarkExecutable(ptr_rsa_verify);
        // Write new fopen
        process.Write(ptr_fopen, (PtrStorage)ptr_rsa_verify + offsetof(Payload, fopen_hook));
    }
};

ModOverlay::ModOverlay() : config_(std::make_unique<Config>()) {}

ModOverlay::~ModOverlay() = default;

std::string ModOverlay::to_string() const noexcept {
    return config_->to_string();
}

void ModOverlay::from_string(std::string const & str) noexcept {
    config_->from_string(str);
}

int ModOverlay::run(std::function<bool(Message)> update, std::filesystem::path const& profilePath) {
    auto process = Process::Find("/LeagueofLegends");
    if (!process) {
        LCS::SleepMS(50);
        return 0;
    }
    if (!update(M_FOUND)) return -1;
    if (!update(M_SCAN)) return -1;
    config_->scan(*process);
    if (!update(M_WAIT_PATCHABLE)) return -1;
    config_->wait_patchable(*process, 1, 2 * 60 * 1000);
    if (!update(M_PATCH)) return -1;
    config_->patch(*process, profilePath.generic_string());
    if (!update(M_WAIT_EXIT)) return -1;
    process->WaitExit();
    return 1;
}

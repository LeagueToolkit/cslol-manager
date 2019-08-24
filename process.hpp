#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <winnt.h>
#include <Psapi.h>
#include <cinttypes>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>
#include <memory>
#include <stdexcept>
#include <vector>

struct Process {
    struct Module {
        char name[MAX_PATH];
        uintptr_t base;
    };
    struct Header {
        size_t codeSize = 0;
        uint32_t checksum = 1;
    };

    HANDLE handle = 0;

    Process(const Process&) = delete;
    Process(Process&&) = delete;
    Process& operator=(const Process&) = delete;
    Process& operator=(Process&&) = delete;

    inline Process(DWORD pid) {
        auto const process = OpenProcess(
            PROCESS_VM_OPERATION
            | PROCESS_VM_READ
            | PROCESS_VM_WRITE
            | PROCESS_QUERY_INFORMATION
            | SYNCHRONIZE,
            false,
            pid);
        if (!process || process == INVALID_HANDLE_VALUE) {
            throw std::runtime_error("Failed to open process");
        }
        handle = process;
    }

    inline ~Process() {
        if (handle && handle != INVALID_HANDLE_VALUE) {
            CloseHandle(handle);
        }
    }

    inline std::vector<Module> GetModules() const {
        std::vector<Module> modules;
        HMODULE mods[1024];
        DWORD bmsize;
        if (!EnumProcessModules(handle, mods, sizeof(mods), &bmsize)) {
            throw std::runtime_error("Failed to enumerate process modules!");
        }
        auto const mend = mods + (bmsize / sizeof(HMODULE));
        modules.reserve(bmsize);
        for (auto m = mods; m < mend; m++) {
            Module mod{};
            if (GetModuleFileNameExA(handle, *m, mod.name, sizeof(mod.name))) {
                mod.base = reinterpret_cast<uintptr_t>(*m);
                modules.push_back(mod);
            }
        }
        return modules;
    }

    inline Header GetHeader(uintptr_t base) const {
        uint8_t raw[0x1000];
        if (!ReadProcessMemory(
            handle,
            reinterpret_cast<LPVOID>(base),
            raw,
            sizeof(raw),
            nullptr)) {
            throw std::runtime_error("Failed to read PE header!");
        }
        auto dos = reinterpret_cast<PIMAGE_DOS_HEADER>(raw);
        if (dos->e_magic != IMAGE_DOS_SIGNATURE) {
            throw std::runtime_error("DOS PE header magic doesn't match!");
        }
        auto nt = reinterpret_cast<PIMAGE_NT_HEADERS>(raw + dos->e_lfanew);
        if (nt->Signature != IMAGE_NT_SIGNATURE) {
            throw std::runtime_error("NT PE header signature doesn't match!");
        }
        return {
            nt->OptionalHeader.SizeOfCode,
            static_cast<uint32_t>(nt->OptionalHeader.CheckSum)
        };
    }

    inline std::string const Dump(uintptr_t base, size_t size) const {
        std::string pbuffer{};
        pbuffer.resize(size + 0x1000);
        for (size_t p = 0; p < size; p += 0x1000) {
            ReadProcessMemory(
                this->handle,
                reinterpret_cast<void*>(base + p),
                &pbuffer[p],
                0x1000,
                nullptr);
        }
        return pbuffer;
    }

    template<typename T>
    inline T Read(T* address) const {
        if (!address) {
            throw std::runtime_error("Can not read memory from nullptr");
        }
        T result = {};
        if (!ReadProcessMemory(this->handle,
            reinterpret_cast<void*>(address),
            &result,
            sizeof(T),
            nullptr)) {
            throw std::runtime_error("Failed to read memory");
        }
        return result;
    }

    template<typename T>
    inline void Write(T const& result, T* address) const {
        if (!address) {
            throw std::runtime_error("Can not write memory to nullptr");
        }
        if (!WriteProcessMemory(this->handle,
            reinterpret_cast<void*>(address),
            &result,
            sizeof(T),
            nullptr)) {
            throw std::runtime_error("Failed to write memory");
        }
    }

    template<typename T>
    inline T* Allocate() const {
        auto result = (VirtualAllocEx(this->handle,
            nullptr,
            sizeof(T),
            MEM_RESERVE | MEM_COMMIT,
            PAGE_READWRITE));
        if (!result) {
            throw std::runtime_error("Failed to allocate memory");
        }
        return reinterpret_cast<T*>(result);
    }

    template<typename T>
    inline DWORD SetProtection(T* address, DWORD flags) const {
        DWORD old = 0;
        if (!VirtualProtectEx(this->handle,
            address,
            sizeof(T),
            flags,
            &old)) {
            throw std::runtime_error("Failed to change protection");
        }
        return old;
    }

    template<typename T>
    inline void MarkExecutable(T* address) const {
        SetProtection(address, PAGE_EXECUTE);
    }
    
    inline void Wait() const {
        WaitForSingleObject(handle, INFINITE);
    }
};

inline uintptr_t ModuleFind(std::vector<Process::Module> const& mods, char const* what) {
    for (auto const& m : mods) {
        if (strstr(m.name, what)) {
            return m.base;
        }
    }
    return 0;
}

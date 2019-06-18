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

struct Process {
    HANDLE handle;
    uintptr_t base;
    size_t codeSize = 0;
    uint32_t checksum = 1;

    Process(const Process&) = delete;
    Process(Process&&) = delete;
    Process& operator=(const Process&) = delete;
    Process& operator=(Process&&) = delete;

    Process(DWORD pid, char const * const what) {
        auto process = OpenProcess(
                    PROCESS_VM_OPERATION
                    | PROCESS_VM_READ
                    | PROCESS_VM_WRITE
                    | PROCESS_QUERY_INFORMATION,
                    false,
                    pid);
        if(!process || process == INVALID_HANDLE_VALUE) {
            throw std::runtime_error("Failed to open process");
        }
        HMODULE mods[1024];
        DWORD bmsize;
        if (!EnumProcessModules(process, mods, sizeof(mods), &bmsize))
        {
            CloseHandle(process);
            throw std::runtime_error("Failed to enumerate process modules!");
        }
        auto const mend = mods + (bmsize / sizeof(HMODULE));
        for (auto m = mods; m < mend; m++)
        {
            char name[MAX_PATH];
            if (GetModuleFileNameExA(process, *m, name, sizeof(name)))
            {
                if(strstr(name, what) != nullptr) {
                    handle = process;
                    base = reinterpret_cast<uintptr_t>(*m);
                    break;
                }
            }
        }
        if(!base) {
            CloseHandle(process);
            throw std::runtime_error("No League of Legends.exe module found!");
        }
        uint8_t raw[0x1000];
        if(!ReadProcessMemory(
                    handle,
                    reinterpret_cast<LPVOID>(base),
                    raw,
                    sizeof(raw),
                    nullptr)) {
            CloseHandle(process);
            throw std::runtime_error("Failed to read PE header!");
        }
        auto dos = reinterpret_cast<PIMAGE_DOS_HEADER>(raw);
        if(dos->e_magic != IMAGE_DOS_SIGNATURE) {
            CloseHandle(process);
            throw std::runtime_error("DOS PE header magic doesn't match!");
        }
        auto nt = reinterpret_cast<PIMAGE_NT_HEADERS>(raw + dos->e_lfanew);
        if(nt->Signature != IMAGE_NT_SIGNATURE) {
            CloseHandle(process);
            throw std::runtime_error("NT PE header signature doesn't match!");
        }
        checksum = static_cast<uint32_t>(nt->OptionalHeader.CheckSum);
        codeSize = nt->OptionalHeader.SizeOfCode;
    }

    ~Process() {
        if(!handle && handle != INVALID_HANDLE_VALUE) {
            CloseHandle(handle);
        }
    }

    std::string const Dump() const {
        std::string pbuffer(codeSize, '\0');
        for(size_t p = 0; p < codeSize; p+= 0x1000) {
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
    T* OffBase(uintptr_t offset) const {
        return reinterpret_cast<T*>(base + offset);
    }

    template<typename T>
    T Read(T* address) const {
        if(!address) {
            throw std::runtime_error("Can not read memory from nullptr");
        }
        T result = {};
        if(!ReadProcessMemory(this->handle,
                              reinterpret_cast<void*>(address),
                              &result,
                              sizeof(T),
                              nullptr)) {
            throw std::runtime_error("Failed to read memory");
        }
        return result;
    }

    template<typename T>
    void Write(T const& result, T* address) const {
        if(!address) {
            throw std::runtime_error("Can not write memory to nullptr");
        }
        if(!WriteProcessMemory(this->handle,
                                 reinterpret_cast<void*>(address),
                                 &result,
                                 sizeof(T),
                               nullptr)) {
            throw std::runtime_error("Failed to write memory");
        }
    }

    template<typename T>
    T* Allocate() const {
        auto result = (VirtualAllocEx(this->handle,
                                      nullptr,
                                      sizeof(T),
                                      MEM_RESERVE | MEM_COMMIT,
                                      PAGE_READWRITE));
        if(!result) {
            throw std::runtime_error("Failed to allocate memory");
        }
        return reinterpret_cast<T*>(result);
    }

    template<typename T>
    void MarkExecutable(T* address) const {
        DWORD old = 0;
       if(!VirtualProtectEx(this->handle,
                            address,
                            sizeof(T),
                            PAGE_EXECUTE,
                            &old)) {
           throw std::runtime_error("Failed to mark memory executable");
       }
    }
};

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <winnt.h>
#include <Psapi.h>
#include <WinUser.h>
#include <utility>
#include <stdexcept>
#include "process.hpp"

static uint32_t FindProcess(char const* name, uint32_t delay) {
    DWORD pids[1024];
    DWORD pidsSize;
    HMODULE mod;
    DWORD modsSize;
    char modName[MAX_PATH];
    
    for(;;)
    if (EnumProcesses(pids, sizeof(pids), &pidsSize)) {
        auto const pidsEnd = pids + pidsSize / sizeof(pidsSize);
        for (auto pid = pids; pid != pidsEnd; pid++) {
            if (*pid) {
                auto const process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, *pid);
                if (process && process != INVALID_HANDLE_VALUE) {
                    if(EnumProcessModules(process, &mod, sizeof(mod), &modsSize)) {
                        if(GetModuleBaseNameA(process, mod, modName, sizeof(modName))) {
                            if(strstr(modName, name)) {
                                CloseHandle(process);
                                return *pid;
                            }
                        }
                    }
                    CloseHandle(process);
                }
            }
        }
        Sleep(delay);
    }
}


static BOOL CALLBACK FindLoLCB(HWND hwnd, LPARAM tgt) {
    auto tgtpid = reinterpret_cast<uint32_t*>(tgt);
    if(DWORD pid = 0; GetWindowThreadProcessId(hwnd, &pid) && *tgtpid == pid){
        *tgtpid = 0;
        return FALSE;
    }
    return TRUE;
}

Process::Process(char const* name, uint32_t delay) : Process(FindProcess(name, delay)) {}

Process::Process(uint32_t apid) {
    auto const process = OpenProcess(
        PROCESS_VM_OPERATION
        | PROCESS_VM_READ
        | PROCESS_VM_WRITE
        | PROCESS_QUERY_INFORMATION
        | SYNCHRONIZE,
        false,
        apid);
    if (!process || process == INVALID_HANDLE_VALUE) {
        throw std::runtime_error("Failed to open process");
    }

    HMODULE mod;
    DWORD modSize;
    char raw[0x1000];
    
    if (!EnumProcessModules(process, &mod, sizeof(mod), &modSize)) {
        CloseHandle(process);
        throw std::runtime_error("Failed to enum  process modules");
    }
    
    if (!ReadProcessMemory(process, mod, raw, sizeof(raw), nullptr)) {
        CloseHandle(process);
        throw std::runtime_error("Failed to open process");
    }
    
    auto const dos = reinterpret_cast<PIMAGE_DOS_HEADER>(raw);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) {
        CloseHandle(process);
        throw std::runtime_error("Failed to open process");
    }
    
    auto const nt = reinterpret_cast<PIMAGE_NT_HEADERS>(raw + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) {
        CloseHandle(process);
        throw std::runtime_error("Failed to open process");
    }
    
    handle = process;
    pid = apid;
    base = reinterpret_cast<uintptr_t>(mod);
    size = nt->OptionalHeader.SizeOfCode;
    checksum = static_cast<uint32_t>(nt->OptionalHeader.CheckSum);
}

Process::~Process() {
    if (handle && handle != INVALID_HANDLE_VALUE) {
        CloseHandle(handle);
        handle = nullptr;
        pid = 0;
    }
}

std::vector<uint8_t> Process::Dump() const {
    std::vector<uint8_t> buffer = {};
    buffer.resize(size + 0x1000);
    for (size_t p = 0; p < size; p += 0x1000) {
        ReadProcessMemory(handle, reinterpret_cast<void*>(base + p), buffer.data() + p, 0x1000, nullptr);
    }
    return buffer;
}

void Process::WaitExit(uint32_t) const {
    WaitForSingleObject(handle, INFINITE);
}

void* Process::WaitMemoryNonZero(void* address, uint32_t delay) const {
    void* result = nullptr;
    for(; !result; Sleep(delay)) {
        ReadProcessMemory(handle, address, &result, sizeof(result), nullptr);
    }
    return result;
}

void Process::WaitWindow(char const*, uint32_t delay) const {
    for(uint32_t tgt = pid; tgt == pid; Sleep(delay)) {
        EnumWindows(FindLoLCB, reinterpret_cast<LPARAM>(&tgt));
    }
}

void Process::ReadMemory(void* address, void* dest, size_t size) const {
    if (!address) {
        throw std::runtime_error("Can not read memory from nullptr");
    }
    if (!ReadProcessMemory(handle, address, dest, size, nullptr)) {
        throw std::runtime_error("Failed to read memory");
    }
}

void Process::WriteMemory(void* address, void const* src, size_t size) const {
    if (!address) {
        throw std::runtime_error("Can not read memory from nullptr");
    }
    if (!WriteProcessMemory(handle, address, src, size, nullptr)) {
        throw std::runtime_error("Failed to read memory");
    }
}

void Process::MarkMemoryExecutable(void *address, size_t size) const {
    DWORD old = 0;
    if (!VirtualProtectEx(handle, address, size, PAGE_EXECUTE, &old)) {
        throw std::runtime_error("Failed to change protection");
    }
}

void* Process::AllocateMemory(size_t size) const {
    auto ptr = VirtualAllocEx(handle, nullptr, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if(!ptr) {
        throw std::runtime_error("Failed to allocate memory");
    }
    return ptr;
}

#define WIN32_LEAN_AND_MEAN
/// Basic windows header
#include <Windows.h>
/// Extra windows header
#include <Psapi.h>
#include <TlHelp32.h>
/// End windows headers
#include "process.hpp"
#include <chrono>
#include <stdexcept>
#include <thread>
#include <utility>

using namespace LCS;

namespace {
    static inline constexpr DWORD PROCESS_NEEDED_ACCESS = PROCESS_VM_OPERATION | PROCESS_VM_READ |
                                                          PROCESS_VM_WRITE |
                                                          PROCESS_QUERY_INFORMATION | SYNCHRONIZE;
    static inline constexpr size_t DUMP_SIZE = 0x4000000;

    static inline auto SafeWinHandle(HANDLE handle) noexcept {
        if (handle == INVALID_HANDLE_VALUE) {
            handle = nullptr;
        }
        constexpr auto deleter = [](auto handle) { CloseHandle(handle); };
        using handle_value = std::remove_pointer_t<HANDLE>;
        return std::unique_ptr<handle_value, decltype(deleter)>(handle);
    }
}

Process::Process(void *handle) noexcept : handle_(handle) {
    if (handle_ == INVALID_HANDLE_VALUE) {
        handle_ = nullptr;
    }
}

Process::Process(uint32_t pid) noexcept : Process(OpenProcess(PROCESS_NEEDED_ACCESS, false, pid)) {}

Process::~Process() noexcept {
    if (handle_ && handle_ != INVALID_HANDLE_VALUE) {
        CloseHandle(handle_);
        handle_ = nullptr;
    }
}

uint32_t Process::FindPID(char const *name) noexcept {
    PROCESSENTRY32 entry = {};
    entry.dwSize = sizeof(PROCESSENTRY32);
    auto handle = SafeWinHandle(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
    if (!handle) {
        return 0;
    }
    for (bool i = Process32First(handle.get(), &entry); i;
         i = Process32Next(handle.get(), &entry)) {
        if (strstr(entry.szExeFile, name)) {
            return entry.th32ProcessID;
        }
    }
    return 0;
}

PtrStorage Process::Base() const {
    if (!base_) {
        HMODULE mod = {};
        DWORD modSize = {};
        if (!EnumProcessModules(handle_, &mod, sizeof(mod), &modSize)) {
            throw std::runtime_error("Failed to enum  process modules");
        }
        base_ = (PtrStorage)(uintptr_t)mod;
    }
    return base_;
}

uint32_t Process::Checksum() const {
    if (!checksum_) {
        char raw[0x1000] = {};
        auto base = Base();
        if (!ReadProcessMemory(handle_, (void const *)(uintptr_t)base, raw, sizeof(raw), nullptr)) {
            throw std::runtime_error("Failed to read memory from process");
        }
        auto const dos = (PIMAGE_DOS_HEADER)(raw);
        if (dos->e_magic != IMAGE_DOS_SIGNATURE) {
            throw std::runtime_error("Failed to get dos header from process");
        }
        auto const nt = (PIMAGE_NT_HEADERS32)(raw + dos->e_lfanew);
        if (nt->Signature != IMAGE_NT_SIGNATURE) {
            throw std::runtime_error("Failed to get nt header from process");
        }
        checksum_ = (uint32_t)(nt->OptionalHeader.CheckSum);
    }
    return checksum_;
}

std::vector<uint8_t> Process::Dump() const {
    std::vector<uint8_t> buffer = {};
    buffer.resize(DUMP_SIZE);
    for (size_t p = 0; p != DUMP_SIZE; p += 0x1000) {
        ReadProcessMemory(handle_, (void *)(base_ + p), buffer.data() + p, 0x1000, nullptr);
    }
    return buffer;
}

bool Process::WaitExit(uint32_t timeout) const {
    switch (WaitForSingleObject(handle_, timeout)) {
    case WAIT_OBJECT_0:
        return true;
    case WAIT_TIMEOUT:
        return false;
    default:
        throw std::runtime_error("Failed to WaitExit!");
    }
}

bool Process::WaitInitialized(uint32_t timeout) const {
    for (size_t i = 0; i != 10; i++) {
        switch (WaitForInputIdle(handle_, timeout)) {
        case 0:
            return true;
        case WAIT_TIMEOUT:
            return false;
        default:
            break;
        }
    }
    throw std::runtime_error("Failed to wait for WaitInitialized!");
}

void Process::ReadMemory(void *address, void *dest, size_t size) const {
    if (!address) {
        throw std::runtime_error("Can not read memory from nullptr");
    }
    if (!ReadProcessMemory(handle_, address, dest, size, nullptr)) {
        throw std::runtime_error("Failed to read memory");
    }
}

void Process::WriteMemory(void* address, void const* src, size_t sizeBytes) const {
    if (!address) {
        throw std::runtime_error("Can not read memory from nullptr");
    }
    if (!WriteProcessMemory(handle_, address, src, sizeBytes, nullptr)) {
        throw std::runtime_error("Failed to read memory");
    }
}

void Process::MarkMemoryExecutable(void *address, size_t sizeBytes) const {
    DWORD old = 0;
    if (!VirtualProtectEx(handle_, address, sizeBytes, PAGE_EXECUTE, &old)) {
        throw std::runtime_error("Failed to change protection");
    }
}

void* Process::AllocateMemory(size_t sizeBytes) const {
    auto ptr =
        VirtualAllocEx(handle_, nullptr, sizeBytes, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if(!ptr) {
        throw std::runtime_error("Failed to allocate memory");
    }
    return ptr;
}

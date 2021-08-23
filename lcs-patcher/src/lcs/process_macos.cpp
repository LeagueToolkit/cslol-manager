#include "process.hpp"
#include <algorithm>
#include <stdexcept>
#include <mach/mach.h>
#include <mach/mach_traps.h>
#include <mach/mach_vm.h>
#include <libproc.h>
#include <unistd.h>
#include <chrono>
#include <thread>

using namespace LCS;

Process::Process (std::uint32_t pid)  {
    mach_port_t task = {};
    if (auto err = task_for_pid(mach_task_self(), pid, &task)) {
        throw std::runtime_error("Failed to open pid(" + std::to_string(pid) + "): " + std::to_string(err));
    }
    handle_ = (void*)(std::uintptr_t)task;
    char pathbuf[PROC_PIDPATHINFO_MAXSIZE] = {};
    if (auto const ret = proc_pidpath (pid, pathbuf, sizeof(pathbuf)); ret < 0) {
        throw std::runtime_error("Failed to get path for pid(" + std::to_string(pid) + "): " + std::to_string(ret));
    } else {
        path_ = std::string { pathbuf, pathbuf + ret };
    }
}

Process::~Process() noexcept {
    if (handle_) {
        mach_port_deallocate(mach_task_self(), (mach_port_t)(std::size_t)handle_);
    }
}

bool Process::ThisProcessHasParent() noexcept {
    return getppid() > 1;
}

std::optional<Process> Process::Find (char const* name) {
    pid_t pids[4096];
    int bytes = proc_listpids(PROC_ALL_PIDS, 0, pids, sizeof(pids));
    int n_proc = bytes / sizeof(pid_t);
    char pathbuf[PROC_PIDPATHINFO_MAXSIZE] = {};
    for (auto p = pids; p != pids + n_proc; p++) {
        if (auto const ret = proc_pidpath (*p, pathbuf, sizeof(pathbuf)); ret > 0) {
            if (std::string_view { pathbuf, static_cast<std::size_t>(ret) }.ends_with(name)) {
                return Process { static_cast<std::uint32_t>(*p) };
            }
        }
    }
    return std::nullopt;
}

std::uint64_t Process::Base() const {
    if (!base_) {
        vm_map_offset_t vmoffset = {};
        vm_map_size_t vmsize = {};
        uint32_t nesting_depth = 0;
        struct vm_region_submap_info_64 vbr[16] = {};
        mach_msg_type_number_t vbrcount = 16;
        kern_return_t kr;
        if (auto const err = mach_vm_region_recurse((mach_port_t)(uintptr_t)handle_,
                                                    &vmoffset,
                                                    &vmsize,
                                                    &nesting_depth,
                                                    (vm_region_recurse_info_t)&vbr,
                                                    &vbrcount)) {
            throw std::runtime_error("Failed to get_base!");
        }
        base_ = vmoffset - 0x100000000;
    }
    return base_;
}

std::filesystem::path Process::Path() const {
    return path_;
}

uint32_t Process::Checksum() const {
    return 0;
}

std::vector<char> Process::Dump() const {
    std::vector<char> buffer = {};
    auto const path = Path().generic_string();
    auto const size = std::filesystem::file_size(path);
    if (auto const file = fopen(path.c_str(), "rb")) {
        buffer.resize(size);
        fread(buffer.data(), buffer.size(), 1, file);
        fclose(file);
    }
    return buffer;
}

bool Process::WaitExit(uint32_t timeout) const {
    int p = 0;
    pid_for_task((mach_port_t)(uintptr_t)handle_, &p);
    return p < 0;
}

bool Process::WaitInitialized(uint32_t timeout) const {
    return true;
}

void Process::ReadMemory(void *address, void *dest, size_t size) const {
    mach_msg_type_number_t orgdata_read = 0;
    vm_offset_t offset = {};
    if (auto const err = mach_vm_read((mach_port_t)(uintptr_t)handle_,
                                      (mach_vm_address_t)address,
                                      size,
                                      &offset,
                                      &orgdata_read)) {
        throw std::runtime_error("Failed to read memory: " + std::to_string(err));
    }
    if (orgdata_read != size) {
        throw std::runtime_error("Failed to read memory: needed("
                                  + std::to_string(size) + "), got("
                                  + std::to_string(orgdata_read) + ")");
    }
    memcpy(dest, (void*)offset, orgdata_read);
}

void Process::WriteMemory(void *address, void const* src, std::size_t sizeBytes) const {
    if (auto const err = mach_vm_write((mach_port_t)(uintptr_t)handle_,
                                       (mach_vm_address_t)address,
                                       (vm_offset_t)src,
                                       sizeBytes)) {
        throw std::runtime_error("Failed to write memory: " + std::to_string(err));
    }
}

void Process::MarkMemoryWritable(void* address, std::size_t size) const {
    if (auto const err = mach_vm_protect((mach_port_t)(uintptr_t)handle_,
                                         (mach_vm_address_t)address,
                                         (mach_vm_size_t)size,
                                         FALSE,
                                         VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE | VM_PROT_COPY)) {
        throw std::runtime_error("Failed to mark rwx memory: " + std::to_string(err));
    }
}

void Process::MarkMemoryExecutable(void* address, std::size_t size) const {
    if (auto const err = mach_vm_protect((mach_port_t)(uintptr_t)handle_,
                                         (mach_vm_address_t)address,
                                         (mach_vm_size_t)size,
                                         FALSE,
                                         VM_PROT_READ | VM_PROT_EXECUTE)) {
        throw std::runtime_error("Failed to mark rx memory: " + std::to_string(err));
    }
}

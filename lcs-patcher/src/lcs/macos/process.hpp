#pragma once
#include <cstdint>
#include <cstddef>
#include <string>
#include <optional>

namespace LCS {
    struct Process {
    public:
        inline Process() {}
        Process(Process const&) = delete;
        Process(Process&& other);
        Process& operator= (Process other);
        Process (std::uint32_t pid);
        ~Process();

        static std::optional<Process> find (char const* name);

        std::uint64_t get_base() const;
        std::string get_path() const;
        void read_memory(std::uint64_t address, void* dst, std::size_t count) const;
        void write_memory(std::uint64_t address, void const* src, std::size_t count) const;
        void mark_memory_rwx(std::uint64_t address, std::size_t size) const;
        void mark_memory_rx(std::uint64_t address, std::size_t size) const;
        void wait_ptr_not_null(std::uint64_t address, uint32_t delay, uint32_t timeout) const;
    private:
        std::size_t handle_ = {};
        std::string path_ = {};
        std::uint64_t mutable base_ = {};
    };
}

/// Implementation

#include <algorithm>
#include <stdexcept>
#include <mach/mach.h>
#include <mach/mach_traps.h>
#include <mach/mach_vm.h>
#include <libproc.h>
#include <unistd.h>
#include <chrono>
#include <thread>

namespace LCS {
    inline Process::Process(Process&& other) {
        std::swap(handle_, other.handle_);
        std::swap(path_, other.path_);
        std::swap(base_, other.base_);
    }

    inline Process& Process::operator= (Process other) {
        std::swap(handle_, other.handle_);
        std::swap(path_, other.path_);
        std::swap(base_, other.base_);
        return *this;
    }

    inline Process::Process (std::uint32_t pid) {
        mach_port_t task = {};
        if (auto err = task_for_pid(mach_task_self(), pid, &task)) {
            throw std::runtime_error("Failed to open pid(" + std::to_string(pid) + "): " + std::to_string(err));
        }
        handle_ = task;
        vm_map_offset_t vmoffset = {};
        vm_map_size_t vmsize = {};
        uint32_t nesting_depth = 0;
        struct vm_region_submap_info_64 vbr[16] = {};
        mach_msg_type_number_t vbrcount = 16;
        kern_return_t kr;
        if (auto const err = mach_vm_region_recurse(handle_,
                                                    &vmoffset,
                                                    &vmsize,
                                                    &nesting_depth,
                                                    (vm_region_recurse_info_t)&vbr,
                                                    &vbrcount)) {
            throw std::runtime_error("Failed to get_base!");
        }
        base_ = vmoffset - 0x100000000;
        char pathbuf[PROC_PIDPATHINFO_MAXSIZE] = {};
        if (auto const ret = proc_pidpath (pid, pathbuf, sizeof(pathbuf)); ret < 0) {
            throw std::runtime_error("Failed to get pid(" + std::to_string(pid) + ") path: " + std::to_string(ret));
        } else {
            path_ = { pathbuf, pathbuf + ret };
        }
    }

    inline Process::~Process() {
        if (handle_) {
            mach_port_deallocate(mach_task_self(), (mach_port_t)(std::size_t)handle_);
        }
    }

    inline std::optional<Process> Process::find (char const* name) {
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

    inline std::uint64_t Process::get_base() const {
        return base_;
    }

    inline std::string Process::get_path() const {
        return path_;
    }

    inline void Process::read_memory(std::uint64_t address, void* dst, std::size_t count) const {
        mach_msg_type_number_t orgdata_read = 0;
        vm_offset_t offset = {};
        if (auto const err = mach_vm_read(handle_,
                                          (mach_vm_address_t)address,
                                          count,
                                          &offset,
                                          &orgdata_read)) {
            throw std::runtime_error("Failed to read memory: " + std::to_string(err));
        }
        memcpy(dst, (void*)offset, orgdata_read);
    }

    inline void Process::write_memory(std::uint64_t address, void const* src, std::size_t count) const {
        if (auto const err = mach_vm_write(handle_,
                                           (mach_vm_address_t)address,
                                           (vm_offset_t)src,
                                           count)) {
            throw std::runtime_error("Failed to write memory: " + std::to_string(err));
        }
    }

    inline void Process::mark_memory_rwx(std::uint64_t address, std::size_t size) const {
        if (auto const err = mach_vm_protect(handle_,
                                             (mach_vm_address_t)address,
                                             (mach_vm_size_t)size,
                                             FALSE,
                                             VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE | VM_PROT_COPY)) {
            throw std::runtime_error("Failed to mark rwx memory: " + std::to_string(err));
        }
    }

    inline void Process::mark_memory_rx(std::uint64_t address, std::size_t size) const {
        if (auto const err = mach_vm_protect(handle_,
                                             (mach_vm_address_t)address,
                                             (mach_vm_size_t)size,
                                             FALSE,
                                             VM_PROT_READ | VM_PROT_EXECUTE)) {
            throw std::runtime_error("Failed to mark rx memory: " + std::to_string(err));
        }
    }

    inline void Process::wait_ptr_not_null(std::uint64_t address, uint32_t delay, uint32_t timeout) const {
        for (; timeout > delay;) {
            vm_offset_t offset = {};
            mach_msg_type_number_t orgdata_read = 0;
            if (auto const err = mach_vm_read(handle_,
                                              (mach_vm_address_t)address,
                                              sizeof(offset),
                                              &offset,
                                              &orgdata_read)) {
                throw std::runtime_error("Failed to read memory: " + std::to_string(err));
            }
            if (offset) {
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            timeout -= delay;
        }
        throw std::runtime_error("Failed to WaitMemoryNonZero!");
    }
}

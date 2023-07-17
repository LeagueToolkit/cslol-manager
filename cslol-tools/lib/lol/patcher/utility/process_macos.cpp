#ifdef __APPLE__
#    include <lol/error.hpp>

#    include "process.hpp"
// do not reoder
#    include <libproc.h>
#    include <mach/mach.h>
#    include <mach/mach_traps.h>
#    include <mach/mach_vm.h>
#    include <unistd.h>

using namespace lol;
using namespace lol::patcher;

Process::Process() noexcept = default;

Process::Process(Process&& other) noexcept {
    std::swap(handle_, other.handle_);
    std::swap(base_, other.base_);
    std::swap(path_, other.path_);
    std::swap(pid_, other.pid_);
}

Process& Process::operator=(Process&& other) noexcept {
    if (this == &other) return *this;
    std::swap(handle_, other.handle_);
    std::swap(base_, other.base_);
    std::swap(path_, other.path_);
    std::swap(pid_, other.pid_);
    return *this;
}

Process::~Process() noexcept {
    if (handle_) {
        mach_port_deallocate(mach_task_self(), (mach_port_t)(std::size_t)handle_);
    }
}

auto Process::FindPid(char const* name) -> std::uint32_t {
    if (name) {
        pid_t pids[4096];
        int bytes = proc_listpids(PROC_ALL_PIDS, 0, pids, sizeof(pids));
        int n_proc = bytes / sizeof(pid_t);
        char pathbuf[PROC_PIDPATHINFO_MAXSIZE] = {};
        for (auto p = pids; p != pids + n_proc; p++) {
            if (auto const ret = proc_pidpath(*p, pathbuf, sizeof(pathbuf)); ret > 0) {
                if (std::string_view{pathbuf, static_cast<std::size_t>(ret)}.ends_with(name)) {
                    return static_cast<std::uint32_t>(*p);
                }
            }
        }
    }
    return 0;
}

auto Process::FindPidWindow(char const* window) -> std::uint32_t { return 0; }

auto Process::Open(std::uint32_t pid) -> Process {
    if (mach_port_t task = {}; !task_for_pid(mach_task_self(), pid, &task)) {
        return Process((void*)(std::uintptr_t)task, pid);
    }
    lol_throw_msg("task_for_pid");
}

auto Process::Base() const -> PtrStorage {
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
            lol_throw_msg("mach_vm_region_recurse: {:#x}", (std::uint32_t)err);
        }
        base_ = vmoffset - 0x100000000;
    }
    return base_;
}

auto Process::TryBase() const noexcept -> std::optional<PtrStorage> {
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
            return std::nullopt;
        }
        base_ = vmoffset - 0x100000000;
    }
    return base_;
}

auto Process::Path() const -> fs::path {
    lol_trace_func();
    if (!path_.empty()) return path_;
    char pathbuf[PROC_PIDPATHINFO_MAXSIZE] = {};
    if (auto const ret = proc_pidpath(pid_, pathbuf, sizeof(pathbuf)); ret <= 0) {
        lol_throw_msg("proc_pidpath(pid: {}): {:#x}", pid_, (std::uint32_t)ret);
    } else {
        path_ = std::string{pathbuf, pathbuf + ret};
    }
    return path_;
}

auto Process::Dump() const -> std::vector<char> {
    std::vector<char> buffer = {};
    auto const path = Path().generic_string();
    if (auto const file = fopen(path.c_str(), "rb")) {
        auto const size = std::filesystem::file_size(path);
        buffer.resize(size);
        fread(buffer.data(), buffer.size(), 1, file);
        fclose(file);
    } else {
        lol_throw_msg("fopen(path: {}, \"rb\") failed", path.c_str());
    }
    return buffer;
}

auto Process::IsExited() const noexcept -> bool {
    int p = 0;
    pid_for_task((mach_port_t)(uintptr_t)handle_, &p);
    return p < 0;
}

auto Process::WaitInitialized(uint32_t timeout) const noexcept -> bool { return true; }

auto Process::TryReadMemory(void* address, void* dest, size_t size) const noexcept -> bool {
    mach_msg_type_number_t orgdata_read = 0;
    vm_offset_t offset = {};
    if (auto const err =
            mach_vm_read((mach_port_t)(uintptr_t)handle_, (mach_vm_address_t)address, size, &offset, &orgdata_read)) {
        return false;
    }
    if (orgdata_read != size) {
        return false;
    }
    memcpy(dest, (void*)offset, orgdata_read);
    return true;
}

auto Process::ReadMemory(void* address, void* dest, size_t size) const -> void {
    mach_msg_type_number_t got_size = 0;
    vm_offset_t offset = {};
    if (auto const err =
            mach_vm_read((mach_port_t)(uintptr_t)handle_, (mach_vm_address_t)address, size, &offset, &got_size)) {
        lol_throw_msg("mach_vm_read: {:#x}", (std::uint32_t)err);
    }
    if (got_size != size) {
        lol_throw_msg("mach_vm_read: got {:#x}", got_size);
    }
    memcpy(dest, (void*)offset, got_size);
}

auto Process::WriteMemory(void* address, void const* src, std::size_t sizeBytes) const -> void {
    if (auto const err =
            mach_vm_write((mach_port_t)(uintptr_t)handle_, (mach_vm_address_t)address, (vm_offset_t)src, sizeBytes)) {
        lol_throw_msg("mach_vm_write: {:#x}", (std::uint32_t)err);
    }
}

auto Process::MarkMemoryWritable(void* address, std::size_t size) const -> void {
    if (auto const err = mach_vm_protect((mach_port_t)(uintptr_t)handle_,
                                         (mach_vm_address_t)address,
                                         (mach_vm_size_t)size,
                                         FALSE,
                                         VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE | VM_PROT_COPY)) {
        lol_throw_msg("mach_vm_protect: {:#x}", (std::uint32_t)err);
    }
}

auto Process::MarkMemoryExecutable(void* address, std::size_t size) const -> void {
    if (auto const err = mach_vm_protect((mach_port_t)(uintptr_t)handle_,
                                         (mach_vm_address_t)address,
                                         (mach_vm_size_t)size,
                                         FALSE,
                                         VM_PROT_READ | VM_PROT_EXECUTE)) {
        lol_throw_msg("mach_vm_protect: {:#x}", (std::uint32_t)err);
    }
}

#endif
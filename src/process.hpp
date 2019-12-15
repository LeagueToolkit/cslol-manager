#pragma once
#include <cinttypes>
#include <cstddef>
#include <vector>
#include <type_traits>


struct Process {
private:
    void* handle = nullptr;
    uint32_t pid = 0;
    uintptr_t base = 0;
    size_t size = 0;
    uint32_t checksum = 1;
public:
    Process(const Process&) = delete;
    Process& operator=(const Process&) = delete;
    Process& operator=(Process&&) = delete;
    
    Process(uint32_t pid);
    Process(char const* name, uint32_t loopInterval);
    ~Process();

    template<typename T = void>
    inline T* Rebase(uintptr_t offset) const noexcept {
        return reinterpret_cast<T*>(offset + base);
    }

    inline uintptr_t Debase(uintptr_t offset) const noexcept {
        return offset - base;
    }
    
    inline uint32_t Checksum() const noexcept {
        return checksum;
    }

    void WaitExit(uint32_t loopInterval) const;

    void* WaitMemoryNonZero(void* address, uint32_t loopInterval) const;
    
    void WaitWindow(char const* name, uint32_t loopInterval) const;
    
    std::vector<uint8_t> Dump() const;
    
    void ReadMemory(void* address, void* dest, size_t size) const;
    
    void WriteMemory(void* address, void const* src, size_t size) const;
    
    void MarkMemoryExecutable(void* address, size_t size) const;
    
    void* AllocateMemory(size_t size) const;
    
    inline Process(Process&& other) noexcept { 
        std::swap(handle, other.handle); 
        std::swap(pid, other.pid);
        std::swap(base, other.base);
        std::swap(size, other.size);
        std::swap(checksum, other.checksum);
    }

    template<typename T>
    inline T* WaitNonZero(T** address, uint32_t delay = 1) const {
        return reinterpret_cast<T*>(WaitMemoryNonZero(address, delay));
    }

    template<typename T>
    inline void Copy(T* from, T* to, size_t count = 1) const {
        std::vector<T> buffer{};
        buffer.resize(count);
        ReadMemory(from, buffer.data(), count * sizeof(T));
        WriteMemory(to, buffer.data(), count * sizeof(T));
    }

    template<typename T>
    inline void Read(T* address, T& dest, size_t count = 1) const {
        ReadMemory(address, &dest, count * sizeof(T));
    }

    template<typename T>
    inline void Write(T* address, T const& src, size_t count = 1) const {
        WriteMemory(address, &src, count * sizeof(T));
    }

    template<typename T>
    inline T* Allocate(size_t count = 1) const {
        return reinterpret_cast<T*>(AllocateMemory(count * sizeof(T)));
    }

    template<typename T>
    void MarkExecutable(T* address, size_t count = 1) const {
        MarkMemoryExecutable(address, count * sizeof(T));
    }
};

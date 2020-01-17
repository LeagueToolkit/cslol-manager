#pragma once
#include <cinttypes>
#include <cstddef>
#include <vector>
#include <type_traits>
#include <thread>

namespace LCS {
    inline void SleepMiliseconds(uint32_t time) noexcept {
         std::this_thread::sleep_for(std::chrono::milliseconds(time));
    }

    using PtrStorage = uint32_t;

    template<typename T>
    struct Ptr;

    template<>
    struct Ptr<void> {
        PtrStorage storage = 0;

        inline Ptr() noexcept = default;
        inline Ptr(Ptr const&) noexcept = default;
        inline Ptr(Ptr&&) noexcept = default;
        inline Ptr& operator=(Ptr const&) noexcept = default;
        inline Ptr& operator=(Ptr&&) noexcept = default;
        inline Ptr(uintptr_t value) noexcept : storage(static_cast<PtrStorage>(value)) {}
        explicit inline Ptr(void* value) noexcept : Ptr(reinterpret_cast<uintptr_t>(value)) {}
        explicit inline operator void*() const noexcept {
            return reinterpret_cast<void*>(static_cast<uintptr_t>(storage));
        }
    };

    template<typename T>
    struct Ptr : Ptr<void> {
        static_assert (!std::is_pointer_v<T>);
        using Ptr<void>::Ptr;
        using Ptr<void>::operator void*;

        inline Ptr(T* value) noexcept : Ptr(reinterpret_cast<uintptr_t>(value)) {}

        inline auto operator->() const noexcept {
            return reinterpret_cast<T*>(static_cast<void*>(*this));
        }
    };


    template<typename T>
    Ptr(T*) -> Ptr<T>;

    struct Process {
    private:
        void* handle = nullptr;
        uint32_t pid = 0;
        PtrStorage base = 0;
        uint32_t size = 0;
        uint32_t checksum = 1;
    public:
        Process(const Process&) = delete;
        Process& operator=(const Process&) = delete;
        Process& operator=(Process&&) = delete;

        Process(uint32_t pid);
        ~Process();

        static uint32_t Find(char const* name) noexcept;

        template<typename T = void>
        inline Ptr<T> Rebase(uintptr_t offset) const noexcept {
            return static_cast<Ptr<T>>(offset + base);
        }

        inline PtrStorage Debase(PtrStorage offset) const noexcept {
            return offset - base;
        }

        inline uint32_t Checksum() const noexcept {
            return checksum;
        }

        bool Exited() const noexcept;

        void WaitExit() const;

        PtrStorage WaitMemoryNonZero(void* address, uint32_t loopInterval, uint32_t timeout) const;

        bool FindWindowName(char const* name) const;

        void WaitWindow(char const* name, uint32_t loopInterval = 50, uint32_t timeout = 1000 * 180) const;

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
        inline Ptr<T> WaitNonZero(Ptr<Ptr<T>> address, uint32_t delay = 1, uint32_t timeout = 1000 * 60) const {
            auto result = WaitMemoryNonZero(static_cast<void*>(address), delay, timeout);
            return static_cast<Ptr<T>>(result);
        }

        template<typename T>
        inline void Copy(Ptr<T> from, Ptr<T> to, size_t count = 1) const {
            std::vector<T> buffer{};
            buffer.resize(count);
            ReadMemory(static_cast<void*>(from), buffer.data(), count * sizeof(T));
            WriteMemory(static_cast<void*>(to), buffer.data(), count * sizeof(T));
        }

        template<typename T>
        inline void Read(Ptr<T> address, T& dest, size_t count = 1) const {
            ReadMemory(static_cast<void*>(address), &dest, count * sizeof(T));
        }

        template<typename T>
        inline void Write(Ptr<T> address, T const& src, size_t count = 1) const {
            WriteMemory(static_cast<void*>(address), &src, count * sizeof(T));
        }

        template<typename T>
        inline Ptr<T> Allocate(size_t count = 1) const {
            auto result = AllocateMemory(count * sizeof(T));
            return static_cast<Ptr<T>>(result);
        }

        template<typename T>
        inline void MarkExecutable(Ptr<T> address, size_t count = 1) const {
            MarkMemoryExecutable(static_cast<void*>(address), count * sizeof(T));
        }
    };
}

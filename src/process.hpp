#pragma once
#include <cinttypes>
#include <cstddef>
#include <optional>
#include <thread>
#include <type_traits>
#include <vector>

namespace LCS {
    inline void SleepMS(uint32_t time) noexcept {
        std::this_thread::sleep_for(std::chrono::milliseconds(time));
    }

    template <typename T>
    struct Ptr;

    template <typename T>
    Ptr(T *)->Ptr<T>;

    using PtrStorage = uint32_t;

    template <>
    struct Ptr<void> {
        PtrStorage storage = {};
        inline Ptr() noexcept = default;
        inline Ptr(Ptr const&) noexcept = default;
        inline Ptr(Ptr&&) noexcept = default;
        inline Ptr& operator=(Ptr const&) noexcept = default;
        inline Ptr& operator=(Ptr&&) noexcept = default;
        inline Ptr(uintptr_t value) noexcept : storage((PtrStorage)(value)) {}
        explicit inline Ptr(void *value) noexcept : Ptr((uintptr_t)(value)) {}
        explicit inline operator void *() const noexcept { return (void *)((uintptr_t)(storage)); }
    };

    template<typename T>
    struct Ptr : Ptr<void> {
        static_assert (!std::is_pointer_v<T>);
        using Ptr<void>::Ptr;
        using Ptr<void>::operator void *;
        inline Ptr(T *value) noexcept : Ptr((uintptr_t)(value)) {}
        inline auto operator-> () const noexcept { return (T *)((void *)(*this)); }
    };

    struct Process {
    private:
        void *handle_ = nullptr;
        mutable PtrStorage base_ = {};
        mutable uint32_t checksum_ = {};

    public:
        inline Process() noexcept = default;
        Process(void *ahandle) noexcept;
        Process(uint32_t pid) noexcept;
        Process(const Process &) = delete;
        inline Process(Process &&other) noexcept {
            std::swap(handle_, other.handle_);
            std::swap(base_, other.base_);
            std::swap(checksum_, other.checksum_);
        }
        Process &operator=(const Process &) = delete;
        inline Process &operator=(Process &&other) noexcept {
            std::swap(handle_, other.handle_);
            std::swap(base_, other.base_);
            std::swap(checksum_, other.checksum_);
            return *this;
        }
        ~Process() noexcept;
        explicit inline operator bool() const noexcept { return handle_; }
        inline bool operator!() const noexcept { return !handle_; }

        static std::optional<Process> Find(char const *name) noexcept;

        PtrStorage Base() const;

        uint32_t Checksum() const;

        std::vector<char> Dump() const;

        void WaitInitialized(uint32_t timeout = 60 * 1000) const;

        bool WaitExit(uint32_t timeout = 2 * 60 * 60 * 1000) const;

        void WaitMemoryNonZero(void *addr, uint32_t delay = 1, uint32_t timeout = 60 * 1000) const;

        void ReadMemory(void *address, void *dest, size_t size) const;

        void WriteMemory(void* address, void const* src, size_t size) const;

        void MarkMemoryExecutable(void* address, size_t size) const;

        void *AllocateMemory(size_t size) const;

        inline void WaitNonZero(Ptr<void> address, uint32_t delay = 1,
                                uint32_t timeout = 60 * 1000) const {
            WaitMemoryNonZero(static_cast<void *>(address), delay, timeout);
        }

        template<typename T>
        inline void Read(Ptr<T> address, T& dest, size_t count = 1) const {
            ReadMemory(static_cast<void*>(address), &dest, count * sizeof(T));
        }

        template<typename T>
        inline void Write(Ptr<T> address, T const& src, size_t count = 1) const {
            WriteMemory(static_cast<void*>(address), &src, count * sizeof(T));
        }

        template <typename T>
        inline void Copy(Ptr<T> src, Ptr<T> dest) const {
            T data;
            ReadMemory(src, data);
            WriteMemory(dest, data);
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

        inline PtrStorage Rebase(PtrStorage offset) const { return offset + Base(); }

        inline PtrStorage Debase(PtrStorage offset) const { return offset - Base(); }
    };
}

#pragma once
#include <lol/common.hpp>
#include <lol/fs.hpp>
#include <optional>

namespace lol::patcher {
    template <typename T>
    struct Ptr;

    template <typename T>
    Ptr(T *) -> Ptr<T>;

    using PtrStorage = uint64_t;

    template <>
    struct Ptr<void> {
        PtrStorage storage = {};
        inline Ptr() noexcept = default;
        inline Ptr(Ptr const &) noexcept = default;
        inline Ptr(Ptr &&) noexcept = default;
        inline Ptr &operator=(Ptr const &) noexcept = default;
        inline Ptr &operator=(Ptr &&) noexcept = default;
        inline Ptr(uintptr_t value) noexcept : storage((PtrStorage)(value)) {}
        explicit inline Ptr(void *value) noexcept : Ptr((uintptr_t)(value)) {}
        explicit inline operator void *() const noexcept { return (void *)((uintptr_t)(storage)); }
        explicit inline operator PtrStorage() const noexcept { return storage; }
    };

    template <typename T>
    struct Ptr : Ptr<void> {
        static_assert(!std::is_pointer_v<T>);
        using Ptr<void>::Ptr;
        using Ptr<void>::operator void *;
        using Ptr<void>::operator PtrStorage;
        inline Ptr(T *value) noexcept : Ptr((uintptr_t)(value)) {}
        inline auto operator->() const noexcept { return (T *)((void *)(*this)); }
    };

    struct Process {
    private:
        void *handle_ = nullptr;
        mutable PtrStorage base_ = {};
        mutable std::filesystem::path path_ = {};
        mutable uint32_t pid_ = {};

        Process(void *handle, std::uint32_t pid) noexcept : handle_(handle), pid_(pid) {}

    public:
        Process() noexcept;

        Process(Process const &) = delete;
        Process &operator=(Process const &) = delete;

        Process(Process &&other) noexcept;
        Process &operator=(Process &&other) noexcept;

        ~Process() noexcept;

        explicit inline operator bool() const noexcept { return handle_; }

        inline bool operator!() const noexcept { return !handle_; }

        static auto Open(std::uint32_t pid) -> Process;

        static auto FindPid(char const *name) -> std::uint32_t;

        static auto FindPidWindow(char const *window) -> std::uint32_t;

        auto Base() const -> PtrStorage;

        auto TryBase() const noexcept -> std::optional<PtrStorage>;

        auto Path() const -> fs::path;

        auto Dump() const -> std::vector<char>;

        auto WaitInitialized(uint32_t timeout = 1) const noexcept -> bool;

        auto IsExited() const noexcept -> bool;

        auto TryReadMemory(void *address, void *dest, size_t size) const noexcept -> bool;

        auto ReadMemory(void *address, void *dest, size_t size) const -> void;

        auto WriteMemory(void *address, void const *src, size_t size) const -> void;

        auto MarkMemoryExecutable(void *address, size_t size) const -> void;

        auto MarkMemoryWritable(void *address, size_t size) const -> void;

        auto AllocateMemory(size_t size) const -> void *;

        template <typename T>
        inline auto TryRead(Ptr<T> address) const noexcept -> std::optional<T> {
            auto value = T{};
            if (!TryReadMemory(static_cast<void *>(address), &value, sizeof(value))) {
                return std::nullopt;
            }
            return value;
        }

        template <typename T>
        inline auto Read(Ptr<T> address) const -> T {
            auto dest = T{};
            ReadMemory(static_cast<void *>(address), &dest, sizeof(T));
            return dest;
        }

        template <typename T>
        inline auto Write(Ptr<T> address, T const &src) const -> void {
            WriteMemory(static_cast<void *>(address), &src, sizeof(T));
        }

        template <typename T>
        inline auto Allocate() const -> Ptr<T> {
            return static_cast<Ptr<T>>(AllocateMemory(sizeof(T)));
        }

        template <typename T>
        inline auto MarkWritable(Ptr<T> address) const -> void {
            MarkMemoryWritable(static_cast<void *>(address), sizeof(T));
        }

        template <typename T>
        inline auto MarkExecutable(Ptr<T> address) const -> void {
            MarkMemoryExecutable(static_cast<void *>(address), sizeof(T));
        }

        inline auto Rebase(PtrStorage offset) const -> PtrStorage { return offset + Base(); }

        inline auto Debase(PtrStorage offset) const -> PtrStorage { return offset - Base(); }

        template <typename T>
        inline auto Rebase(PtrStorage offset) const -> Ptr<T> {
            return Ptr<T>{offset + Base()};
        }
    };
}

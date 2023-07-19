#pragma once
#include <array>
#include <chrono>
#include <cstdint>
#include <stdexcept>

namespace lol {
    template <std::size_t S>
    struct Intervals {
        Intervals() = default;

        template <typename... T>
        Intervals(T... intervals)
            : intervals{static_cast<std::int64_t>(
                  std::chrono::duration_cast<std::chrono::milliseconds>(intervals).count())...} {}

        std::array<std::int64_t, S> intervals{1.0};
    };

    template <typename... T>
    Intervals(T...) -> Intervals<sizeof...(T)>;

    Intervals() -> Intervals<1>;

    struct PatcherTimeout : std::runtime_error {
        using std::runtime_error::runtime_error;
    };

    template <std::size_t S>
    inline auto run_until(auto deadline, Intervals<S> intervals, char const* what, auto&& func) {
        auto total = static_cast<std::int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(deadline).count());
        for (auto interval : intervals.intervals) {
            auto slice = total / S;
            while (slice > 0) {
                if (auto result = func()) {
                    return result;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds{interval});
                slice -= interval;
            }
        }
        throw PatcherTimeout(std::string("Timed out: ") + what);
    }
}
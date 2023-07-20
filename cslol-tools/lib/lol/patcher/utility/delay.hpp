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

    template <std::size_t S>
    inline auto run_until_or(auto deadline, Intervals<S> intervals, auto&& poll, auto&& fail) {
        auto total = static_cast<std::int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(deadline).count());
        for (auto interval : intervals.intervals) {
            auto slice = total / S;
            while (slice > 0) {
                if (auto result = poll()) {
                    return result;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds{interval});
                slice -= interval;
            }
        }
        return fail();
    }
}
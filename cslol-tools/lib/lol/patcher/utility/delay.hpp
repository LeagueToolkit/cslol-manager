#pragma once
#include <array>
#include <chrono>
#include <cstdint>
#include <stdexcept>

#include "lol/common.hpp"

namespace lol {
    template <std::size_t S>
    struct Intervals {
        Intervals() = default;

        template <typename... T>
        Intervals(T... intervals)
            : intervals{static_cast<std::int64_t>(
                  std::chrono::duration_cast<std::chrono::milliseconds>(intervals).count())...} {}

        std::array<std::int64_t, S> intervals{1};
    };

    template <typename... T>
    Intervals(T...) -> Intervals<sizeof...(T)>;

    Intervals() -> Intervals<1>;

    template <typename D, std::size_t S, typename P, typename F>
    inline auto run_until_or(D deadline, Intervals<S> intervals, P&& poll, F&& fail) {
        auto total = static_cast<std::int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(deadline).count());
        for (auto interval : intervals.intervals) {
            auto slice = total / S;
            while (slice > 0) {
                if (auto result = poll()) {
                    return result;
                }
                sleep_ms(interval);
                slice -= interval;
            }
        }
        return fail();
    }
}
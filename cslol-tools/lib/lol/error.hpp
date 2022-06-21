#pragma once
#include <lol/common.hpp>
#include <stdexcept>
#include <string>
#include <system_error>

#ifdef _MSC_VER
#    define __PRETTY_FUNCTION__ __FUNCTION__
#endif

#define lol_paste_impl(x, y) x##y
#define lol_paste(x, y) lol_paste_impl(x, y)

#define lol_trace_func(...)                                                                \
    ::lol::error::trace lol_paste(trace_, __LINE__) {                                      \
        [&, func = __PRETTY_FUNCTION__] { ::lol::error::stack_push(func, ##__VA_ARGS__); } \
    }

#define lol_trace_var(fstr, name) fmt::format("{} = " fstr, #name, name)

#define lol_throw_msg(fstr, ...) ::lol::error::raise(::fmt::format(fstr, ##__VA_ARGS__))

#define lol_throw_if_msg(cond, fstr, ...)                                          \
    do {                                                                           \
        [[unlikely]] if (auto expr = cond) { lol_throw_msg(fstr, ##__VA_ARGS__); } \
    } while (false)

#define lol_throw_if(cond)                                                             \
    do {                                                                               \
        [[unlikely]] if (auto expr = cond) { lol_throw_msg("{} -> {}", #cond, expr); } \
    } while (false)


namespace lol::error {
    [[noreturn]] extern auto raise(std::string const &what) -> void;

    extern auto stack() noexcept -> std::string &;

    extern auto stack_trace() noexcept -> std::string;

    extern auto path_sanitize(std::string str) noexcept -> std::string;

    template <typename... Args>
    inline auto stack_push(std::string_view func, Args &&...args) noexcept -> void {
        auto &stack_ref = stack();
        stack_ref.append("\n  ").append(func.substr(0, func.find_first_of("("))).append(":");
        (stack_ref.append("\n    ").append(std::forward<Args>(args)), ...);
    }

    template <typename Func>
    struct trace : private Func {
        inline trace(Func &&func) noexcept : Func(std::forward<Func>(func)) {}
        inline ~trace() noexcept { [[unlikely]] if (std::uncaught_exceptions()) Func::operator()(); }
    };

    constexpr auto fix_func(std::string_view func) noexcept -> std::string_view {
        return func.substr(0, func.find_first_of("("));
    }
}

template <>
struct fmt::formatter<std::error_code> : formatter<std::string> {
    template <typename FormatContext>
    auto format(std::error_code const &code, FormatContext &ctx) {
        return formatter<std::string>::format(lol::error::path_sanitize(code.message()), ctx);
    }
};

template <>
struct fmt::formatter<std::exception> : formatter<std::string> {
    template <typename FormatContext>
    auto format(std::exception const &ex, FormatContext &ctx) {
        return formatter<std::string>::format(lol::error::path_sanitize(ex.what()), ctx);
    }
};
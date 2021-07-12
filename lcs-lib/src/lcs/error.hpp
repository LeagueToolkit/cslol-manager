#ifndef LCS_ERROR_HPP
#define LCS_ERROR_HPP
#include <string>
#include <utility>
#include <vector>
#include "string.hpp"

#define lcs_paste_impl(x, y) x ## y
#define lcs_paste(x, y) lcs_paste_impl(x, y)
#define lcs_assert_msg(msg, ...) do {                     \
        if(!(__VA_ARGS__)) {                              \
            ::LCS::throw_error(msg);                      \
        }                                                 \
    } while(false)
#define lcs_assert(...) do {                              \
        if(!(__VA_ARGS__)) {                              \
            ::LCS::throw_error(#__VA_ARGS__);             \
        }                                                 \
    } while(false)
#define lcs_rethrow(...)                                  \
    [&]() {                                               \
        try {                                             \
            return __VA_ARGS__;                           \
        } catch (...) {                                   \
            ::LCS::push_error_msg(__VA_ARGS__);           \
            throw;                                        \
        }                                                 \
    }()

#ifdef _MSC_VER
#define __PRETTY_FUNCTION__ __FUNCTION__
#endif

#define lcs_trace_var(name) u8"\n\t" #name " = ", name
#define lcs_trace_func(...) ::LCS::ErrorTrace lcs_paste(trace_,__LINE__) {                      \
    [&, func = __PRETTY_FUNCTION__, line = __LINE__] () {                                       \
        ::LCS::push_error_msg(func_to_u8string(func), u8":", line, u8":", __VA_ARGS__);         \
    }                                                                                           \
}
#define lcs_hint(...) \
    ::LCS::ErrorTrace lcs_paste(hint_,__LINE__) { [&] { ::LCS::push_hint_msg(__VA_ARGS__); } }

#define lcs_trace_func_hint(...) lcs_trace_func(__VA_ARGS__); lcs_hint(__VA_ARGS__)

namespace LCS {
    [[noreturn]] extern void throw_error(char const* msg);

    extern std::u8string& error_stack() noexcept;
    extern std::u8string error_stack_trace() noexcept;

    extern std::u8string& hint_stack() noexcept;
    extern std::u8string hint_stack_trace() noexcept;

    template<typename...Args>
    inline void push_error_msg(Args&&...args) noexcept {
        auto& stack = error_stack();
        std::u8string msg = u8"\n";
        ((msg += ::LCS::to_u8string(std::forward<Args>(args))), ...);
        stack.insert(stack.begin(), msg.begin(), msg.end());
    }

    template<typename...Args>
    inline void push_hint_msg(Args&&...args) noexcept {
        auto& stack = hint_stack();
        std::u8string msg = u8"\n";
        ((msg += ::LCS::to_u8string(std::forward<Args>(args))), ...);
        stack.insert(stack.begin(), msg.begin(), msg.end());
    }

    template<typename Func>
    struct ErrorTrace : Func {
        inline ErrorTrace(Func&& func) noexcept : Func(std::move(func)) {}
        inline ~ErrorTrace() noexcept {
            if (std::uncaught_exceptions()) {
                Func::operator()();
            }
        }
    };

    extern void error_print(std::runtime_error const& error) noexcept;
}
#endif // LCS_ERROR_HPP

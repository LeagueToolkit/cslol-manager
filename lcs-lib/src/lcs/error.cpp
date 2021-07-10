#include "error.hpp"
#include <stdexcept>
#ifdef WIN32
#include <cwchar>
#endif

using namespace LCS;

[[noreturn]] void LCS::throw_error(char const* msg) {
    throw std::runtime_error(msg);
}

std::u8string& LCS::error_stack() noexcept {
    thread_local std::u8string instance = {};
    return instance;
}

std::u8string LCS::error_stack_trace() noexcept {
    auto& ss = error_stack();
    std::u8string message = ss;
    ss.clear();
    return message;
}

std::u8string& LCS::hint_stack() noexcept {
    thread_local std::u8string instance = {};
    return instance;
}

std::u8string LCS::hint_stack_trace() noexcept {
    auto& ss = hint_stack();
    std::u8string message = ss;
    ss.clear();
    return message;
}

void LCS::error_print(std::runtime_error const& error) noexcept {
    auto stack_trace = error_stack_trace();
    // FIXME: convert u8string to u16string and wprint
    printf("Error: %s\n%s\n", error.what(), reinterpret_cast<char const*>(stack_trace.c_str()));
}

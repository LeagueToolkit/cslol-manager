#include "error.hpp"
#include <stdexcept>


using namespace LCS;

[[noreturn]] void LCS::throw_error(char const* msg) {
    throw std::runtime_error(msg);
}

std::basic_stringstream<char8_t>& LCS::error_stack() noexcept {
    thread_local std::basic_stringstream<char8_t> instance = {};
    return instance;
}

std::u8string LCS::error_stack_trace() noexcept {
    auto& ss = error_stack();
    std::u8string message = ss.str();
    ss.str(u8"");
    ss.clear();
    return message;
}

std::string LCS::error_stack_trace_cstr(std::string message) noexcept {
    auto result = error_stack_trace();
    return message + std::string { result.begin(), result.end() };
}

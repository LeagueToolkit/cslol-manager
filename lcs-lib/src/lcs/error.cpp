#include "error.hpp"
#include <stdexcept>


using namespace LCS;

[[noreturn]] void LCS::throw_error(char const* msg) {
    throw std::runtime_error(msg);
}

std::stringstream& LCS::error_stack() noexcept {
    thread_local std::stringstream instance = {};
    return instance;
}

std::string LCS::error_stack_trace(std::string message) noexcept {
    auto& ss = error_stack();
    message.append(ss.str());
    ss.str("");
    ss.clear();
    return message;
}

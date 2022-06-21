#include <exception>
#include <lol/error.hpp>
#include <lol/fs.hpp>

using namespace lol;

[[noreturn]] auto error::raise(std::string const& what) -> void { throw std::runtime_error(what); }

auto error::stack() noexcept -> std::string& {
    thread_local std::string instance = {};
    return instance;
}

auto error::stack_trace() noexcept -> std::string {
    auto& ss = stack();
    std::string message = ss;
    ss.clear();
    return message;
}

auto error::path_sanitize(std::string str) noexcept -> std::string {
    if (auto const& cd = fs::current_path_str(); !cd.empty()) {
        for (auto i = str.find(cd); i != std::string::npos; i = str.find(cd)) {
            str.replace(i, cd.size(), "./");
        }
    }
    if (auto const& home = fs::home_path_str(); !home.empty()) {
        for (auto i = str.find(home); i != std::string::npos; i = str.find(home)) {
            str.replace(i, home.size(), "~/");
        }
    }
    return str;
}
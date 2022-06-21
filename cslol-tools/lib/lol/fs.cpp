#include <stdlib.h>

#include <lol/error.hpp>
#include <lol/fs.hpp>

using namespace lol;

auto fs::home_path_str() noexcept -> std::string const& {
    static std::string const instance = []() -> std::string {
        auto result = std::string{};
#ifdef _WIN32
        auto homedrive = _wgetenv(L"HOMEDRIVE");
        auto homepath = _wgetenv(L"HOMEPATH");
        if (homedrive && homepath) result = (fs::path(homedrive) / homepath).generic_string();
#else
        auto home = getenv("HOME");
        if (home) result = fs::path(home).generic_string();
#endif
        if (!result.empty() && !result.ends_with('/')) result.push_back('/');
        // std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        return std::string(result.begin(), result.end());
    }();
    return instance;
}

auto fs::current_path_str() noexcept -> std::string const& {
    static std::string const instance = []() -> std::string {
        auto result = std::string{};
        auto errc = std::error_code{};
        result = fs::current_path().generic_string();
        if (!result.empty() && !result.ends_with('/')) result.push_back('/');
        // std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        return std::string(result.begin(), result.end());
        ;
    }();
    return instance;
}

fs::tmp_dir::tmp_dir(fs::path path) {
    lol_trace_func(lol_trace_var("{}", path));
    if (fs::exists(path)) {
        fs::remove_all(path);
    }
    fs::create_directories(path);
    this->path = std::move(path);
}

fs::tmp_dir::~tmp_dir() noexcept {
    if (!path.empty()) {
        std::error_code ec = {};
        fs::remove_all(path, ec);
    }
}

auto fs::tmp_dir::move(fs::path const& dst) -> void {
    lol_trace_func(lol_trace_var("{}", path), lol_trace_var("{}", dst));
    fs::rename(this->path, dst);
    this->path.clear();
}

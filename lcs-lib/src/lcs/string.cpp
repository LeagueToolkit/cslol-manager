#include "string.hpp"

using namespace LCS;

std::u8string LCS::func_to_u8string(char const* func) {
    std::u8string str = to_u8string(func);
    auto const end = str.find_first_of(u8'(');
    if (end != std::u8string::npos) {
        str.resize((size_t)end);
    }
    return str;
}

std::map<std::u8string, std::u8string>& LCS::path_remap() {
    thread_local std::map<std::u8string, std::u8string> instance = {
        { u8"<LCS>", u8"" },
        { u8"<Game>", u8"" },
        { u8"<PWD>", u8"" },
        { u8"<Desktop>", u8"" },
        { u8"<Documents>", u8"" },
        { u8"<Downloads>", u8"" },
        { u8"<Home>", u8"" },
    };
    return instance;
}

std::u8string LCS::to_u8string(fs::path const& from) {
    auto result = from.generic_u8string();
    for (auto const& [remap_to, remap_from]: path_remap()) {
        if (remap_from.empty()) {
            continue;
        }
        if (result.starts_with(remap_from)) {
            return remap_to + u8"/" + result.substr(remap_from.size());
        }
    }
    return result;
}

std::u8string LCS::to_u8string(fs::filesystem_error const& err) {
    constexpr auto extract_path = [] (std::u8string& source) -> fs::path {
        auto const start = source.find_first_of(u8"[\"");
        if (start == std::u8string::npos) {
            return u8"";
        }
        auto const end = source.find_first_of(u8"]\"", start + 1);
        if (end == std::u8string::npos) {
            return u8"";
        }
        auto const before = source.substr(0, start);
        auto const result = source.substr(start + 1, end - (start + 1));
        auto const after = source.substr(end + 1);
        source = before + after;
        return result;
    };
    std::u8string result = to_u8string(err.what());
    fs::path path1 = extract_path(result);
    fs::path path2 = extract_path(result);
    if (!path1.empty()) {
        result += u8"\n\tpath1 = ";
        result += to_u8string(path1);
    }
    if (!path2.empty()) {
        result += u8"\n\tpath2 = ";
        result += to_u8string(path2);
    }
    return result;
}

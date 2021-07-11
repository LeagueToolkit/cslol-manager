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
        { u8"<LOL>", u8"" },
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
            return remap_to + result.substr(remap_from.size());
        }
    }
    return result;
}

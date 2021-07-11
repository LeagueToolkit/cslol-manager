#include "string.hpp"

using namespace LCS;

std::map<std::u8string, std::u8string>& LCS::path_remap() {
    thread_local std::map<std::u8string, std::u8string> instance = {
        { u8"<LCS>", u8"" },
        { u8"<LOL>", u8"" },
        { u8"<PWD>", u8"" },
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

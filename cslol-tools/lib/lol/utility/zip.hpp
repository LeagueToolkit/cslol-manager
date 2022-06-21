#pragma once
#include <lol/common.hpp>
#include <lol/fs.hpp>

namespace lol::utility {
    extern auto zip(fs::path const& src, fs::path const& dst) -> void;
    extern auto unzip(fs::path const& src, fs::path const& dst) -> void;
}

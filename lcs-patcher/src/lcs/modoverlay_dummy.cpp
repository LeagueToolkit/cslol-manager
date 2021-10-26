#include "modoverlay.hpp"

using namespace LCS;

struct ModOverlay::Impl {};

ModOverlay::ModOverlay() : impl_(std::make_unique<Impl>()) {}

ModOverlay::~ModOverlay() = default;

std::string ModOverlay::to_string() const noexcept {
    return "lcs-patcher-dummy-v0";
}

void ModOverlay::from_string(std::string const & str) noexcept {}

void ModOverlay::run(std::function<bool(Message)> update, std::filesystem::path const& profilePath) {
    if (!update(M_DONE)) return;
}

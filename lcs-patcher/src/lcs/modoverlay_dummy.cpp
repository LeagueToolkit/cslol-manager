#include "modoverlay.hpp"

using namespace LCS;


char const ModOverlay::SCHEMA[] = "lcs-overlay-dummy v0";
char const ModOverlay::INFO[] = "lcs-overlay-dummy v0";

struct ModOverlay::Config {};

ModOverlay::ModOverlay() : config_(std::make_unique<Config>()) {}

ModOverlay::~ModOverlay() = default;

std::string ModOverlay::to_string() const noexcept {
    return SCHEMA;
}

void ModOverlay::from_string(std::string const & str) noexcept {}

void ModOverlay::run(std::function<bool(Message)> update, std::filesystem::path const& profilePath) {
    if (!update(M_DONE)) return;
}

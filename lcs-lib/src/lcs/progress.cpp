#include "progress.hpp"
using namespace LCS;

Progress::Progress() noexcept {}

Progress::~Progress() noexcept { }

void Progress::startItem(fs::path const&, std::uint64_t) noexcept {}

void Progress::consumeData(std::uint64_t) noexcept {}

void Progress::finishItem() noexcept {}

ProgressMulti::ProgressMulti() noexcept {}

ProgressMulti::~ProgressMulti() noexcept {}

void ProgressMulti::startMulti (size_t, std::uint64_t) noexcept  {}

void ProgressMulti::finishMulti() noexcept {}

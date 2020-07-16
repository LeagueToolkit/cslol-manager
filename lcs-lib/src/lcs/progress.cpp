#include "progress.hpp"
using namespace LCS;

Progress::Progress() noexcept {}

Progress::~Progress() noexcept { }

void Progress::startItem(fs::path const&, size_t) noexcept {}

void Progress::consumeData(size_t) noexcept {}

void Progress::finishItem() noexcept {}

ProgressMulti::ProgressMulti() noexcept {}

ProgressMulti::~ProgressMulti() noexcept {}

void ProgressMulti::startMulti (size_t, size_t) noexcept  {}

void ProgressMulti::finishMulti() noexcept {}

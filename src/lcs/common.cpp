#include "common.hpp"
using namespace LCS;

Progress::Progress() noexcept {}

Progress::~Progress() noexcept
{

}

void Progress::startItem(fs::path const&, size_t) noexcept {}

void Progress::consumeData(size_t) noexcept {}

void Progress::finishItem() noexcept {}


ProgressMulti::ProgressMulti() noexcept {}

ProgressMulti::~ProgressMulti() noexcept {}

void ProgressMulti::startMulti (size_t, size_t) noexcept  {}

void ProgressMulti::finishMulti() noexcept {}


ConflictError::ConflictError(uint64_t xxhash, fs::path const& orgpath, fs::path const& newpath)
    : std::runtime_error(std::string("Wad conflict!\n\txxhash:")
                         + std::to_string(xxhash) + "\n\torgpath:"
                         + orgpath.generic_string() + "\n\tnewpath:"
                         + newpath.generic_string()) {

}

ConflictError::ConflictError(std::string const& name, fs::path const& orgpath, fs::path const& newpath)
    : std::runtime_error(std::string("Wad conflict!\n\tname:")
                         + name + "\n\torgpath:"
                         + orgpath.generic_string() + "\n\tnewpath:"
                         + newpath.generic_string()) {
}

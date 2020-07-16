#include "wadindexcache.hpp"
#include "error.hpp"
#include <utility>

using namespace LCS;

WadIndexCache::WadIndexCache(LCS::WadIndex const& index) noexcept :
    path_(index.path())
{
    lcs_trace_func();
    lcs_trace("path_: ", path_);
    for(auto const& [name, wad]: index.wads()) {
        wads_.insert_or_assign(name, fs::relative(wad->path(), path_));
    }
    for(auto const& [xxhash, wad]: index.lookup()) {
        lookup_.insert(std::make_pair(xxhash, wad->name()));
    }
}

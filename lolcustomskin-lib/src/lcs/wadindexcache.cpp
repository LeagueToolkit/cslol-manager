#include "wadindexcache.hpp"

using namespace LCS;

WadIndexCache::WadIndexCache(LCS::WadIndex const& index) noexcept :
    path_(index.path())
{
    for(auto const& [name, wad]: index.wads()) {
        wads_.insert_or_assign(name, fs::relative(wad->path(), path_));
    }

    for(auto const& [xxhash, wad]: index.lookup()) {
        lookup_.insert(std::pair { xxhash, wad->name() });
    }
}

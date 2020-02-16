#ifndef LCS_HASHTABLE_HPP
#define LCS_HASHTABLE_HPP
#include "common.hpp"
#include <unordered_map>
#include <optional>

namespace LCS {
    struct HashTable {
        void add_from_file(fs::path const& path);
        inline std::optional<fs::path> find(uint64_t hash) const noexcept {
            if (auto i = hashes_.find(hash); i != hashes_.end()) {
                return i->second;
            }
            return std::nullopt;
        }
    private:
        std::unordered_map<uint64_t, fs::path> hashes_;
    };
}

#endif // HASHTABLE_HPP

#pragma once
#include <lol/common.hpp>
#include <lol/fs.hpp>
#include <unordered_map>

namespace lol::hash {
    //! Container for unhashed
    struct Dict {
        //! Create empty hash dictionary.
        Dict() noexcept {}

        //! Create hash dictionary with reserved size.
        Dict(std::size_t hint_reserve) noexcept : unhashed_(hint_reserve) {}

        //! Load hash dictionary entries.
        auto load(fs::path const &path) noexcept -> bool;

        //! Save hash dictionary entries.
        auto save(fs::path const &path) const -> void;

        //! Get key from hash dictionary.
        template <typename HashType>
        auto get(HashType key) const noexcept -> std::string {
            if (auto i = unhashed_.find((std::uint64_t)key); i != unhashed_.end()) {
                return i->second;
            }
            return {};
        }

        //! Add key to hash dictionary.
        template <typename HashType>
        auto add(HashType key, std::string value) noexcept -> void {
            unhashed_[(std::uint64_t)key] = value;
        }

    protected:
        std::unordered_map<std::uint64_t, std::string> unhashed_;
    };
}

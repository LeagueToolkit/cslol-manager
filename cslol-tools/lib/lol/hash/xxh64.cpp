#include <charconv>
#include <lol/error.hpp>
#include <lol/hash/xxh64.hpp>
#include <xxh3.h>

using namespace lol;
using namespace lol::hash;

Xxh64::Xxh64(std::string_view str) noexcept : value_(XXH64(str.data(), str.size(), 0)) {}

auto Xxh64::from_path(std::string_view str) noexcept -> Xxh64 {
    while (str.starts_with('.') || str.starts_with('/')) str.remove_prefix(1);
    auto tmp = str.substr(0, str.find_first_of('.'));
    if (auto const data = (char const *)tmp.data(); tmp.size() == 16) {
        std::uint64_t value = 0;
        auto const [ptr, ec] = std::from_chars(data, data + tmp.size(), value, 16);
        if (ptr == data + tmp.size() && ec == std::errc{}) {
            return Xxh64(value);
        }
    }
    return Xxh64(str);
}

#include <charconv>
#include <lol/error.hpp>
#include <lol/hash/fnv1a32.hpp>

using namespace lol;
using namespace lol::hash;

Fnv1a32::Fnv1a32(std::string_view str) noexcept {
    std::uint32_t h = 0x811c9dc5u;
    for (std::uint8_t c : str) {
        c = c >= 'A' && c <= 'Z' ? (c - 'A') + 'a' : c;
        h = ((h ^ c) * 0x01000193u) & 0xFFFFFFFFu;
    }
    value_ = h;
}

#include <charconv>
#include <lol/error.hpp>
#include <lol/hash/xxh64.hpp>

using namespace lol;
using namespace lol::hash;

Xxh64::Xxh64(std::string_view str) noexcept {
    std::uint64_t seed = 0;
    auto data = str.data();
    auto const size = str.size();
    auto const end = data + size;
    constexpr std::uint64_t Prime1 = 11400714785074694791U;
    constexpr std::uint64_t Prime2 = 14029467366897019727U;
    constexpr std::uint64_t Prime3 = 1609587929392839161U;
    constexpr std::uint64_t Prime4 = 9650029242287828579U;
    constexpr std::uint64_t Prime5 = 2870177450012600261U;
    constexpr auto Char = [](std::uint8_t c) constexpr->std::uint64_t {
        return c >= 'A' && c <= 'Z' ? (c - 'A') + 'a' : c;
    };
    constexpr auto HalfBlock = [Char](char const *data) constexpr->std::uint64_t {
        return Char(*data) | (Char(*(data + 1)) << 8) | (Char(*(data + 2)) << 16) | (Char(*(data + 3)) << 24);
    };
    constexpr auto Block = [Char](char const *data) constexpr->std::uint64_t {
        return Char(*data) | (Char(*(data + 1)) << 8) | (Char(*(data + 2)) << 16) | (Char(*(data + 3)) << 24) |
               (Char(*(data + 4)) << 32) | (Char(*(data + 5)) << 40) | (Char(*(data + 6)) << 48) |
               (Char(*(data + 7)) << 56);
    };
    constexpr auto ROL = [](std::uint64_t value, int ammount) -> std::uint64_t { return std::rotl(value, ammount); };
    std::uint64_t result = 0;
    if (size >= 32u) {
        std::uint64_t s1 = seed + Prime1 + Prime2;
        std::uint64_t s2 = seed + Prime2;
        std::uint64_t s3 = seed;
        std::uint64_t s4 = seed - Prime1;
        for (; data + 32 <= end; data += 32) {
            s1 = ROL(s1 + Block(data) * Prime2, 31) * Prime1;
            s2 = ROL(s2 + Block(data + 8) * Prime2, 31) * Prime1;
            s3 = ROL(s3 + Block(data + 16) * Prime2, 31) * Prime1;
            s4 = ROL(s4 + Block(data + 24) * Prime2, 31) * Prime1;
        }
        result = ROL(s1, 1) + ROL(s2, 7) + ROL(s3, 12) + ROL(s4, 18);
        result ^= ROL(s1 * Prime2, 31) * Prime1;
        result = result * Prime1 + Prime4;
        result ^= ROL(s2 * Prime2, 31) * Prime1;
        result = result * Prime1 + Prime4;
        result ^= ROL(s3 * Prime2, 31) * Prime1;
        result = result * Prime1 + Prime4;
        result ^= ROL(s4 * Prime2, 31) * Prime1;
        result = result * Prime1 + Prime4;
    } else {
        result = seed + Prime5;
    }
    result += (std::uint64_t)size;
    for (; data + 8 <= end; data += 8) {
        result ^= ROL(Block(data) * Prime2, 31) * Prime1;
        result = ROL(result, 27) * Prime1 + Prime4;
    }
    for (; data + 4 <= end; data += 4) {
        result ^= HalfBlock(data) * Prime1;
        result = ROL(result, 23) * Prime2 + Prime3;
    }
    for (; data != end; ++data) {
        result ^= Char(*data) * Prime5;
        result = ROL(result, 11) * Prime1;
    }
    result ^= result >> 33;
    result *= Prime2;
    result ^= result >> 29;
    result *= Prime3;
    result ^= result >> 32;
    value_ = result;
}

auto Xxh64::from_path(std::string_view str) noexcept -> Xxh64 {
    while (str.starts_with('.') || str.starts_with('/')) str.remove_prefix(1);
    auto tmp = str.substr(0, str.find_first_of('.'));
    if (auto const data = (char const *)tmp.data(); tmp.size() == 16) {
        if (std::uint64_t value = 0;
            std::from_chars(data, data + tmp.size(), value, 16) == std::from_chars_result{data + tmp.size()}) {
            return Xxh64(value);
        }
    }
    return Xxh64(str);
}

/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org/>
✂--------------------------------[ Cut here ]----------------------------------
*/
#pragma once
#include <array>
#include <cstring>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <tuple>

namespace ppp {
    template <std::size_t N>
    struct pattern {
        std::uint16_t op_count = {};
        std::uint16_t ops[N] = {};
        std::uint16_t group_count = {};
        std::uint16_t group_start[N] = {};
        std::uint16_t group_size[N] = {};
        char group_tag[N] = {};
    private:
        static constexpr int32_t parse_hex(char c) noexcept  {
            if (c >= '0' && c <= '9') {
                return static_cast<uint8_t>(c - '0');
            } else if (c >= 'a' && c <= 'f') {
                return static_cast<uint8_t>(c - 'a' + 10);
            } else if (c >= 'A' && c <= 'F') {
                return static_cast<uint8_t>(c - 'A' + 10);
            } else {
                return -1;
            }
        };

        constexpr void parse_cap_group(char const*& next) {
            std::uint16_t group_index = group_count++;
            std::uint16_t group_op_start = op_count;
            for (;;) {
                if (char cur = *next++; cur == '\0') {
                    if (group_index != 0) {
                        throw "Unterminated capture group!";
                    }
                    break;
                } else if (cur == ']') {
                    if (group_index == 0) {
                        throw "Stray capture group terminator!";
                    }
                    break;
                } else if (cur == '[') {
                    parse_cap_group(next);
                } else if (cur == 's' || cur == 'u' || cur == 'i' || cur == 'o' || cur == 'r') {
                    if (*next != '[') {
                        throw "Capture group tag must be followed by capture group!";
                    }
                    group_tag[group_count] = cur;
                } else if (cur == '.') {
                    ops[op_count++] = 0x100u;
                } else if (cur == '?') {
                    ops[op_count++] = 0x100u;
                    if (*next == '?') {
                        ++next;
                    }
                } else if (auto const first = parse_hex(cur); first != -1) {
                    if (auto const second = parse_hex(*next); second != -1) {
                        ops[op_count++] = static_cast<std::uint16_t>((first << 4) | second);
                        ++next;
                    } else {
                        ops[op_count++] = static_cast<std::uint16_t>(first);
                    }
                } else if (cur == '`' || cur == '\'' || cur == '"') {
                    for (;;) {
                        if (char const escaped = *next++; escaped == '\0') {
                            throw "Unexpected end of string inside string literal";
                        } else if (escaped == cur) {
                            break;
                        } else {
                            ops[op_count++] = static_cast<uint8_t>(escaped);
                        }
                    }
                } else if (cur == '#' || cur == ';') {
                    while (*next && *next != '\n') {
                        ++next;
                    }
                } else if (cur == ' ' || cur == '\t' || cur == '\r' || cur == '\n') {
                } else {
                    throw "Invalid pattern character!";
                }
            }
            std::uint16_t group_op_count = op_count - group_op_start;
            if (group_op_count == 0) {
                throw "Empty capture group!";
            }
            group_start[group_index]= group_op_start;
            group_size[group_index] = group_op_count;
        }
    public:
        constexpr pattern(char const (&str)[N]) {
            char const* iter = str;
            parse_cap_group(iter);
        }
    };

    struct result {
        std::span<char const> data = {};
        std::uint64_t offset = {};
    };

    template <char TAG, std::uint16_t START, std::uint16_t SIZE>
    struct group;

    template <std::uint16_t START, std::uint16_t SIZE>
    struct group<'\0', START, SIZE> {
        static constexpr std::span<char const> pack(result result) noexcept {
            auto const data = result.data.subspan<START, SIZE>();
            return data;
        }
    };

    template <std::uint16_t START, std::uint16_t SIZE>
    struct group<'s', START, SIZE> {
        static constexpr std::string_view pack(result result) noexcept {
            auto const data = result.data.subspan<START, SIZE>();
            return { data.data(), data.size() };
        }
    };

    template <std::uint16_t START>
    struct group<'u', START, 1> {
        static constexpr std::uint8_t pack(result result) noexcept {
            auto const data = result.data.subspan(START, 1);
            return static_cast<std::uint8_t>(data[0]);
        }
    };

    template <std::uint16_t START>
    struct group<'u', START, 2> {
        static constexpr std::uint16_t pack(result result) noexcept {
            auto const data = result.data.subspan<START, 2>();
            std::uint16_t number = 0;
            number |= static_cast<std::uint16_t>(static_cast<std::uint8_t>(data[0]));
            number |= static_cast<std::uint16_t>(static_cast<std::uint8_t>(data[1])) << 8;
            return number;
        }
    };

    template <std::uint16_t START>
    struct group<'u', START, 4> {
        static constexpr std::uint32_t pack(result result) noexcept {
            auto const data = result.data.subspan<START, 4>();
            std::uint32_t number = 0;
            number |= static_cast<std::uint32_t>(static_cast<std::uint8_t>(data[0]));
            number |= static_cast<std::uint32_t>(static_cast<std::uint8_t>(data[1])) << 8;
            number |= static_cast<std::uint32_t>(static_cast<std::uint8_t>(data[2])) << 16;
            number |= static_cast<std::uint32_t>(static_cast<std::uint8_t>(data[3])) << 24;
            return number;
        }
    };

    template <std::uint16_t START>
    struct group<'u', START, 8> {
        static constexpr std::uint64_t pack(result result) noexcept {
            auto const data = result.data.subspan<START, 4>();
            std::uint64_t number = 0;
            number |= static_cast<std::uint64_t>(static_cast<std::uint8_t>(data[0]));
            number |= static_cast<std::uint64_t>(static_cast<std::uint8_t>(data[1])) << 8;
            number |= static_cast<std::uint64_t>(static_cast<std::uint8_t>(data[2])) << 16;
            number |= static_cast<std::uint64_t>(static_cast<std::uint8_t>(data[3])) << 24;
            number |= static_cast<std::uint64_t>(static_cast<std::uint8_t>(data[4])) << 32;
            number |= static_cast<std::uint64_t>(static_cast<std::uint8_t>(data[5])) << 40;
            number |= static_cast<std::uint64_t>(static_cast<std::uint8_t>(data[6])) << 48;
            number |= static_cast<std::uint64_t>(static_cast<std::uint8_t>(data[7])) << 56;
            return number;
        }
    };

    template <std::uint16_t START, std::uint16_t SIZE>
    struct group<'i', START, SIZE> {
        static constexpr auto pack(result result) noexcept {
            auto const number = group<'u', START, SIZE>::pack(result);
            using T = std::make_signed_t<std::remove_cvref_t<decltype(number)>>;
            return static_cast<T>(number);
        }
    };

    template <std::uint16_t START, std::uint16_t SIZE>
    struct group<'o', START, SIZE> {
        static constexpr auto pack(result result) noexcept {
            return result.offset + START;
        }
    };

    template <std::uint16_t START, std::uint16_t SIZE>
    struct group<'r', START, SIZE> {
        static constexpr auto pack(result result) noexcept {
            auto const offset = result.offset + START + SIZE;
            auto const relative = group<'i', START, SIZE>::pack(result);
            return offset + relative;
        }
    };

    template <typename...GROUPS>
    struct group_list {
        static constexpr auto pack(result result) noexcept {
            return std::make_tuple(GROUPS::pack(result)...);
        }
    };

    template <typename GROUPS, std::uint16_t...OPS>
    struct iterator {
    private:
        result result_ = {};

        constexpr iterator() noexcept = default;

        constexpr iterator(result result) noexcept : result_(result) {}
    public:
        static constexpr iterator find(std::span<char const> data, std::uint64_t base) noexcept {
            for (std::size_t i = 0; (data.size() - i) >= sizeof...(OPS); ++i, ++base) {
                auto j = data.data() + i;
                if (((((std::uint8_t)*j++ == (std::uint8_t)OPS) || OPS > 0xFF) && ...)) {
                    return { { data.subspan(i), base } };
                }
            }
            return {};
        }

        constexpr operator bool() const noexcept {
            return !result_.data.empty();
        }

        constexpr auto begin() const noexcept {
            return *this;
        }

        constexpr auto end() const noexcept {
            return false;
        }

        constexpr auto operator*() const noexcept {
            return GROUPS::pack(result_);
        }

        constexpr iterator& operator++() noexcept {
            if (!result_.data.empty()) {
                auto const next_data = result_.data.subspan(sizeof...(OPS));
                auto const next_offset = result_.offset + sizeof...(OPS);
                *this = find(next_data, next_offset);
            }
            return *this;
        }

        static constexpr auto matches(std::span<char const> data, std::uint64_t offset) noexcept {
            using result_t = std::optional<decltype(GROUPS::pack(result{}))>;
            if (data.size() >= sizeof...(OPS)) {
                auto j = data.data();
                if (((((std::uint8_t)*j++ == (std::uint8_t)OPS) || OPS > 0xFF) && ...)) {
                    return result_t { GROUPS::pack({ data, offset }) };
                }
            }
            return result_t {};
        }
    };

    template <auto...rest>
    constexpr auto any(std::span<char const> data, std::uint64_t offset) noexcept {
        auto out = std::optional<decltype((*rest(data, offset),...))> {};
        while (!data.empty() && !((out = decltype(rest(data, offset))::matches(data, offset)) || ...)) {
            data = data.subspan(1);
            ++offset;
        }
        return out;
    }
}


#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-string-literal-operator-template"
template<class Char = char, Char...c>
consteval auto operator"" _pattern() {
    constexpr auto pat = ppp::pattern<sizeof...(c) + 1>({ c..., '\0' });
#pragma clang diagnostic pop
#else
template<ppp::pattern pat>
consteval auto operator"" _pattern() noexcept {
#endif
    return []<std::size_t...O, std::size_t...G> (std::index_sequence<O...>, std::index_sequence<G...>) {
        using groups_t = ppp::group_list<ppp::group<pat.group_tag[G], pat.group_start[G], pat.group_size[G]>...>;
        using result_t = ppp::iterator<groups_t, pat.ops[O]...>;
        return &result_t::find;
    } (std::make_index_sequence<pat.op_count>(), std::make_index_sequence<pat.group_count>());
}


#pragma once
#include <cinttypes>
#include <charconv>
#include <cstddef>
#include <string>

namespace LCS::LineConfigImpl {
    template <std::size_t S>
    struct CTStr {
        char data[S];
        constexpr CTStr(char const (&src)[S]) noexcept : data{} {
            for (std::size_t i = 0; i != S; ++i) {
                data[i] = src[i];
            }
        }
        constexpr auto operator<=>(CTStr const&) const noexcept = default;
    };

    template <typename T, CTStr name>
    struct Field {
        static constexpr auto NAME = name;
        T value = {};
        template <CTStr what> requires(name == what)
        constexpr auto get() && noexcept {
            return value;
        }
        template <CTStr what> requires(name == what)
        constexpr auto& get() & noexcept {
            return value;
        }
        template <CTStr what> requires(name == what)
        constexpr auto const& get() const & noexcept {
            return value;
        }
    };

    template <typename T, CTStr magic, CTStr...names>
    struct List : Field<T, names>... {
        using Field<T, names>::get...;

        inline bool check() const noexcept {
            return (static_cast<Field<T, names> const*>(this)->value && ...);
        }

        inline std::string to_string() const noexcept {
            std::string buffer = {};
            auto print_str = [&buffer] (std::string_view src) mutable {
                buffer += src;
            };
            auto print_hex = [&buffer] (auto val) mutable {
                constexpr std::size_t hex_digits = sizeof(val) * 2;
                char tmp[32] = { "0000000000000000" };
                auto const ec_ptr = std::to_chars(tmp + hex_digits, tmp + sizeof(tmp), val, 16);
                buffer += std::string_view { ec_ptr.ptr - hex_digits, hex_digits };
            };
            print_str(magic.data);
            ((print_str(" ")
                    , print_str(names.data)
                    , print_str(":")
                    , print_hex(static_cast<Field<T, names> const*>(this)->value)
                    ), ...);
            return buffer;
        }

        inline void from_string(std::string_view src) & noexcept {
            std::string_view buffer = {};
            bool error = false;
            auto skip_spaces = [&buffer] () mutable {
                while (buffer.starts_with(" ")) {
                    buffer.remove_prefix(1);
                }
            };
            auto parse_str = [&buffer, &error] (std::string_view src) mutable {
                if (buffer.starts_with(src)) {
                    buffer.remove_prefix(src.size());
                } else {
                    error = true;
                }
            };
            auto parse_hex = [&buffer, &error] (auto& val) mutable {
                val = 0;
                auto const start = buffer.data();
                auto const end = buffer.data() + buffer.size();
                auto const ec_ptr = std::from_chars(start, end, val, 16);
                buffer = { ec_ptr.ptr, end };
                if (ec_ptr.ec != std::errc{}) {
                    error = true;
                }
            };
            parse_str(magic.data);
            ((parse_str(" ")
                    , skip_spaces()
                    , parse_str(names.data)
                    , skip_spaces()
                    , parse_str(":")
                    , skip_spaces()
                    , parse_hex(static_cast<Field<T, names>*>(this)->value)
                    ), ...);
            if (error) {
                *this = {};
            }
        }
    };
}

namespace LCS {
    template <typename T, LineConfigImpl::CTStr...names>
    using LineConfig = LineConfigImpl::List<T, names...>;
}

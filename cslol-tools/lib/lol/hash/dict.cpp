#include <charconv>
#include <lol/error.hpp>
#include <lol/hash/dict.hpp>
#include <lol/io/bytes.hpp>
#include <lol/io/file.hpp>

using namespace lol;
using namespace lol::hash;

auto Dict::load(fs::path const &path) noexcept -> bool {
    try {
        auto src = io::Bytes::from_file(path);
        auto i = src.data();
        auto const end = src.data() + src.size();
        while (i != end) {
            while (i != end && std::isspace(*i)) ++i;
            std::uint64_t h = {};
            if (auto [ptr, ec] = std::from_chars(i, end, h, 16); ec == std::errc{}) {
                i = ptr;
                while (i != end && std::isspace(*i)) ++i;
                auto start = i;
                while (i != end && *i != '\r' && *i != '\n') ++i;
                unhashed_[h] = std::string_view(start, (std::size_t)(i - start));
            } else {
                while (i != end && *i != '\r' && *i != '\n') ++i;
            }
        }
        return true;
    } catch (std::exception const &) {
        error::stack().clear();
        return false;
    }
}

auto Dict::save(fs::path const &path) const -> void {
    auto dst = io::File::create(path);
    auto pos = 0;
    for (auto const &[h, name] : unhashed_) {
        auto line = fmt::format("{:016X} {}\n", h, name);
        dst.write(pos, line.data(), line.size());
        pos += line.size();
    }
    dst.resize(pos);
}

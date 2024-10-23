#include <xxh3.h>

#include <array>
#include <lol/error.hpp>
#include <lol/io/file.hpp>
#include <lol/utility/magic.hpp>
#include <lol/wad/entry.hpp>

using namespace lol;
using namespace lol::wad;

auto EntryData::Impl::empty() noexcept -> std::shared_ptr<Impl> const& {
    static auto instance = std::make_shared<Impl>();
    return instance;
}

auto EntryData::from_raw(io::Bytes bytes, std::uint64_t checksum) -> EntryData {
    auto result = EntryData(std::make_shared<Impl>());
    result.impl_->type = EntryType::Raw;
    result.impl_->size_decompressed = bytes.size();
    result.impl_->checksum = checksum;
    result.impl_->bytes = std::move(bytes);
    result.impl_->decompressed = result.impl_;
    return result;
}

auto EntryData::from_link(io::Bytes bytes, std::uint64_t checksum) -> EntryData {
    auto result = EntryData(std::make_shared<Impl>());
    result.impl_->type = EntryType::Link;
    result.impl_->size_decompressed = bytes.size();
    result.impl_->checksum = checksum;
    result.impl_->bytes = std::move(bytes);
    result.impl_->compressed = result.impl_;
    result.impl_->decompressed = result.impl_;
    return result;
}

auto EntryData::from_gzip(io::Bytes bytes, std::size_t decompressed, std::uint64_t checksum) -> EntryData {
    auto result = EntryData(std::make_shared<Impl>());
    result.impl_->type = EntryType::Gzip;
    result.impl_->checksum = checksum;
    result.impl_->size_decompressed = decompressed;
    result.impl_->bytes = std::move(bytes);
    return result;
}

auto EntryData::from_zstd(io::Bytes bytes, std::size_t decompressed, std::uint64_t checksum) -> EntryData {
    auto result = EntryData(std::make_shared<Impl>());
    result.impl_->type = EntryType::Zstd;
    result.impl_->size_decompressed = decompressed;
    result.impl_->checksum = checksum;
    result.impl_->bytes = std::move(bytes);
    result.impl_->compressed = result.impl_;
    return result;
}

auto EntryData::from_zstd_multi(io::Bytes bytes,
                                std::size_t decompressed,
                                std::uint64_t checksum,
                                std::uint8_t subchunk_count,
                                std::uint32_t subchunk_index) -> EntryData {
    auto result = EntryData(std::make_shared<Impl>());
    result.impl_->type = EntryType::ZstdMulti;
    result.impl_->subchunk_count = subchunk_count;
    result.impl_->subchunk_index = subchunk_index;
    result.impl_->size_decompressed = decompressed;
    result.impl_->checksum = checksum;
    result.impl_->bytes = std::move(bytes);
    result.impl_->compressed = result.impl_;
    return result;
}

auto EntryData::from_file(fs::path const& path) -> EntryData { return from_raw(io::Bytes::from_file(path), 0); }

auto EntryData::from_loc(io::Bytes src, EntryLoc const& loc) -> EntryData {
    lol_trace_func(lol_trace_var("{:#x}", loc.offset), lol_trace_var("{:#x}", loc.size));
    auto bytes = src.copy(loc.offset, loc.size);
    switch (loc.type) {
        case EntryType::Raw:
            return EntryData::from_raw(std::move(bytes), loc.checksum);
        case EntryType::Link:
            return EntryData::from_link(std::move(bytes), loc.checksum);
        case EntryType::Gzip:
            return EntryData::from_gzip(std::move(bytes), loc.size_decompressed, loc.checksum);
        case EntryType::Zstd:
            return EntryData::from_zstd(std::move(bytes), loc.size_decompressed, loc.checksum);
        case EntryType::ZstdMulti:
            return EntryData::from_zstd_multi(std::move(bytes),
                                              loc.size_decompressed,
                                              loc.checksum,
                                              loc.subchunk_count,
                                              loc.subchunk_index);
        default:
            lol_throw_msg("Unknown EntryType: {:#x}", (unsigned)loc.type);
    }
}

auto EntryData::checksum() const -> std::uint64_t {
    auto result = impl_->checksum;
    if (!result) {
        result = XXH3_64bits(impl_->bytes.data(), impl_->bytes.size());
        impl_->checksum = result;
    }
    return result;
}

auto EntryData::extension() const -> std::string_view {
    lol_trace_func();
    auto result = impl_->extension;
    if (!result) {
        switch (impl_->type) {
            case EntryType::Raw:
                result = utility::Magic::find({bytes_data(), bytes_size()});
                break;
            case EntryType::Link:
                // TODO: scan for extension by reading link data...
                result = "";
                break;
            case EntryType::Gzip:
                result = into_decompressed().extension();
                break;
            case EntryType::Zstd:
            case EntryType::ZstdMulti: {
                char buffer[128];
                auto buffer_cap = std::min(size_decompressed(), sizeof(buffer));
                auto buffer_size = bytes().readsome_decompress_zstd(0, bytes_size(), buffer, buffer_cap);
                result = utility::Magic::find({buffer, buffer_size});
            }; break;
            default:
                lol_throw_msg("Unreachable Type!");
        }
        impl_->extension = result;
    }
    return *result;
}

auto EntryData::into_compressed() const -> EntryData {
    lol_trace_func();
    auto result = impl_->compressed.lock();
    if (!result) {
        switch (impl_->type) {
            case EntryType::Link:
                result = impl_;
                break;
            case EntryType::Raw:
                result = std::make_shared<Impl>();
                result->type = EntryType::Zstd;
                result->size_decompressed = size_decompressed();
                result->bytes = impl_->bytes.copy_compress_zstd();
                result->compressed = result;
                result->decompressed = impl_;
                break;
            case EntryType::Gzip:
                result = into_decompressed().into_compressed().impl_;
                break;
            case EntryType::Zstd:
                result = impl_;
                break;
            case EntryType::ZstdMulti:
                result = into_decompressed().into_compressed().impl_;
                break;
            default:
                lol_throw_msg("Unreachable Type!");
        }
        impl_->compressed = result;
    }
    return EntryData(std::move(result));
}

auto EntryData::into_decompressed() const -> EntryData {
    lol_trace_func();
    auto result = impl_->decompressed.lock();
    if (!result) {
        switch (impl_->type) {
            case EntryType::Link:
                result = impl_;
                break;
            case EntryType::Raw:
                result = impl_;
                break;
            case EntryType::Gzip:
                result = std::make_shared<Impl>();
                result->type = EntryType::Raw;
                result->size_decompressed = size_decompressed();
                result->bytes = bytes().copy_decompress_gzip(size_decompressed());
                result->decompressed = result;
                break;
            case EntryType::Zstd:
                result = std::make_shared<Impl>();
                result->type = EntryType::Raw;
                result->size_decompressed = size_decompressed();
                result->bytes = bytes().copy_decompress_zstd(size_decompressed());
                result->compressed = impl_;
                result->decompressed = result;
                break;
            case EntryType::ZstdMulti:
                result = std::make_shared<Impl>();
                result->type = EntryType::Raw;
                result->size_decompressed = size_decompressed();
                result->bytes = bytes().copy_decompress_zstd_hack(size_decompressed());
                result->decompressed = result;
                break;
            default:
                lol_throw_msg("Unreachable Type!");
        }
        impl_->decompressed = result;
    }
    return EntryData(std::move(result));
}

auto EntryData::into_optimal() const -> EntryData {
    lol_trace_func();
    auto result = *this;
    if (result.impl_->is_optimal) {
        return result;
    }
    switch (impl_->type) {
        case EntryType::Raw:
            if (auto ext = extension(); !(ext == ".bnk" || ext == ".wpk")) {
                result = into_compressed();
            }
            break;
        case EntryType::Link:
            break;
        case EntryType::Gzip:
            result = into_decompressed().into_optimal();
            break;
        case EntryType::Zstd:
            if (auto ext = extension(); ext == ".bnk" || ext == ".wpk") {
                result = into_decompressed();
            }
            break;
        case EntryType::ZstdMulti:
            if (auto ext = extension(); ext == ".bnk" || ext == ".wpk") {
                result = into_decompressed();
            } else {
                result = into_compressed();
            }
            break;
        default:
            lol_throw_msg("Unreachable Type!");
    }
    result.impl_->is_optimal = true;
    return result;
}

auto EntryData::write_to_file(fs::path const& path) const -> void {
    lol_trace_func(lol_trace_var("{}", path));
    auto file = io::File::create(path);
    auto bytes = into_decompressed().bytes();
    file.write(0, bytes.data(), bytes.size());
    file.resize(bytes.size());
}

auto EntryData::write_to_dir(hash::Xxh64 name, fs::path const& dir, hash::Dict const* dict) const -> void {
    lol_trace_func(lol_trace_var("{}", name), lol_trace_var("{}", dir));
    auto decompressed = into_decompressed();
    auto bytes = decompressed.bytes();
    fs::path rel_path = dict ? fs::path(dict->get(name)) : "";
    if (!rel_path.empty()) {
        for (auto const& part : rel_path) {
            if (part.generic_string().size() > 127) {
                rel_path = "";
                break;
            }
        }
    }

    io::File file;
    if (!rel_path.empty()) {
        try {
            file = io::File::create(dir / rel_path);
        } catch (std::exception const& e) {
            error::stack_trace().clear();
            rel_path.clear();
        }
    }

    if (rel_path.empty()) {
        rel_path = fmt::format("{:016x}{}", (std::uint64_t)name, decompressed.extension());
        file = io::File::create(dir / rel_path);
    }

    file.write(0, bytes.data(), bytes.size());
    file.resize(bytes.size());
}

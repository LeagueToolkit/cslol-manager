#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace lol {
    struct MachO {
        using data_t = unsigned char const*;
        struct Header {
            std::uint32_t magic;
            std::uint32_t cputype;
            std::uint32_t cpusubtype;
            std::uint32_t filetype;
            std::uint32_t ncmds;
            std::uint32_t sizeofcmds;
            std::uint32_t flags;
            std::uint32_t reserved;
        };

        struct Command {
            std::uint32_t cmd;
            std::uint32_t cmdsize;
        };

        struct Segment {
            char segname[16];
            std::uint64_t vmaddr;
            std::uint64_t vmsize;
            std::uint64_t fileoff;
            std::uint64_t filesize;
            std::int32_t maxprot;
            std::int32_t initprot;
            std::uint32_t nsects;
            std::uint32_t flags;
        };

        struct Section {
            char sectname[16];
            char segname[16];
            std::uint64_t addr;
            std::uint64_t size;
            std::uint32_t offset;
            std::uint32_t align;
            std::uint32_t reloff;
            std::uint32_t nreloc;
            std::uint32_t flags;
            std::uint32_t reserved1;
            std::uint32_t reserved2;
            std::uint32_t reserved3;
        };

        struct DyldInfo {
            std::uint32_t rebase_off;
            std::uint32_t rebase_size;
            std::uint32_t bind_off;
            std::uint32_t bind_size;
            std::uint32_t weak_bind_off;
            std::uint32_t weak_bind_size;
            std::uint32_t lazy_bind_off;
            std::uint32_t lazy_bind_size;
            std::uint32_t export_off;
            std::uint32_t export_size;
        };

        struct Symtab {
            std::uint32_t symoff;
            std::uint32_t nsyms;
            std::uint32_t stroff;
            std::uint32_t strsize;
        };

        struct Export {
            struct ReExport {
                std::string name;
                std::uint64_t ordinal;
            };
            struct StubAndResolver {
                std::uint64_t stub_offset;
                std::uint64_t resolver_offset;
            };
            std::optional<std::uint64_t> symbol_offset;
            std::optional<ReExport> reexport;
            std::optional<StubAndResolver> stub_and_resolver;
        };

        struct Symbol {
            std::uint32_t n_strx;
            std::uint8_t n_type;
            std::uint8_t n_sect;
            std::int16_t n_desc;
            std::uint64_t n_value;
        };

        struct BindingLazy {
            std::uint8_t binding_type = 0;
            std::uint8_t sym_flags = 0;
            std::uint8_t segment = 0;
            std::int64_t lib_ord = 0;
            std::uint64_t address = 0;
        };

        void parse_data(data_t data, size_t size);
        void parse_file(std::filesystem::path const& filename);
        std::uint64_t find_export(char const* func_name) const;
        std::uint64_t find_import_ptr(char const* func_name) const;
        std::uint64_t find_stub_refs(std::uint64_t address) const;

    private:
        struct Stream;
        void read_stubs(Stream const& org_stream, std::uint64_t address);
        void read_exports(Stream const& org_stream);
        void read_binding_lazy(Stream const& org_stream);

        std::vector<Segment> segments;
        std::unordered_map<std::string, Export> exports;
        std::unordered_map<std::string, BindingLazy> binding_lazy;
        std::unordered_map<std::uint64_t, std::uint64_t> stub_refs;
    };
}

/*************************************************************************************************/
/* Implementation                                                                                */
/*************************************************************************************************/

namespace lol {
    struct MachO::Stream {
        data_t data_ = {};
        size_t size_ = {};

        inline bool empty() { return !size_; }

        template <typename T>
        inline T read_raw() {
            if (size_ < sizeof(T)) {
                throw std::runtime_error("Failed to read raw: no more data!");
            }
            unsigned char raw[sizeof(T)];
            memcpy(raw, data_, sizeof(T));
            T result;
            memcpy(&result, raw, sizeof(T));
            data_ = data_ + sizeof(T);
            size_ -= sizeof(T);
            return result;
        }

        inline std::uint64_t read_uleb64() {
            std::uint64_t result = 0;
            for (int shift = 0; true; shift += 7) {
                if (shift + 7 > 63) {
                    throw std::runtime_error("Failed to read uleb64: value too big!");
                }
                auto const c = read_raw<std::uint8_t>();
                result |= ((std::uint64_t)(c & 0x7f) << shift);
                if (!(c & 0x80)) {
                    break;
                }
            }
            return result;
        }

        inline std::int64_t read_sleb64() {
            std::int64_t result = 0;
            for (int shift = 0; true; shift += 7) {
                if (shift + 7 > 63) {
                    throw std::runtime_error("Failed to read uleb64: value too big!");
                }
                auto const c = read_raw<std::uint8_t>();
                result |= ((std::uint64_t)(c & 0x7f) << shift);
                if (!(c & 0x80)) {
                    result <<= 64 - (shift + 7);
                    result >>= 64 - (shift + 7);
                    break;
                }
            }
            return result;
        }

        inline std::string read_zstring() {
            auto result = std::string{};
            while (auto const c = read_raw<char>()) {
                result.push_back(c);
            }
            return result;
        }

        inline Stream copy(std::size_t offset = 0, std::size_t count = (std::size_t)-1) const {
            if (offset > size_) {
                throw std::runtime_error("Failed to seek: out of range!");
            }
            if (auto const remain_data = size_ - offset; count == (std::size_t)-1) {
                count = remain_data;
            } else {
                if (remain_data < count) {
                    throw std::runtime_error("Failed to seek: out of range!");
                }
            }
            return Stream{data_ + offset, count};
        }

        inline void skip(std::size_t count) {
            if (count > size_) {
                throw std::runtime_error("Failed to skip: out of range!");
            }
            data_ += count;
            size_ -= count;
        }
    };

    inline void MachO::parse_data(data_t data, size_t data_size) {
        segments.clear();
        exports.clear();
        binding_lazy.clear();
        stub_refs.clear();
        auto const file_stream = Stream{data, data_size};
        auto stream = file_stream.copy();
        auto const header = stream.read_raw<Header>();
        for (auto count = header.ncmds; count; --count) {
            auto cmd_stream = stream.copy();
            auto const command = cmd_stream.read_raw<Command>();
            if (command.cmd == 0x19) {
                auto const segment = cmd_stream.read_raw<Segment>();
                this->segments.push_back(segment);
                for (auto count = segment.nsects; count; --count) {
                    auto const section = cmd_stream.read_raw<Section>();
                    if (std::string_view{section.sectname} == "__stubs") {
                        read_stubs(file_stream.copy(segment.fileoff + section.offset, section.size), section.addr);
                    }
                }
            }
            if ((command.cmd & ~0x80000000u) == 0x22) {
                auto const info = cmd_stream.read_raw<DyldInfo>();
                read_exports(file_stream.copy(info.export_off, info.export_size));
                read_binding_lazy(file_stream.copy(info.lazy_bind_off, info.lazy_bind_size));
            }
            stream.skip(command.cmdsize);
        }
    }

    inline void MachO::parse_file(std::filesystem::path const& filename) {
        auto result = std::vector<unsigned char>{};
        if (auto file = std::ifstream(filename, std::ios::binary)) {
            auto beg = file.tellg();
            file.seekg(std::ios::end);
            auto end = file.tellg();
            file.seekg(std::ios::beg);
            result.resize((std::size_t)(end - beg));
            file.read((char*)result.data(), end - beg);
        }
        parse_data(result.data(), result.size());
    }

    inline void MachO::read_stubs(const Stream& org_stream, std::uint64_t address) {
        auto stream = org_stream.copy();
        while (!stream.empty()) {
            auto const opcode = stream.read_raw<std::uint16_t>();
            if (opcode != 0x25FF) {
                // throw error here
            }
            auto const relative = stream.read_raw<std::int32_t>();
            stub_refs[(address + 6) + relative] = address + 2;
            address += 6;
        }
    }

    inline void MachO::read_exports(Stream const& org_stream) {
        for (auto stack = std::vector<std::pair<std::string, std::size_t>>{{"", 0ull}}; !stack.empty();) {
            auto const [sym_name, offset] = stack.back();
            stack.pop_back();
            auto stream = org_stream.copy(offset);
            if (auto const info_len = stream.read_uleb64()) {
                if (auto const flags = stream.read_uleb64(); flags & 0x08) {
                    auto const ordinal = stream.read_uleb64();
                    auto const name = stream.read_zstring();
                    exports[sym_name] = Export{.reexport = Export::ReExport{
                                                   .name = name,
                                                   .ordinal = ordinal,
                                               }};
                } else if (flags & 0x10) {
                    auto const stub_offset = stream.read_uleb64();
                    auto const resolver_offset = stream.read_uleb64();
                    exports[sym_name] = Export{.stub_and_resolver = Export::StubAndResolver{
                                                   .stub_offset = stub_offset,
                                                   .resolver_offset = resolver_offset,
                                               }};
                } else {
                    auto const symbol_offset = stream.read_uleb64();
                    exports[sym_name] = Export{.symbol_offset = symbol_offset};
                }
            }
            for (auto child_count = (std::uint32_t)stream.read_raw<std::uint8_t>(); child_count; --child_count) {
                auto const child_name = sym_name + stream.read_zstring();
                auto const child_offset = stream.read_uleb64();
                stack.emplace_back(child_name, child_offset);
            }
        }
    }

    inline void MachO::read_binding_lazy(Stream const& org_stream) {
        std::string sym_name = "";
        std::uint8_t binding_type = 0;
        std::uint8_t segment = 0;
        std::uint8_t sym_flags = 0;
        std::int64_t lib_ord = 0;
        std::uint64_t address = 0;
        for (auto stream = org_stream; !stream.empty();) {
            auto const raw_opcode = stream.read_raw<std::uint8_t>();
            auto const opcode = raw_opcode & 0xF0;
            auto const immediate = raw_opcode & 0x0F;
            switch (opcode) {
                case 0x00:
                    sym_name = "";
                    binding_type = 1;
                    segment = 0;
                    sym_flags = 0;
                    lib_ord = 0;
                    address = 0;
                    break;
                case 0x10:
                    lib_ord = immediate;
                    break;
                case 0x20:
                    lib_ord = stream.read_uleb64();
                    break;
                case 0x30:
                    if (immediate) {
                        lib_ord = immediate - std::int64_t{16};
                    } else {
                        lib_ord = 0;
                    }
                    break;
                case 0x40:
                    sym_name = stream.read_zstring();
                    sym_flags = immediate;
                    break;
                case 0x50:
                    binding_type = immediate;
                    break;
                case 0x70:
                    address = stream.read_uleb64();
                    segment = immediate;
                    break;
                case 0x90:
                    binding_lazy[sym_name] = MachO::BindingLazy{
                        .binding_type = binding_type,
                        .sym_flags = sym_flags,
                        .segment = segment,
                        .lib_ord = lib_ord,
                        .address = address,
                    };
                    break;
                default:
                    throw std::runtime_error("Unknown symbol binding opcode");
            }
        }
    }

    inline std::uint64_t MachO::find_export(char const* func_name) const {
        if (auto i = exports.find(func_name); i != exports.end()) {
            if (i->second.symbol_offset) {
                return segments.at(0).vmsize + *i->second.symbol_offset;
            }
        }
        return {};
    }

    inline std::uint64_t MachO::find_import_ptr(char const* func_name) const {
        if (auto i = binding_lazy.find(func_name); i != binding_lazy.end()) {
            if (i->second.binding_type == 1) {
                return segments.at(i->second.segment).vmaddr + i->second.address;
            }
        }
        return {};
    }

    inline std::uint64_t MachO::find_stub_refs(std::uint64_t address) const {
        if (auto i = stub_refs.find(address); i != stub_refs.end()) {
            return i->second;
        }
        return {};
    }
}

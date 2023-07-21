#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

namespace lol {
    struct PeEx {
        using data_t = char const*;

        struct DosHeader {
            static inline constexpr std::uint16_t MAGIC = 0x5A4D;

            std::uint16_t magic;
            std::uint16_t count_bytes_last_page;
            std::uint16_t count_pages;
            std::uint16_t count_relocations;
            std::uint16_t count_paragraphs_header;
            std::uint16_t min_paragraphs_alloc;
            std::uint16_t max_paragraphs_alloc;
            std::uint16_t init_ss_relative;
            std::uint16_t init_sp;
            std::uint16_t checksum;
            std::uint16_t init_ip;
            std::uint16_t init_cs_relative;
            std::uint16_t file_address_relocations;
            std::uint16_t overlay_number;
            std::uint16_t reserved[4];
            std::uint16_t oem_id;
            std::uint16_t oem_info;
            std::uint16_t reserved2[10];
            std::uint32_t file_address_new;
        };

        struct FileHeader {
            std::uint16_t machine;
            std::uint16_t number_of_sections;
            std::uint32_t date_timestamp;
            std::uint32_t pointer_to_symbols;
            std::uint32_t number_of_symbols;
            std::uint16_t size_optional_header;
            std::uint16_t characteristics;
        };

        struct DataDirectory {
            std::uint32_t address;
            std::uint32_t size;
        };

        struct OptionalHeader {
            static inline constexpr std::uint32_t MAGIC = 0x20B;

            std::uint16_t magic;
            std::uint8_t major_linker_version;
            std::uint8_t minor_linker_version;
            std::uint32_t size_of_code;
            std::uint32_t size_of_initialized_data;
            std::uint32_t size_of_uninitialized_data;
            std::uint32_t address_of_entry_point;
            std::uint32_t base_of_code;
            std::uint64_t image_base;
            std::uint32_t section_alignment;
            std::uint32_t file_alignment;
            std::uint16_t major_operating_system_version;
            std::uint16_t minor_operating_system_version;
            std::uint16_t major_image_version;
            std::uint16_t minor_image_version;
            std::uint16_t major_subsystem_version;
            std::uint16_t minor_subsystem_version;
            std::uint32_t win32_version_value;
            std::uint32_t size_of_image;
            std::uint32_t size_of_headers;
            std::uint32_t checksum;
            std::uint16_t subsystem;
            std::uint16_t dll_characteristics;
            std::uint64_t size_of_stack_reserve;
            std::uint64_t size_of_stack_commit;
            std::uint64_t size_of_heap_reserve;
            std::uint64_t size_of_heap_commit;
            std::uint32_t loader_flags;
            std::uint32_t number_of_rva_and_sizes;
            DataDirectory export_directory;
            DataDirectory import_directory;
            DataDirectory resource_directory;
            DataDirectory exception_directory;
            DataDirectory security_directory;
            DataDirectory base_relocation_directory;
            DataDirectory debug_directory;
            DataDirectory copyright_directory;
            DataDirectory architecture_directory;
            DataDirectory global_pointer_directory;
            DataDirectory tls_directory;
            DataDirectory load_configuration_directory;
            DataDirectory bound_import_directory;
            DataDirectory import_address_table_directory;
            DataDirectory delay_import_descriptors_directory;
            DataDirectory com_descriptor_directory;
        };

        struct NtHeader {
            static inline constexpr std::uint32_t SIGNATURE = 0x4550;

            std::uint32_t signature;
            FileHeader file_header;
            OptionalHeader optional_header;
        };

        struct Section {
            char name[8];
            std::uint32_t virtual_size;
            std::uint32_t virtual_address;
            std::uint32_t raw_size;
            std::uint32_t raw_address;
            std::uint32_t relocations_address;
            std::uint32_t line_numbers_address;
            std::uint16_t number_of_relocations;
            std::uint16_t number_of_line_numbers;
            std::uint32_t characterstics;

            operator std::string_view() const noexcept {
                return std::string_view(name, (name + sizeof(name)) - std::find(name, name + sizeof(name), '\0'));
            };
        };

        void parse_data(data_t data, size_t size);

        DosHeader const& dos_header() const& noexcept { return dos_header_; }

        NtHeader const& nt_header() const& noexcept { return nt_header_; }

        std::uint32_t offset_rdata() const noexcept {
            // assume rdata comes after text for now
            return nt_header_.optional_header.base_of_code + nt_header_.optional_header.size_of_code;
        }

        std::uint32_t checksum() const noexcept { return nt_header_.optional_header.checksum; }

        std::vector<Section> const& sections() const noexcept { return sections_; }

        Section const* section(std::string_view name) const noexcept {
            auto result = std::find(sections_.begin(), sections_.end(), name);
            if (result == sections_.end()) {
                return nullptr;
            }
            return &*result;
        }

        std::uint64_t raw_to_virtual(std::uint32_t off) const noexcept {
            for (auto const& section : sections_) {
                if (off >= section.raw_address) {
                    if (auto disp = off - section.raw_address; disp < section.raw_size) {
                        return section.virtual_address + disp;
                    }
                }
            }
            return 0;
        }

    private:
        struct Stream;
        DosHeader dos_header_;
        NtHeader nt_header_;
        std::vector<Section> sections_;
    };
}

/*************************************************************************************************/
/* Implementation                                                                                */
/*************************************************************************************************/

namespace lol {
    struct PeEx::Stream {
        data_t data_ = {};
        size_t size_ = {};

        template <typename T>
        inline T read_raw() {
            if (size_ < sizeof(T)) {
                throw std::runtime_error("Failed to read raw: no more data!");
            }
            T result;
            memcpy(&result, data_, sizeof(T));
            data_ += sizeof(T);
            size_ -= sizeof(T);
            return result;
        }

        template <typename T>
        inline std::vector<T> read_vector(std::size_t count) {
            auto size = sizeof(T) * count;
            if (size_ < size) {
                throw std::runtime_error("Failed to read raw: no more data!");
            }
            auto results = std::vector<T>(count);
            memcpy(results.data(), data_, size);
            data_ += size;
            size_ -= size;
            return results;
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
    };

    inline void PeEx::parse_data(data_t data, size_t data_size) {
        auto const file_stream = Stream{data, data_size};
        dos_header_ = file_stream.copy().read_raw<DosHeader>();
        if (dos_header_.magic != dos_header_.MAGIC) {
            throw std::runtime_error("Bad dos_header magic!");
        }
        nt_header_ = file_stream.copy(dos_header_.file_address_new).read_raw<NtHeader>();
        if (nt_header_.signature != nt_header_.SIGNATURE) {
            throw std::runtime_error("Bad nt_header signature!");
        }
        auto const sections_offset = dos_header_.file_address_new + 0x18 + nt_header_.file_header.size_optional_header;
        sections_ = file_stream.copy(sections_offset).read_vector<Section>(nt_header_.file_header.number_of_sections);
    }
}

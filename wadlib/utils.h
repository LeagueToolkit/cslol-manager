#ifndef UTILS_H
#define UTILS_H

#include <string>

extern uint32_t xx32(std::string str) noexcept;

extern uint64_t xx64(std::string str) noexcept;

extern std::string read_link(std::string const& path) noexcept;

extern std::string find_league(std::string from) noexcept;

extern std::string read_line(std::string const& from) noexcept;

extern bool write_line(std::string const& to, std::string const& line) noexcept;

#endif // UTILS_H

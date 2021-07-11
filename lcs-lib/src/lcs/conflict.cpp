#include "conflict.hpp"
#include "error.hpp"
#include <string>

using namespace LCS;


void LCS::raise_hash_conflict(uint64_t xxhash,
                              fs::path const& orgpath,
                              fs::path const& newpath) {
    lcs_trace_func(
                lcs_trace_var(to_hex_string(xxhash)),
                lcs_trace_var(orgpath),
                lcs_trace_var(newpath)
                );
    throw std::runtime_error("Hash conflict!");
}

void LCS::raise_wad_conflict(fs::path const& name,
                        fs::path const& orgpath,
                        fs::path const& newpath) {
    lcs_trace_func(
                lcs_trace_var(name),
                lcs_trace_var(orgpath),
                lcs_trace_var(newpath)
                );
    throw std::runtime_error("Wad conflict!");
}

cmake_minimum_required(VERSION 3.20)

project(xxhash)
add_library(xxhash STATIC src/xxhash.h src/xxh3.h src/xxhash.c)
target_include_directories(xxhash PUBLIC src/)
target_compile_definitions(xxhash INTERFACE -DXXH_STATIC_LINKING_ONLY)
target_compile_definitions(xxhash PRIVATE -DXXH_CPU_LITTLE_ENDIAN)

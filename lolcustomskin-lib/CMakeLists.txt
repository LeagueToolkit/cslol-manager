cmake_minimum_required(VERSION 3.10)

project(lolcustomskin-tools LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(dep/picosha2)
add_subdirectory(dep/json)
add_subdirectory(dep/xxhash)
add_subdirectory(dep/miniz)
add_subdirectory(dep/zstd)

add_library(lolcustomskin-lib STATIC
    src/lcs/common.hpp
    src/lcs/common.cpp
    src/lcs/hashtable.hpp
    src/lcs/hashtable.cpp
    src/lcs/mod.hpp
    src/lcs/mod.cpp
    src/lcs/modindex.hpp
    src/lcs/modindex.cpp
    src/lcs/modunzip.hpp
    src/lcs/modunzip.cpp
    src/lcs/wad.hpp
    src/lcs/wad.cpp
    src/lcs/wadindex.hpp
    src/lcs/wadindex.cpp
    src/lcs/wadindexcache.hpp
    src/lcs/wadindexcache.cpp
    src/lcs/wadmake.hpp
    src/lcs/wadmake.cpp
    src/lcs/wadmakequeue.hpp
    src/lcs/wadmakequeue.cpp
    src/lcs/wadmerge.hpp
    src/lcs/wadmerge.cpp
    src/lcs/wadmergequeue.hpp
    src/lcs/wadmergequeue.cpp
    src/lcs/wxyextract.hpp
    src/lcs/wxyextract.cpp
    src/lcs/utility.hpp
    src/lcs/utility.cpp
)

target_link_libraries(lolcustomskin-lib PRIVATE picosha2 json xxhash miniz zstd)
target_include_directories(lolcustomskin-lib PUBLIC src/)

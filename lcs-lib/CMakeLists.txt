cmake_minimum_required(VERSION 3.10)

project(lcs-tools LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(dep/picosha2)
add_subdirectory(dep/json)
add_subdirectory(dep/xxhash)
add_subdirectory(dep/miniz)
add_subdirectory(dep/zstd)

add_library(lcs-lib STATIC
    src/lcs/common.cpp
    src/lcs/common.hpp
    src/lcs/conflict.cpp
    src/lcs/conflict.hpp
    src/lcs/error.cpp
    src/lcs/error.hpp
    src/lcs/hashtable.cpp
    src/lcs/hashtable.hpp
    src/lcs/iofile.cpp
    src/lcs/iofile.hpp
    src/lcs/mod.cpp
    src/lcs/mod.hpp
    src/lcs/modindex.cpp
    src/lcs/modindex.hpp
    src/lcs/modunzip.cpp
    src/lcs/modunzip.hpp
    src/lcs/progress.cpp
    src/lcs/progress.hpp
    src/lcs/utility.cpp
    src/lcs/utility.hpp
    src/lcs/wad.cpp
    src/lcs/wad.hpp
    src/lcs/wadindex.cpp
    src/lcs/wadindex.hpp
    src/lcs/wadindexcache.cpp
    src/lcs/wadindexcache.hpp
    src/lcs/wadmake.cpp
    src/lcs/wadmake.hpp
    src/lcs/wadmakequeue.cpp
    src/lcs/wadmakequeue.hpp
    src/lcs/wadmerge.cpp
    src/lcs/wadmerge.hpp
    src/lcs/wadmergequeue.cpp
    src/lcs/wadmergequeue.hpp
    src/lcs/wxyextract.cpp
    src/lcs/wxyextract.hpp
)

target_link_libraries(lcs-lib PRIVATE picosha2 json xxhash miniz zstd)
target_include_directories(lcs-lib PUBLIC src/)

cmake_minimum_required(VERSION 3.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(wad)

add_subdirectory(dep/xxhash)
add_subdirectory(dep/picosha2)
#add_subdirectory(dep/zlib)

add_library(wadlib STATIC
    wadlib/file.cpp
    wadlib/file.hpp
    wadlib/wad.cpp
    wadlib/wad.h
    wadlib/link.cpp
    wadlib/link.h
    wadlib/utils.cpp
    wadlib/utils.h
    wadlib/installer.cpp
    wadlib/installer.h
)
target_link_libraries(wadlib PRIVATE xxhash picosha2)
target_include_directories(wadlib PUBLIC wadlib)

add_library(wadapi SHARED wadapi/api.cpp wadapi/api.h)
target_link_libraries(wadapi PRIVATE wadlib)

add_executable(wadinstall wadinstall/main.cpp)
target_link_libraries(wadinstall PUBLIC wadlib)

add_executable(wadmake wadmake/main.cpp)
target_link_libraries(wadmake PUBLIC wadlib)

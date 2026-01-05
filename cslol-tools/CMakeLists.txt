cmake_minimum_required(VERSION 3.20)

project(cslol-tools LANGUAGES C CXX)

cmake_policy(SET CMP0169 OLD)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(dep)

add_library(cslol-lib STATIC
    lib/lol/common.cpp
    lib/lol/common.hpp
    lib/lol/error.cpp
    lib/lol/error.hpp
    lib/lol/fs.cpp
    lib/lol/fs.hpp
    lib/lol/log.hpp
    lib/lol/log.cpp

    lib/lol/hash/dict.cpp
    lib/lol/hash/dict.hpp
    lib/lol/hash/fnv1a32.cpp
    lib/lol/hash/fnv1a32.hpp
    lib/lol/hash/xxh64.cpp
    lib/lol/hash/xxh64.hpp

    lib/lol/io/buffer.hpp
    lib/lol/io/buffer.cpp
    lib/lol/io/bytes.cpp
    lib/lol/io/bytes.hpp
    lib/lol/io/file.cpp
    lib/lol/io/file.hpp
    lib/lol/io/sys.cpp
    lib/lol/io/sys.hpp

    lib/lol/patcher/patcher.hpp
    lib/lol/patcher/patcher_dummy.cpp
    lib/lol/patcher/patcher_macos.cpp
    lib/lol/patcher/patcher_win32.cpp
    lib/lol/patcher/utility/delay.hpp
    lib/lol/patcher/utility/macho.hpp
    lib/lol/patcher/utility/process.hpp
    lib/lol/patcher/utility/process_macos.cpp

    lib/lol/utility/cli.hpp
    lib/lol/utility/cli.cpp
    lib/lol/utility/magic.cpp
    lib/lol/utility/magic.hpp
    lib/lol/utility/zip.cpp
    lib/lol/utility/zip.hpp

    lib/lol/wad/archive.cpp
    lib/lol/wad/archive.hpp
    lib/lol/wad/mounted.hpp
    lib/lol/wad/mounted.cpp
    lib/lol/wad/entry.cpp
    lib/lol/wad/entry.hpp
    lib/lol/wad/index.cpp
    lib/lol/wad/index.hpp
    lib/lol/wad/toc.cpp
    lib/lol/wad/toc.hpp
)
if (WIN32)
    target_sources(cslol-lib INTERFACE
        res/longpath.manifest
        res/utf8.manifest
    )
endif()

target_compile_definitions(cslol-lib PUBLIC -D_CRT_SECURE_NO_WARNINGS) # lmao, fuck off
target_link_libraries(cslol-lib PUBLIC fmt fmtlog)
target_link_libraries(cslol-lib PRIVATE libdeflate zstd xxhash miniz)
target_include_directories(cslol-lib PUBLIC lib/)

add_executable(mod-tools
    src/main_mod_tools.cpp
)
target_link_libraries(mod-tools PRIVATE cslol-lib)

add_executable(wad-extract
    src/main_wad_extract.cpp
)
target_link_libraries(wad-extract PRIVATE cslol-lib)

add_executable(wad-make
    src/main_wad_make.cpp
)
target_link_libraries(wad-make PRIVATE cslol-lib)

if (WIN32)
    add_executable(cslol-diag src/main_diag.cpp)
    target_sources(cslol-diag PUBLIC
        res/longpath.manifest
        res/utf8.manifest
    )
    target_link_libraries(cslol-diag PRIVATE wintrust)
endif()

if (WIN32)
    add_library(cslol-patcher SHARED IMPORTED GLOBAL)
    set_target_properties(cslol-patcher PROPERTIES
        IMPORTED_IMPLIB "${CMAKE_CURRENT_SOURCE_DIR}/vendor/cslol-patcher/cslol-dll.lib"
        IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/vendor/cslol-patcher/cslol-dll.dll"
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/vendor/cslol-patcher"
    )

    target_link_libraries(cslol-lib PUBLIC cslol-patcher)
    add_custom_command(TARGET mod-tools POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy -t $<TARGET_FILE_DIR:mod-tools> $<TARGET_RUNTIME_DLLS:mod-tools>
      COMMAND_EXPAND_LISTS
    )
endif()

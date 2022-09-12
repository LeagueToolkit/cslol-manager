cmake_minimum_required(VERSION 3.20)

include(FetchContent)

FetchContent_Declare(
    xxhash
    GIT_REPOSITORY https://github.com/Cyan4973/xxHash.git
    GIT_TAG        v0.8.1
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
)
FetchContent_GetProperties(xxhash)
if(NOT xxhash_POPULATED)
    FetchContent_Populate(xxhash)
    add_library(xxhash STATIC ${xxhash_SOURCE_DIR}/xxhash.c)
    target_include_directories(xxhash PUBLIC ${xxhash_SOURCE_DIR})
    target_compile_definitions(xxhash INTERFACE -DXXH_STATIC_LINKING_ONLY)
    target_compile_definitions(xxhash PRIVATE -DXXH_CPU_LITTLE_ENDIAN)
endif()


FetchContent_Declare(
    zstd
    GIT_REPOSITORY https://github.com/facebook/zstd.git
    GIT_TAG        v1.5.2
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
)
FetchContent_GetProperties(zstd)
if(NOT zstd_POPULATED)
    FetchContent_Populate(zstd)
    file(GLOB zstd_SRCS
        ${zstd_SOURCE_DIR}/lib/common/*.c
        ${zstd_SOURCE_DIR}/lib/compress/*.c
        ${zstd_SOURCE_DIR}/lib/decompress/*.c
    )
    add_library(zstd STATIC ${zstd_SRCS})
    target_include_directories(zstd PUBLIC ${zstd_SOURCE_DIR}/lib)
    target_include_directories(zstd PRIVATE ${zstd_SOURCE_DIR}/lib/common)
    target_compile_definitions(zstd INTERFACE -DZSTD_STATIC_LINKING_ONLY)
    target_compile_definitions(zstd PRIVATE -DZSTD_DISABLE_ASM)
    if(MSVC)
        target_compile_options(zstd PRIVATE /wd4267)
    endif()
endif()


FetchContent_Declare(
    libdeflate
    GIT_REPOSITORY https://github.com/ebiggers/libdeflate.git
    GIT_TAG        v1.12
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
)
FetchContent_GetProperties(libdeflate)
if(NOT libdeflate_POPULATED)
    FetchContent_Populate(libdeflate)
    file(GLOB libdeflate_SRCS
        ${libdeflate_SOURCE_DIR}/lib/*.c
        ${libdeflate_SOURCE_DIR}/lib/arm/*.c
        ${libdeflate_SOURCE_DIR}/lib/x86/*.c
    )
    add_library(libdeflate STATIC ${libdeflate_SRCS})
    target_include_directories(libdeflate PUBLIC ${libdeflate_SOURCE_DIR})
    target_include_directories(libdeflate PRIVATE ${libdeflate_SOURCE_DIR}/lib)
endif()


FetchContent_Declare(
    miniz
    GIT_REPOSITORY https://github.com/richgel999/miniz.git
    GIT_TAG        76b3a872855388c735c564905da030f26334f3b3
)
FetchContent_GetProperties(miniz)
if(NOT miniz_POPULATED)
    FetchContent_Populate(miniz)
    add_subdirectory(${miniz_SOURCE_DIR} ${miniz_BINARY_DIR})
    target_compile_definitions(miniz PRIVATE -DMINIZ_DISABLE_ZIP_READER_CRC32_CHECKS=1)
endif()


FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG        8.1.1
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
)
FetchContent_GetProperties(fmt)
if(NOT fmt_POPULATED)
    FetchContent_Populate(fmt)
    add_library(fmt STATIC ${fmt_SOURCE_DIR}/src/format.cc ${fmt_SOURCE_DIR}/src/os.cc)
    target_include_directories(fmt PUBLIC ${fmt_SOURCE_DIR}/include)
    target_compile_definitions(fmt PRIVATE -DFMT_UNICODE=1)
endif()


FetchContent_Declare(
    fmtlog
    GIT_REPOSITORY https://github.com/MengRao/fmtlog.git
    GIT_TAG        16183b638d302042faf5f9cf2d089ab367ace933
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
)
FetchContent_GetProperties(fmtlog)
if(NOT fmtlog_POPULATED)
    FetchContent_Populate(fmtlog)
    add_library(fmtlog STATIC ${fmtlog_SOURCE_DIR}/fmtlog.cc)
    target_include_directories(fmtlog PUBLIC ${fmtlog_SOURCE_DIR})
    target_link_libraries(fmtlog PUBLIC fmt)
endif()

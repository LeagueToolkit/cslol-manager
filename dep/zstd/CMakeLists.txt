cmake_minimum_required(VERSION 3.1)

project(zstd)

add_library(zstd STATIC
    src/common/bitstream.h
    src/common/compiler.h
    src/common/cpu.h
    src/common/debug.c
    src/common/debug.h
    src/common/entropy_common.c
    src/common/error_private.c
    src/common/error_private.h
    src/common/fse.h
    src/common/fse_decompress.c
    src/common/huf.h
    src/common/mem.h
    src/common/pool.c
    src/common/pool.h
    src/common/threading.c
    src/common/threading.h
    src/common/xxhash.c
    src/common/xxhash.h
    src/common/zstd_common.c
    src/common/zstd_errors.h
    src/common/zstd_internal.h
    src/compress/fse_compress.c
    src/compress/hist.c
    src/compress/hist.h
    src/compress/huf_compress.c
    src/compress/zstd_compress.c
    src/compress/zstd_compress_internal.h
    src/compress/zstd_compress_literals.c
    src/compress/zstd_compress_literals.h
    src/compress/zstd_compress_sequences.c
    src/compress/zstd_compress_sequences.h
    src/compress/zstd_cwksp.h
    src/compress/zstd_double_fast.c
    src/compress/zstd_double_fast.h
    src/compress/zstd_fast.c
    src/compress/zstd_fast.h
    src/compress/zstd_lazy.c
    src/compress/zstd_lazy.h
    src/compress/zstd_ldm.c
    src/compress/zstd_ldm.h
    src/compress/zstd_opt.c
    src/compress/zstd_opt.h
    src/compress/zstdmt_compress.c
    src/compress/zstdmt_compress.h
    src/decompress/huf_decompress.c
    src/decompress/zstd_ddict.c
    src/decompress/zstd_ddict.h
    src/decompress/zstd_decompress.c
    src/decompress/zstd_decompress_block.c
    src/decompress/zstd_decompress_block.h
    src/decompress/zstd_decompress_internal.h
)

target_include_directories(zstd PUBLIC src)
target_include_directories(zstd PRIVATE src/common)
target_compile_definitions(zstd INTERFACE -DZSTD_STATIC_LINKING_ONLY)
if(MSVC)
    target_compile_options(zstd PRIVATE /wd4267)
endif()

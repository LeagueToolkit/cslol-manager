cmake_minimum_required(VERSION 3.1)

project(zlib)

add_library(zlib STATIC 
src/adler32.c
src/compress.c
src/crc32.c
src/crc32.h
src/deflate.c
src/deflate.h
src/gzclose.c
src/gzguts.h
src/gzlib.c
src/gzread.c
src/gzwrite.c
src/infback.c
src/inffast.c
src/inffast.h
src/inffixed.h
src/inflate.c
src/inflate.h
src/inftrees.c
src/inftrees.h
src/trees.c
src/trees.h
src/uncompr.c
src/zconf.h
src/zlib.h
src/zutil.c
src/zutil.h)
target_compile_definitions(zlib PUBLIC
-DHAVE_SYS_TYPES_H
-DHAVE_STDINT_H
-DHAVE_STDDEF_H
)
if(MSVC)
target_compile_options(zlib PRIVATE /wd4996)
target_compile_definitions(zlib PRIVATE -D_CRT_SECURE_NO_WARNINGS)
endif()
target_include_directories(zlib PUBLIC src)

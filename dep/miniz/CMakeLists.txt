cmake_minimum_required(VERSION 3.1)

project(miniz)
add_library(miniz STATIC src/miniz.h  src/miniz.c)
target_include_directories(miniz PUBLIC src/)


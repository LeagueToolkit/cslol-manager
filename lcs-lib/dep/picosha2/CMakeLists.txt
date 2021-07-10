cmake_minimum_required(VERSION 3.20)

project(picosha2)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(picosha2 INTERFACE)
target_include_directories(picosha2 INTERFACE src/)

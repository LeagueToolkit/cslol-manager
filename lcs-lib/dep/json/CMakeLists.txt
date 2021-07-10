cmake_minimum_required(VERSION 3.20)

project(json)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(json INTERFACE)
target_include_directories(json INTERFACE src/)

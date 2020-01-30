cmake_minimum_required(VERSION 3.10)

project(lolcustomskin-wadextract LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(lolcustomskin-wadextract main.cpp)
target_link_libraries(lolcustomskin-wadextract PRIVATE lolcustomskin-lib)


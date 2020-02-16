cmake_minimum_required(VERSION 3.10)

project(lcs-wxyextract LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(lcs-wxyextract main.cpp)
target_link_libraries(lcs-wxyextract PRIVATE lcs-lib)


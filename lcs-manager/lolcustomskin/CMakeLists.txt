cmake_minimum_required(VERSION 3.14)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
project(lolcustomskin)
add_executable(lolcustomskin
    src/modoverlay.hpp
    src/modoverlay.cpp
    src/ppp.hpp
    src/process.hpp
    src/process.cpp
    src/main.cpp
)
target_include_directories(lolcustomskin PRIVATE src/)
target_link_libraries(lolcustomskin PRIVATE psapi)

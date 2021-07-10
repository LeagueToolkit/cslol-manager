cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
project(lolcustomskin)

add_library(lolcustomskin-lib STATIC
    src/lcs/modoverlay.hpp
    src/lcs/modoverlay.cpp
    src/lcs/ppp.hpp
    src/lcs/process.hpp
    src/lcs/process.cpp
    )
target_link_libraries(lolcustomskin-lib PUBLIC psapi)
target_include_directories(lolcustomskin-lib PUBLIC src/)

add_executable(lolcustomskin src/main.cpp)
target_link_libraries(lolcustomskin PRIVATE lolcustomskin-lib)

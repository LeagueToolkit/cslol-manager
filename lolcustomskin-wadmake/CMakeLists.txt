cmake_minimum_required(VERSION 3.10)

project(lolcustomskin-wadmake LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(lolcustomskin-wadmake main.cpp)
target_link_libraries(lolcustomskin-wadmake PRIVATE lolcustomskin-lib)


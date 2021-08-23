cmake_minimum_required(VERSION 3.20)

project(lolcustomskin-tools LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(lcs-patcher)
add_subdirectory(lcs-lib)
add_subdirectory(lcs-manager)
add_subdirectory(lcs-wadextract)
add_subdirectory(lcs-wadmake)
add_subdirectory(lcs-wxyextract)

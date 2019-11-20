cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_LIBRARIES "kernel32.lib User32.lib")
project(lolcustomskin)
add_executable(lolcustomskin main_modoverlay.cpp process_win32.cpp)

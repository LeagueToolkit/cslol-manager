cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_LIBRARIES "kernel32.lib User32.lib")
#set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} /MT")
project(customlolskins)
add_executable(customlolskins
    main.cpp process.hpp patscanner.hpp def.hpp config.hpp)

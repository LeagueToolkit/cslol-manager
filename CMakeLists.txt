cmake_minimum_required(VERSION 3.5)

project(lolcustomskin-tools LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(dep/picosha2)
add_subdirectory(dep/json)
add_subdirectory(dep/xxhash)
add_subdirectory(dep/miniz)
add_subdirectory(dep/zstd)

find_package(Qt5 COMPONENTS Core Quick Widgets REQUIRED)

add_executable(lolcustomskin-tools WIN32
    src/lcs/common.hpp
    src/lcs/common.cpp
    src/lcs/mod.hpp
    src/lcs/modindex.hpp
    src/lcs/modindex.cpp
    src/lcs/modunzip.hpp
    src/lcs/modunzip.cpp
    src/lcs/mod.cpp
    src/lcs/wad.hpp
    src/lcs/wad.cpp
    src/lcs/wadindex.hpp
    src/lcs/wadindex.cpp
    src/lcs/wadindexcache.hpp
    src/lcs/wadindexcache.cpp
    src/lcs/wadmake.hpp
    src/lcs/wadmake.cpp
    src/lcs/wadmakequeue.hpp
    src/lcs/wadmakequeue.cpp
    src/lcs/wadmerge.hpp
    src/lcs/wadmerge.cpp
    src/lcs/wadmergequeue.hpp
    src/lcs/wadmergequeue.cpp
    src/LCSTools.h
    src/LCSTools.cpp
    src/LCSToolsImpl.h
    src/LCSToolsImpl.cpp
    src/main.cpp
    qml/qml.qrc
    qml/rc.rc
)
target_compile_definitions(lolcustomskin-tools PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)

target_link_libraries(lolcustomskin-tools PRIVATE
    Qt5::Core Qt5::Quick Qt5::Widgets picosha2 json xxhash miniz zstd)

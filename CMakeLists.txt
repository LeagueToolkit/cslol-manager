cmake_minimum_required(VERSION 3.10)

project(lolcustomskin-tools LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 COMPONENTS Core Quick Gui Widgets QmlImportScanner REQUIRED)
if(MSVC)
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MT")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
endif()

add_subdirectory(lolcustomskin-lib)
add_executable(lolcustomskin-tools WIN32
    lolcustomskin/src/modoverlay.hpp
    lolcustomskin/src/patscanner.hpp
    lolcustomskin/src/process.hpp
    lolcustomskin/src/process_win32.cpp
    src/LCSTools.h
    src/LCSTools.cpp
    src/LCSToolsImpl.h
    src/LCSToolsImpl.cpp
    src/main.cpp
    src/qml/qml.qrc
    src/qml/rc.rc
)
target_compile_definitions(lolcustomskin-tools PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)
target_link_libraries(lolcustomskin-tools PRIVATE lolcustomskin-lib Qt5::Core Qt5::Gui Qt5::Quick Qt5::Widgets)
target_include_directories(lolcustomskin-tools PRIVATE lolcustomskin/src)
qt5_import_qml_plugins(lolcustomskin-tools)

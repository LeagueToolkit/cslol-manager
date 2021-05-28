cmake_minimum_required(VERSION 3.10)

project(lcs-manager LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 COMPONENTS Core Quick Gui Widgets QmlImportScanner REQUIRED)
find_package(Threads REQUIRED)

if (WIN32)
    add_subdirectory(lolcustomskin)
    add_executable(lcs-manager WIN32
        lolcustomskin/src/modoverlay.hpp
        lolcustomskin/src/modoverlay.cpp
        lolcustomskin/src/process.hpp
        lolcustomskin/src/process.cpp
        src/LCSTools.h
        src/LCSTools.cpp
        src/LCSToolsImpl.h
        src/LCSToolsImpl.cpp
        src/main.cpp
        src/qml/qml.qrc
        src/qml/rc.rc
    )
    target_link_libraries(lcs-manager PRIVATE lcs-lib psapi)
else()
    add_executable(lcs-manager 
        src/LCSTools.h
        src/LCSTools.cpp
        src/LCSToolsImpl.h
        src/LCSToolsImpl.cpp
        src/main.cpp
        src/qml/qml.qrc
        src/qml/rc.rc
    )
endif()
target_compile_definitions(lcs-manager PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)
target_link_libraries(lcs-manager PRIVATE lcs-lib Qt5::Core Qt5::Gui Qt5::Quick Qt5::Widgets Threads::Threads)
target_include_directories(lcs-manager PRIVATE lolcustomskin/src)
qt5_import_qml_plugins(lcs-manager)

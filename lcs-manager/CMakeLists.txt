cmake_minimum_required(VERSION 3.20)

project(lcs-manager LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

option(USE_QT6 "Use Qt6 instead of Qt5" OFF)
if (USE_QT6)
    find_package(Qt6 6.1 COMPONENTS Core Gui Quick QmlImportScanner REQUIRED)
else()
    find_package(Qt5 5.15 COMPONENTS Core Gui Quick QmlImportScanner REQUIRED)
endif()
find_package(Threads REQUIRED)

if (WIN32)
    add_subdirectory(lolcustomskin)
    add_executable(lcs-manager WIN32
        src/LCSTools.h
        src/LCSTools.cpp
        src/LCSToolsImpl.h
        src/LCSToolsImpl.cpp
        src/main.cpp
        src/qml/qml.qrc
        src/qml/rc.rc
        src/lcs-manager.exe.manifest
    )
    target_link_libraries(lcs-manager PRIVATE lcs-lib lolcustomskin-lib)
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
target_link_libraries(lcs-manager PRIVATE lcs-lib Qt::Core Qt::Gui Qt::Quick Threads::Threads)
target_include_directories(lcs-manager PRIVATE lolcustomskin/src)
qt_import_qml_plugins(lcs-manager)

cmake_minimum_required(VERSION 3.20)

project(cslol-manager LANGUAGES CXX C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTOMOC OFF)
set(CMAKE_AUTORCC OFF)

add_subdirectory(cslol-tools)

option(USE_QT6 "Use Qt6 instead of Qt5" OFF)
if (USE_QT6)
    find_package(Qt6 6.1 COMPONENTS Core Gui Quick QmlImportScanner Network REQUIRED)
else()
    find_package(Qt5 5.15 COMPONENTS Core Gui Quick QmlImportScanner Network REQUIRED)
endif()
find_package(Threads REQUIRED)
find_package(Git QUIET REQUIRED)

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/CSLOLversion.cpp"
    COMMAND "${GIT_EXECUTABLE}"
            "-C" "${CMAKE_SOURCE_DIR}"
            "log"
            "-1"
            "--output" "${CMAKE_CURRENT_BINARY_DIR}/CSLOLversion.cpp"
            "--date=short"
            "--format=\
#include <CSLOLVersion.h>%n\
char const* CSLOL::VERSION=\"%ad-%h\";%n\
char const* CSLOL::COMMIT=\"%H\";%n\
char const* CSLOL::DATE=\"%aI\";%n"
    DEPENDS "${CMAKE_SOURCE_DIR}/.git/index"
    VERBATIM
    )

if (WIN32)
    add_executable(cslol-manager WIN32
        src/main.cpp
        src/res/rc.rc
        src/res/cslol-manager.exe.manifest
    )
    target_link_libraries(cslol-manager PRIVATE user32)
elseif(APPLE)
    set_source_files_properties(src/res/icon.icns
        PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources")
    add_executable(cslol-manager MACOSX_BUNDLE
        src/main.cpp
        src/res/icon.icns
        )
    target_link_libraries(cslol-manager PRIVATE "-framework Security")
    set_target_properties(cslol-manager
        PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/src/res/MacOSXBundleInfo.plist.in"
        MACOSX_BUNDLE_ICON_FILE icon
        MACOSX_BUNDLE_BUNDLE_NAME cslol-manager
        MACOSX_BUNDLE_BUNDLE_VERSION "0.1"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "0.1"
        MACOSX_BUNDLE_GUI_IDENTIFIER cslol.manager
        )
    install(CODE [[
      include(BundleUtilities)
      fixup_bundle("${CMAKE_BINARY_DIR}/cslol-manager/cslol-manager.app" "" "")
    ]] COMPONENT Runtime)
else()
    add_executable(cslol-manager
        src/main.cpp
    )
endif()

target_sources(cslol-manager PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}/CSLOLversion.cpp"
    src/CSLOLTools.h
    src/CSLOLTools.cpp
    src/CSLOLToolsImpl.h
    src/CSLOLToolsImpl.cpp
    src/CSLOLUtils.h
    src/CSLOLUtils.cpp
    src/CSLOLVersion.h
    src/qml/qml.qrc
)

set_target_properties(cslol-manager PROPERTIES AUTOUIC ON)
set_target_properties(cslol-manager PROPERTIES AUTOMOC ON)
set_target_properties(cslol-manager PROPERTIES AUTORCC ON)

target_compile_definitions(cslol-manager PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)
target_link_libraries(cslol-manager PRIVATE Qt::Core Qt::Gui Qt::Quick Qt::Network Threads::Threads)
target_include_directories(cslol-manager PRIVATE src/)

qt_import_qml_plugins(cslol-manager)

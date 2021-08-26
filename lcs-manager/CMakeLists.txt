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
        src/main.cpp
        res/rc.rc
        res/lcs-manager.exe.manifest
    )
    target_link_libraries(lcs-manager PRIVATE user32)
elseif(APPLE)
    set_source_files_properties(res/icon.icns
        PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources")
    add_executable(lcs-manager MACOSX_BUNDLE
        src/main.cpp
        res/icon.icns
        )
    set_target_properties(lcs-manager
        PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/res/MacOSXBundleInfo.plist.in"
        MACOSX_BUNDLE_ICON_FILE icon
        MACOSX_BUNDLE_BUNDLE_NAME lcs-manager
        MACOSX_BUNDLE_BUNDLE_VERSION "0.1"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "0.1"
        MACOSX_BUNDLE_GUI_IDENTIFIER lcs.manager
        )
    install(CODE [[
      include(BundleUtilities)
      fixup_bundle("${CMAKE_BINARY_DIR}/lcs-manager/lcs-manager.app" "" "")
    ]] COMPONENT Runtime)
else()
    add_executable(lcs-manager src/main.cpp)
endif()

target_sources(lcs-manager PUBLIC
    src/LCSTools.h
    src/LCSTools.cpp
    src/LCSToolsImpl.h
    src/LCSToolsImpl.cpp
    src/LCSUtils.h
    src/LCSUtils.cpp
    qml/qml.qrc
)

target_compile_definitions(lcs-manager PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)
target_link_libraries(lcs-manager PRIVATE lcs-patcher lcs-lib Qt::Core Qt::Gui Qt::Quick Threads::Threads)

qt_import_qml_plugins(lcs-manager)

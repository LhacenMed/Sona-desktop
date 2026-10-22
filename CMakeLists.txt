cmake_minimum_required(VERSION 3.21)

project(Sona VERSION 0.1.0 LANGUAGES CXX)

# App identity, shared by the executable and the Windows installer.
set(SONA_APP_NAME "Sona")
set(SONA_PUBLISHER "Sona")
set(SONA_APP_USER_MODEL_ID "Sona.MediaPlayer")

if(WIN32)
    enable_language(RC)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.10 REQUIRED COMPONENTS Quick Qml OpenGL Svg)
qt_standard_project_setup(REQUIRES 6.10)

# Path to the extracted mpv-dev package (shinchiro/mpv-winbuild-cmake).
set(MPV_DIR "C:/dev/mpv-dev" CACHE PATH "Root of the libmpv dev package")

add_library(libmpv SHARED IMPORTED)
set_target_properties(libmpv PROPERTIES
    IMPORTED_LOCATION "${MPV_DIR}/libmpv-2.dll"
    IMPORTED_IMPLIB "${MPV_DIR}/libmpv.dll.a"
    INTERFACE_INCLUDE_DIRECTORIES "${MPV_DIR}/include"
)

# QWindowKit: native window behavior (snap layouts, system menu, resize, animations) for the custom title bar.
set(QWINDOWKIT_BUILD_STATIC ON)
set(QWINDOWKIT_BUILD_WIDGETS OFF)
set(QWINDOWKIT_BUILD_QUICK ON)
set(QWINDOWKIT_ENABLE_STYLE_AGENT OFF)
set(QWINDOWKIT_INSTALL OFF)
# It links Qt's private modules, which Qt 6.9+ requires to be found explicitly.
set(QT_NO_PRIVATE_MODULE_WARNING ON)
find_package(Qt6 REQUIRED COMPONENTS CorePrivate GuiPrivate QuickPrivate)
add_subdirectory(third_party/qwindowkit EXCLUDE_FROM_ALL)

add_subdirectory(src)

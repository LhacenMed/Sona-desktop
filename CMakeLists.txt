cmake_minimum_required(VERSION 3.21)

project(Sona VERSION 0.1.0 LANGUAGES CXX RC)

# App identity, shared by the executable and the Windows installer.
set(SONA_APP_NAME "Sona")
set(SONA_PUBLISHER "Sona")
set(SONA_APP_USER_MODEL_ID "Sona.MediaPlayer")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# The C++ runtime is linked in, so the app needs no Visual C++ redistributable.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# The libVLC SDK and runtime, laid out as include/, lib/ and bin/ (DLLs, plugins, lua) from the
# VLC Windows release archive (https://download.videolan.org/pub/videolan/vlc/last/win64/).
set(LIBVLC_DIR "C:/dev/libvlc-3.0.24" CACHE PATH "Root of the libVLC SDK and runtime")

add_library(libvlc SHARED IMPORTED)
set_target_properties(libvlc PROPERTIES
    IMPORTED_LOCATION "${LIBVLC_DIR}/bin/libvlc.dll"
    IMPORTED_IMPLIB "${LIBVLC_DIR}/lib/libvlc.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${LIBVLC_DIR}/include"
)

add_subdirectory(src)

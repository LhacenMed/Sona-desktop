# Windows installer: `cmake --build --preset mingw-release --target installer` writes Sona-<version>-setup.exe
# to the build folder. Requires Inno Setup's ISCC on PATH.
find_program(ISCC_EXECUTABLE ISCC)
if(NOT ISCC_EXECUTABLE)
    message(STATUS "Inno Setup (ISCC) not found: the 'installer' target is unavailable")
    return()
endif()

set(SONA_STAGE_DIR "${CMAKE_BINARY_DIR}/stage")
set(SONA_EXE_NAME "sona.exe")
configure_file(sona.iss.in "${CMAKE_BINARY_DIR}/sona.iss" @ONLY)

# windeployqt finds the MinGW runtime DLLs on PATH.
cmake_path(GET CMAKE_CXX_COMPILER PARENT_PATH SONA_COMPILER_BIN_DIR)

add_custom_target(installer
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${SONA_STAGE_DIR}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${SONA_STAGE_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:sona>" "${MPV_DIR}/libmpv-2.dll" "${SONA_STAGE_DIR}"
    COMMAND ${CMAKE_COMMAND} -E env --modify "PATH=path_list_prepend:${SONA_COMPILER_BIN_DIR}" --
        "$<TARGET_FILE:Qt6::windeployqt>" "$<IF:$<CONFIG:Debug>,--debug,--release>" --no-translations
        --qmldir "${PROJECT_SOURCE_DIR}/src" "${SONA_STAGE_DIR}/${SONA_EXE_NAME}"
    COMMAND "${ISCC_EXECUTABLE}" /Q "${CMAKE_BINARY_DIR}/sona.iss"
    DEPENDS sona
    VERBATIM
)

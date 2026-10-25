# Windows installer: `cmake --build --preset release --target installer` writes Sona-<version>-setup.exe
# to the build folder. Requires Inno Setup's ISCC on PATH.
find_program(ISCC_EXECUTABLE ISCC)
if(NOT ISCC_EXECUTABLE)
    message(STATUS "Inno Setup (ISCC) not found: the 'installer' target is unavailable")
    return()
endif()

set(SONA_STAGE_DIR "${CMAKE_BINARY_DIR}/stage")
set(SONA_EXE_NAME "sona.exe")
configure_file(sona.iss.in "${CMAKE_BINARY_DIR}/sona.iss" @ONLY)

add_custom_target(installer
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${SONA_STAGE_DIR}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${SONA_STAGE_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:sona>" "${SONA_STAGE_DIR}"
    COMMAND ${CMAKE_COMMAND} -DLIBVLC_BIN=${LIBVLC_DIR}/bin -DDESTINATION=${SONA_STAGE_DIR}
        -P "${PROJECT_SOURCE_DIR}/cmake/DeployLibVlc.cmake"
    COMMAND "${ISCC_EXECUTABLE}" /Q "${CMAKE_BINARY_DIR}/sona.iss"
    DEPENDS sona
    VERBATIM
)

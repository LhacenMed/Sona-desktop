# Builds an MSVC import library for libvlc.dll from its exports, at configure time.
#
# The SDK's lib/libvlc.lib is MinGW's, which MSVC's linker links but can't delay-load (it silently
# loads the DLL at launch instead), and the app delay-loads libVLC so it isn't loaded before the
# window shows. Sets LIBVLC_IMPORT_LIBRARY.
get_filename_component(_msvc_tools "${CMAKE_LINKER}" DIRECTORY)
find_program(SONA_DUMPBIN dumpbin HINTS "${_msvc_tools}" REQUIRED)
find_program(SONA_LIB lib HINTS "${_msvc_tools}" REQUIRED)

set(_dll "${LIBVLC_DIR}/bin/libvlc.dll")
set(LIBVLC_IMPORT_LIBRARY "${CMAKE_BINARY_DIR}/libvlc-msvc/libvlc.lib")
if(NOT EXISTS "${LIBVLC_IMPORT_LIBRARY}" OR "${_dll}" IS_NEWER_THAN "${LIBVLC_IMPORT_LIBRARY}")
    execute_process(COMMAND "${SONA_DUMPBIN}" /nologo /exports "${_dll}"
                    OUTPUT_VARIABLE _exports COMMAND_ERROR_IS_FATAL ANY)
    # Export rows read "ordinal hint RVA name".
    string(REGEX MATCHALL "\n +[0-9]+ +[0-9A-F]+ +[0-9A-F]+ +[A-Za-z_][A-Za-z0-9_]*" _rows "${_exports}")
    set(_def "LIBRARY libvlc.dll\nEXPORTS\n")
    foreach(_row IN LISTS _rows)
        string(REGEX REPLACE ".* ([A-Za-z_][A-Za-z0-9_]*)$" "\\1" _name "${_row}")
        string(APPEND _def "    ${_name}\n")
    endforeach()
    file(WRITE "${CMAKE_BINARY_DIR}/libvlc-msvc/libvlc.def" "${_def}")
    execute_process(COMMAND "${SONA_LIB}" /nologo /machine:x64 "/def:${CMAKE_BINARY_DIR}/libvlc-msvc/libvlc.def"
                            "/out:${LIBVLC_IMPORT_LIBRARY}"
                    COMMAND_ERROR_IS_FATAL ANY)
endif()

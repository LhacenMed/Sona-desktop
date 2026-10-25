# Puts the libVLC runtime next to sona.exe: cmake -DLIBVLC_BIN=<libVLC bin/> -DDESTINATION=<dir> -P DeployLibVlc.cmake
# Runs once per destination. It also builds the plugin cache (plugins.dat), without which libVLC
# scans every plugin DLL on each launch.
if(EXISTS "${DESTINATION}/plugins/plugins.dat")
    return()
endif()

file(COPY "${LIBVLC_BIN}/libvlc.dll" "${LIBVLC_BIN}/libvlccore.dll" DESTINATION "${DESTINATION}")
# hrtfs holds the spatial-audio data; VLC's own interfaces (plugins/gui) are not part of the SDK copy.
file(COPY "${LIBVLC_BIN}/plugins" "${LIBVLC_BIN}/lua" "${LIBVLC_BIN}/hrtfs" DESTINATION "${DESTINATION}")
execute_process(COMMAND "${LIBVLC_BIN}/vlc-cache-gen.exe" "${DESTINATION}/plugins" COMMAND_ERROR_IS_FATAL ANY)

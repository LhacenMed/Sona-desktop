add_executable(sona WIN32
    app/main.cpp
    app/sona.manifest
    commands/CommandRegistry.cpp
    core/Text.cpp
    core/TimeText.cpp
    data/Bookmarks.cpp
    data/Database.cpp
    data/History.cpp
    data/Presets.cpp
    graphics/BackdropCapture.cpp
    graphics/Canvas.cpp
    graphics/GraphicsDevice.cpp
    graphics/WindowSurface.cpp
    library/HistoryRecorder.cpp
    library/HomePage.cpp
    library/ItemViews.cpp
    library/LibraryItems.cpp
    library/LibraryView.cpp
    library/NavigationRail.cpp
    library/Thumbnails.cpp
    maininterface/MainCommands.cpp
    maininterface/MainMenus.cpp
    maininterface/MainWindow.cpp
    onboarding/KeyCap.cpp
    onboarding/OnboardingWindow.cpp
    onboarding/ShortcutsPage.cpp
    onboarding/ThemePage.cpp
    onboarding/WelcomePage.cpp
    platform/Clipboard.cpp
    platform/FileDialogs.cpp
    platform/KeyChord.cpp
    platform/KnownFolders.cpp
    platform/PopupWindow.cpp
    platform/Resources.cpp
    platform/ShellThumbnail.cpp
    platform/SystemTheme.cpp
    platform/Timer.cpp
    platform/Window.cpp
    platform/WorkerThread.cpp
    playback/ActivePreset.cpp
    playback/MediaFiles.cpp
    playback/Player.cpp
    playback/Queue.cpp
    player/AutoHideBar.cpp
    player/BarItems.cpp
    player/ControlBar.cpp
    player/PlayButton.cpp
    player/PlayerView.cpp
    player/PreferencesPanel.cpp
    player/QueuePanel.cpp
    player/SeekBar.cpp
    player/SeekPreview.cpp
    player/TimeLabel.cpp
    player/TitleBar.cpp
    player/TracksPanel.cpp
    player/VideoHost.cpp
    player/VideoView.cpp
    player/VolumeBar.cpp
    settings/PlaybackPreferences.cpp
    settings/SettingsStore.cpp
    style/Theme.cpp
    widgets/Animation.cpp
    widgets/Button.cpp
    widgets/CaptionButton.cpp
    widgets/DialogWindow.cpp
    widgets/DisabledArea.cpp
    widgets/Dropdown.cpp
    widgets/Expander.cpp
    widgets/Icon.cpp
    widgets/IconButton.cpp
    widgets/Label.cpp
    widgets/ListRow.cpp
    widgets/ListView.cpp
    widgets/Menu.cpp
    widgets/Overlay.cpp
    widgets/ScrollView.cpp
    widgets/Segmented.cpp
    widgets/Slider.cpp
    widgets/Stack.cpp
    widgets/Tabs.cpp
    widgets/TextField.cpp
    widgets/Toggle.cpp
    widgets/View.cpp
    widgets/WindowButtons.cpp
    # The widget gallery (sona.exe --gallery), for checking the toolkit against Figma.
    $<$<CONFIG:Debug>:gallery/GalleryWindow.cpp>
)

# The icon and the version info Windows shows as the app's name (e.g. in "Open with").
configure_file(platform/windows/sona.rc.in sona.rc @ONLY)

# Every SVG and font under assets/ is embedded as a resource named by its path there
# (e.g. "icons/play-dark.svg"); adding an asset is just adding its file.
file(GLOB_RECURSE SONA_ASSETS CONFIGURE_DEPENDS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/assets" assets/*.svg assets/*.ttf)
set(SONA_ASSETS_RC "")
set(SONA_ASSET_FILES "")
foreach(asset IN LISTS SONA_ASSETS)
    # The name is unquoted: rc.exe would keep the quotes as part of it.
    string(APPEND SONA_ASSETS_RC "${asset} RCDATA \"${CMAKE_CURRENT_SOURCE_DIR}/assets/${asset}\"\n")
    list(APPEND SONA_ASSET_FILES "${CMAKE_CURRENT_SOURCE_DIR}/assets/${asset}")
endforeach()
file(CONFIGURE OUTPUT assets.rc CONTENT "${SONA_ASSETS_RC}")
set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/assets.rc" PROPERTIES OBJECT_DEPENDS "${SONA_ASSET_FILES}")

target_sources(sona PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/sona.rc" "${CMAKE_CURRENT_BINARY_DIR}/assets.rc")

target_include_directories(sona PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

target_compile_definitions(sona PRIVATE
    UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN
    SONA_APP_NAME=L"${SONA_APP_NAME}"
    SONA_PUBLISHER=L"${SONA_PUBLISHER}"
    SONA_APP_USER_MODEL_ID=L"${SONA_APP_USER_MODEL_ID}"
    $<$<CONFIG:Debug>:SONA_GALLERY>
)

target_compile_options(sona PRIVATE /W4 /permissive- /utf-8 /Zc:__cplusplus /MP)

target_link_libraries(sona PRIVATE libvlc sqlite3 windowsapp d3d11 dxgi d2d1 dwrite dcomp dwmapi dxguid imm32 shcore shlwapi)

# The libVLC runtime next to the executable, so it runs from the build folder.
add_custom_command(TARGET sona POST_BUILD
    COMMAND ${CMAKE_COMMAND} -DLIBVLC_BIN=${LIBVLC_DIR}/bin -DDESTINATION=$<TARGET_FILE_DIR:sona>
        -P "${PROJECT_SOURCE_DIR}/cmake/DeployLibVlc.cmake"
)

add_subdirectory(platform/windows)

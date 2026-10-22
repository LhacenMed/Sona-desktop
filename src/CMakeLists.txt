qt_add_executable(sona
    main.cpp
    maininterface/WindowCommands.h
    maininterface/WindowCommands.cpp
    player/PlayerController.h
    player/PlayerController.cpp
    player/VideoSurface.h
    player/VideoSurface.cpp
    style/ThemedSvgProvider.h
    style/ThemedSvgProvider.cpp
)

if(WIN32)
    # The icon and the version info Windows shows as the app's name (e.g. in "Open with").
    configure_file(platform/windows/sona.rc.in sona.rc @ONLY)
    target_sources(sona PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/sona.rc")
endif()

set_source_files_properties(
    settings/qml/AppSettings.qml
    style/qml/Theme.qml
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE
)

# Every SVG and font under assets/ is bundled; adding an icon is just adding its file.
file(GLOB_RECURSE SONA_ASSETS CONFIGURE_DEPENDS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    assets/*.svg
    assets/*.ttf
)

qt_add_qml_module(sona
    URI Sona
    VERSION 1.0
    QML_FILES
        maininterface/qml/CaptionButton.qml
        maininterface/qml/KeyboardShortcuts.qml
        maininterface/qml/MainWindow.qml
        maininterface/qml/TitleBar.qml
        onboarding/qml/KeyCap.qml
        onboarding/qml/OnboardingWindow.qml
        onboarding/qml/ShortcutsPage.qml
        onboarding/qml/ThemePage.qml
        onboarding/qml/WelcomePage.qml
        player/qml/ControlBar.qml
        player/qml/PlayButton.qml
        player/qml/SeekBar.qml
        player/qml/TimeLabel.qml
        player/qml/VideoView.qml
        player/qml/VolumeBar.qml
        settings/qml/AppSettings.qml
        style/qml/Theme.qml
        widgets/qml/AppWindowAgent.qml
        widgets/qml/Icon.qml
        widgets/qml/IconButton.qml
        widgets/qml/IconWrapper.qml
        widgets/qml/TextButton.qml
    RESOURCES
        ${SONA_ASSETS}
)

set_target_properties(sona PROPERTIES WIN32_EXECUTABLE TRUE)

# Generated QML type registration includes headers by bare name, so every
# folder holding QML_ELEMENT classes must be on the include path.
target_include_directories(sona PRIVATE maininterface player)

target_compile_definitions(sona PRIVATE
    SONA_APP_NAME="${SONA_APP_NAME}"
    SONA_VERSION="${PROJECT_VERSION}"
    SONA_PUBLISHER="${SONA_PUBLISHER}"
    SONA_APP_USER_MODEL_ID="${SONA_APP_USER_MODEL_ID}"
)

target_link_libraries(sona PRIVATE Qt6::Quick Qt6::Qml Qt6::OpenGL Qt6::Svg QWindowKit::Quick libmpv)

# Copy libmpv-2.dll next to the executable so it runs from the build folder.
add_custom_command(TARGET sona POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${MPV_DIR}/libmpv-2.dll" "$<TARGET_FILE_DIR:sona>"
)

if(WIN32)
    add_subdirectory(platform/windows)
endif()

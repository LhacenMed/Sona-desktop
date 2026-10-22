# Sona

Desktop media app built with C++20, Qt 6 (QML) and libmpv.

## Toolchain (Windows)

| Component | Location |
| --- | --- |
| Qt 6.10.3 (MinGW 64-bit) | `C:/Qt/6.10.3/mingw_64` |
| MinGW 13.1 (g++, gdb) | `C:/Qt/Tools/mingw1310_64` |
| Ninja | `C:/Qt/Tools/Ninja` |
| CMake ≥ 3.21 | on `PATH` |
| libmpv dev package | `C:/dev/mpv-dev` ([shinchiro/mpv-winbuild-cmake](https://github.com/shinchiro/mpv-winbuild-cmake/releases)) |
| Inno Setup 7 (installer only) | `ISCC` on `PATH` (`scoop install extras/inno-setup`) |
| QWindowKit 1.5.0 | `third_party/qwindowkit` submodule: `git submodule update --init --recursive` |

Qt was installed with [aqtinstall](https://github.com/miurahr/aqtinstall):

```sh
aqt install-qt windows desktop 6.10.3 win64_mingw -O C:/Qt
aqt install-tool windows desktop tools_mingw1310 qt.tools.win64_mingw1310 -O C:/Qt
aqt install-tool windows desktop tools_ninja qt.tools.ninja -O C:/Qt
```

To use a different libmpv location, configure with `-DMPV_DIR=<path>`.

## Build

```sh
cmake --preset mingw-debug
cmake --build --preset mingw-debug
```

Use `mingw-release` for an optimized build.

In VS Code (Qt C++ Extension Pack installed): open the folder, pick the
`MinGW Debug` preset when CMake Tools asks, then press F5 to build and debug.

## Installer

```sh
cmake --preset mingw-release
cmake --build build/mingw-release --target installer
```

This stages the app with `windeployqt` in `build/mingw-release/stage` (a portable, runnable copy) and writes
`build/mingw-release/Sona-<version>-setup.exe`: a wizard that installs for the current user or all users, adds
Start-menu (and optional desktop) shortcuts, registers Sona for video files under Settings > Default apps, offers to
launch Sona, and registers an uninstaller in Settings > Apps. App name, version, publisher and taskbar identity are
set once in the top-level `CMakeLists.txt` and shared by the executable and the installer.

## Project structure

Mirrors VLC's Qt interface (`modules/gui/qt`): one folder per feature, each holding its C++ and its `qml/`.

```
src/
├── main.cpp              app bootstrap (identity, OpenGL scene graph, fonts, QML engine)
├── player/               PlayerController (libmpv core → `Player` QML singleton), VideoSurface, player UI
├── maininterface/qml/    MainWindow, TitleBar, KeyboardShortcuts (the one list of shortcuts)
├── onboarding/qml/       first-run onboarding: welcome, theme and accent, shortcut guide
├── settings/qml/         AppSettings: persisted preferences
├── widgets/qml/          Icon, IconButton, IconWrapper, TextButton, AppWindowAgent (shared controls)
├── style/                Theme (Dark/Light/System tokens from Figma) and the themed-SVG image provider
├── assets/               icons/ (from the Figma icon set), images/, fonts/, logo/
└── platform/windows/     executable icon and the Inno Setup installer
```

- Playback state lives only in mpv; `PlayerController` mirrors it to QML and forwards every action back as an mpv command.
- Colors come only from `Theme`, which follows `AppSettings` (theme and accent). Icons are referenced by name
  (`Icon { name: "play-dark" }`) and recolored for the theme by the `image://themed/` provider.
- Windows use QWindowKit (`AppWindowAgent`) so the custom title bar behaves natively: snap layouts, system menu,
  native caption tooltips, resize, Aero Snap and window animations.
- Any SVG or font dropped under `assets/` is bundled automatically. New QML files are added to `qt_add_qml_module` in `src/CMakeLists.txt`; a new folder with `QML_ELEMENT` C++ classes also goes into its `target_include_directories`.

Shortcuts are listed in `KeyboardShortcuts.qml` and shown in the onboarding guide; `Ctrl+O` opens a file and `Esc`
leaves fullscreen. Files can also be dropped on the window or passed on the command line.

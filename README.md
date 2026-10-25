# Sona

A native Windows media player: C++20, Win32, Direct2D, DirectWrite and DirectComposition for the
interface, libVLC for playback. The design is the Figma "VLC Redesign" file.

## Toolchain

| Component | Location |
| --- | --- |
| Visual Studio 2022 Build Tools (C++ workload, Windows 11 SDK) | `winget install Microsoft.VisualStudio.2022.BuildTools` |
| CMake ≥ 3.21 | on `PATH` |
| libVLC 3.0 SDK and runtime | `C:/dev/libvlc-3.0.24` (`include/`, `lib/`, `bin/`) |
| Inno Setup 7 (installer only) | `ISCC` on `PATH` (`scoop install extras/inno-setup`) |

libVLC comes from the VLC Windows release archive
([vlc-3.0.24-win64.7z](https://download.videolan.org/pub/videolan/vlc/3.0.24/win64/)): its `sdk/include` and
`sdk/lib` become `include/` and `lib/`, and `libvlc.dll`, `libvlccore.dll`, `vlc-cache-gen.exe`, `plugins/`
(without `plugins/gui`), `lua/` and `hrtfs/` go in `bin/`. To use another location, configure with
`-DLIBVLC_DIR=<path>`.

## Build

```sh
cmake --preset msvc
cmake --build --preset debug
```

Use the `release` build preset for an optimized build. The first build copies the libVLC runtime next to
`sona.exe` and generates its plugin cache. In VS Code (C/C++ and CMake Tools extensions): open the folder, then
press F5.

## Installer

```sh
cmake --build --preset release --target installer
```

This stages `sona.exe` with the libVLC runtime in `build/msvc/stage` (a portable, runnable copy) and writes
`build/msvc/Sona-<version>-setup.exe`: a wizard that installs for the current user or all users, adds Start-menu
(and optional desktop) shortcuts, registers Sona for video files under Settings > Default apps, and registers an
uninstaller. App name, version, publisher and taskbar identity are set once in the top-level `CMakeLists.txt`.

## Design source

`python tools/figma_export.py` exports the whole Figma file to `design/` (gitignored): the node tree
(`figma.json`), an index of frames and components, PNG renders of every screen and SVGs of every component.
It needs a Figma personal access token in `FIGMA_TOKEN`.

The structure and the plan for the remaining features are in [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md).

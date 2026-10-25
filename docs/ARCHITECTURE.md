# Sona architecture

Sona is a native Windows app. The interface is a small retained-mode view toolkit drawn with Direct2D
and composited with DirectComposition; playback is libVLC 3. There is no UI framework in between, which
is what keeps startup and memory low.

## Modules

Each folder under `src/` is one module. Dependencies only point down this list, never up or sideways
between features:

| Module | What it owns |
| --- | --- |
| `app/` | `wWinMain`: creates the services below in order and runs the message loop. |
| `core/` | Platform-free primitives: `Geometry` (DIPs), `Color`, `Signal`/`Subscription`, UTF-8 conversion. |
| `platform/` | Win32: `Window` (custom frame with native behavior), embedded `Resources`, `FileDialogs`, `KnownFolders`, `SystemTheme`, installer files under `windows/`. |
| `graphics/` | `GraphicsDevice` (D3D11, D2D, DComp, DWrite with the bundled font), `WindowSurface` (per-window swap chain), `Canvas` (the drawing API). |
| `style/` | The design system: `Theme` (Figma tokens resolved for theme and accent), `Typography`. |
| `settings/` | `SettingsStore` (registry) and `AppSettings` (every preference, declared once). |
| `widgets/` | The toolkit: `View`, `Row`, `Label`, `Icon`, `IconButton`, `TextButton`. |
| `playback/` | `Player`: the only code that includes libVLC. |
| `player/`, `maininterface/`, `onboarding/` | Feature UI: the control bar and video, the main window and title bar, first-run onboarding. |

## Rules that keep it consistent

- **One source of truth.** Playback state lives in libVLC (`Player` mirrors it). Preferences live in the
  registry (`SettingsStore` reads it directly). Colors live in `Theme`. Shortcuts live in
  `KeyboardShortcuts` and drive both the key handling and the onboarding guide.
- **Views read shared state when they paint.** They subscribe to a `Signal` and call `invalidate()`; the
  window redraws on the next `WM_PAINT`. Nothing copies state into a view except text labels.
- **The title bar is the native caption.** `Window::setNativeCaption` extends the DWM frame under the
  title bar: Windows paints its background in the caption colour and draws and runs the minimize,
  maximize and close buttons (the system's look, Snap Layouts, animations), and the bar's views leave
  their background transparent. The bar is as tall as those buttons (`Window::captionHeight`, which
  follows the window state and DPI), its own buttons are `CaptionButton`s (system icon font, native
  hover fills) and its text is in the system font. Views declare a `HitRole` (`Caption`, `SystemMenu`)
  for the rest of the bar. The buttons' size is measured on a cloaked stand-in window, so the first
  frame is laid out before a plain first showing, which is what plays Windows' open animation.
- **libVLC runs off the UI thread.** `Player` queues every command on its engine thread and marshals every
  libVLC event back to the UI thread. Never call libVLC from a view.
- **Sizes are DIPs from Figma.** Views use the Figma numbers; `Window` scales to the monitor's DPI.

## Adding things

- **A preference**: one line in `settings/AppSettings.h`.
- **A design token**: a field in `Palette` and its value in `makePalette` (`style/Theme.cpp`).
- **An icon or image**: drop the SVG under `src/assets/`; it's embedded automatically. Draw it with
  `drawIcon(canvas, L"name", ...)` or `canvas.drawSvg(L"images/name.svg", ...)`. Export icons from
  `design/components/Icon [42-356]/`.
- **A shortcut**: a binding in `maininterface/KeyboardShortcuts.cpp`; give it a `KeyChord` and an action
  once the feature exists.
- **A playback capability**: a method on `Player` that `post`s the libVLC call; state that the UI shows
  becomes a field updated in `Player::sync` or `handleEvent`.
- **A screen or panel**: a folder under `src/` with `View` subclasses, laid out with the Figma numbers.
  A separate top-level window derives from `Window` (see `onboarding/OnboardingWindow`).

## Feature map

The remaining features of the Figma file (`design/index.md`) and of VLC (`AndroidStudioProjects/VLC`,
`modules/gui/qt`), with the module each will live in and what libVLC 3 provides:

| Feature | Figma frames | VLC source | Module | libVLC 3 |
| --- | --- | --- | --- | --- |
| Queue (play next, drag to reorder) | Player 970:3308… | `playlist/` | `queue/` | `libvlc_media_list`, `libvlc_media_list_player` |
| Library: videos, music, streams, playlists, pins, recents, home | Player 4767:23726, sidebar 848:1996 | `medialibrary/` | `library/` | none: VLC's media library isn't in libVLC 3; index folders ourselves (`libvlc_media_parse_with_options` for metadata) |
| Subtitles: add to file/folder/playlist, auto-match, presets, timing, style | Add Subtitles 1911:7545…, Settings - subtitles | `dialogs/` | `subtitles/` | `libvlc_media_player_add_slave`, `libvlc_video_set_spu_delay`, `libvlc_video_set_spu` |
| Audio panel: tracks, output device, delay, equalizer, filters | Audio - Equalizer 6114:13610… | `player/`, `dialogs/extended` | `audio/` | `libvlc_audio_equalizer_*`, `libvlc_audio_output_device_*`, `libvlc_audio_set_delay` |
| Video panel: tracks, crop, aspect, rotate, adjust, HDR | Video - Colors 614:1430… | `dialogs/extended` | `video/` | `libvlc_video_set_adjust_*`, `libvlc_video_set_crop_geometry`, `libvlc_video_set_aspect_ratio` |
| Playback panel: speed, sleep timer, bookmarks, chapters | Player - customize… | `dialogs/bookmarks` | `player/` | `libvlc_media_player_set_rate`, `libvlc_media_player_*_chapter*` (bookmarks need our own store) |
| Open dialogs: file, folder, disc, network stream, capture device | Open network stream 6216:13614 | `dialogs/open` | `dialogs/` | `libvlc_media_new_location` (`dshow://`, `dvd://`, …) |
| Network discovery (UPnP, SMB, SAP) | sidebar | `network/` | `network/` | `libvlc_media_discoverer_*` |
| Stream to a device (Chromecast), convert, record | Settings - recording 6571:16429 | `dialogs/sout` | `streaming/` | `libvlc_renderer_discoverer_*`, `:sout=` media options |
| Settings (13 categories, info on every option) | Settings - global 999:5047 … extensions 6575:17137 | `dialogs/preferences` | `preferences/` | the registry for our options; libVLC options at `libvlc_new` / per media |
| Keyboard shortcuts dialog | Keyboard Shortcuts 2437:14832 | `dialogs/preferences` (hotkeys) | `maininterface/` | — |
| Layout customization | Player - customize 2107:5604… | `dialogs/toolbar` | `customize/` | — |
| Mini player | Miniplayer 970:3091 | `maininterface/` | `player/` | — |
| Media information, messages | — | `dialogs/mediainfo`, `dialogs/messages` | `dialogs/` | `libvlc_media_get_meta`, `libvlc_media_tracks_get`, `libvlc_log_set` |
| Extensions | Settings - extensions | `dialogs/extensions` | — | not exposed by libVLC 3 |

Some ideas in the redesign aren't libVLC features and need our own work: seek-bar thumbnails (a second
media player decoding at a timestamp), the subtitle timeline and waveform (parse the subtitle file; decode
audio), and reverse playback (not possible with libVLC).

When libVLC 4 is released, only `playback/` changes: its media library, frame callbacks and new player API
can replace the gaps above.

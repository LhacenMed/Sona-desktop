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

## Roadmap

What is still to build (every screen of the Figma file, the system pieces it needs, how each feature maps to
libVLC 3, and the order to build it in) is in `docs/ROADMAP.md`.

When libVLC 4 is released, only `playback/` changes: its media library, frame callbacks and new player API
can replace the gaps listed in the roadmap (section 6.9).

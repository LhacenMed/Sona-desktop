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
| `core/` | Platform-free primitives: `Geometry` (DIPs), `Color`, `Signal`/`Subscription`, UTF-8 conversion, `TimeText` (how times are written). |
| `platform/` | Win32: `Window` (custom frame with native behavior), embedded `Resources`, `FileDialogs`, `KnownFolders`, `SystemTheme`, `WorkerThread` (a background thread that hands results back to the UI thread), `ShellThumbnail` (Windows' thumbnail cache), installer files under `windows/`. |
| `graphics/` | `GraphicsDevice` (D3D11, D2D, DComp, DWrite with the bundled font), `WindowSurface` (per-window swap chain), `Canvas` (the drawing API), `BackdropCapture` (a window as Windows composes it, video included, for frosted popups to blur). |
| `style/` | The design system: `Theme` (Figma tokens resolved for theme and accent), `Surface` (the raised surfaces: menu, list, callout, panel; frosted ones blur what's behind them), `Typography`. |
| `settings/` | `SettingsStore` (registry) and `AppSettings` (every preference, declared once); `PlaybackPreferences` (the preferences panel's settings, declared once: control, range, fallback and when libVLC applies them). |
| `data/` | `Database` (SQLite, `%LOCALAPPDATA%\Sona\sona.db`, its migrations) and the stores on it: `Bookmarks`, `History` (each media played, where it was left), `Presets` (the preferences panel's values by name, and each folder's preset). |
| `commands/` | `CommandRegistry`: every user action (`CommandId`) with its shortcuts and what it runs. |
| `widgets/` | The toolkit: `View`, `Row`/`Column` (`Stack`), `ScrollView`, `Overlay` (tooltips, values, messages), `Menu`, `Label`, `Icon`, `IconButton`, `CaptionButton`, `WindowButtons`, `Button`, `Toggle`, `Segmented`, `Dropdown`, `Slider`, `Tabs`, `ListRow`, `TextField`, `ListView`, `Expander`, `DisabledArea`, `DialogWindow`, `Animation`. |
| `playback/` | `Player`: the only code that includes libVLC. `Queue`: what plays next, the one place media is opened from. `MediaFiles`: what counts as media and how folders expand. `ActivePreset`: the preset in force, the playing media's folder's, applied to the player. |
| `player/`, `library/`, `maininterface/`, `onboarding/` | Feature UI: the library (`LibraryView`: navigation, header, Home's sections from the history by `LibraryItems`; `Thumbnails`; `HistoryRecorder`, which keeps the history as media plays), the player (`PlayerView`: video, both bars and their auto-hide; the bars built from `BarLayout` by `BarItems`; `QueuePanel`, `TracksPanel`, `PreferencesPanel`), the main window with its commands and menus (`MainCommands`, `MainMenus`), first-run onboarding. |
| `gallery/` | Debug builds only (`sona.exe --gallery`): every widget on one page, for checking against Figma. |

## Rules that keep it consistent

- **One source of truth.** Playback state lives in libVLC (`Player` mirrors it). Preferences live in the
  registry (`SettingsStore` reads it directly). Colors live in `Theme`. Commands and their shortcuts
  live in the `CommandRegistry` and drive both the key handling and the onboarding guide.
- **Views read shared state when they paint.** They subscribe to a `Signal` and call `invalidate()`; the
  window redraws on the next `WM_PAINT`. Nothing copies state into a view except text labels.
- **The title bar is views that act as Windows' caption.** So it can slide over the video like the
  control bar, minimize, maximize and close are `CaptionButton`s (`WindowButtons`: system icon font,
  native hover fills, the red close) at the size Windows gives its own (`Window::captionButtonSize`,
  measured on cloaked stand-in windows, restored and maximized, so the first frame is laid out before a
  plain first showing, which plays Windows' open animation). Views declare a `HitRole`: `Caption`,
  `SystemMenu`, and `Minimize`/`Maximize`/`Close`, which the window answers as Windows' own buttons, so
  Snap Layouts open over maximize; their non-client mouse messages are handled as the views' clicks.
- **The window knows where the pointer is over all of it** (`Window::pointerPosition`), the title bar
  and frame included, which Windows reports as non-client: leaving the content for the title bar isn't
  leaving the window.
- **Input reaches views through the window.** The mouse goes to the view under it (the `Overlay`
  first), keys to the focused view and then to the window's commands, wheel scrolls up the view
  tree until one scrolls. An open popup (`PopupWindow`, which never takes activation) gets every
  key; a popup closes on Escape or a click elsewhere, and that click does nothing else, except on a
  view that acts on popups itself (`View::actsOnPopups`: the bar's panel buttons, which close their
  panel or swap it for theirs in one click).
  Until then the content under it takes no part in the mouse (`Window::viewAt`): no hover, pointer
  shape, tooltip or scroll, as behind native menus and flyouts; a menu stays live under its submenu.
  Views are removed with `View::remove`, so the window never keeps a pointer to a dead view.
  Hover feedback eases in and out for every view (`View::hoverAmount`); widgets tint by it rather
  than by `isHovered`. Menu rows are the exception, as in native menus: their highlight moves at once.
- **A menu and its submenus are one system** (`Menu`): each submenu knows the menu it opened from,
  the open submenu follows the highlighted row once the pointer rests on it, the pointer can cross
  rows on its way into the submenu, and a click inside a menu leaves its submenu to the menu's rows.
  A menu keeps its icons' column only while one of its rows has an icon.
- **Nothing runs its own message loop inside an input handler.** A file picker (`FileDialogs`) opens
  through `Window::runLater`, once the click or key that asked for it is handled, as a native menu's
  command arrives after the menu has closed; otherwise its loop would free the menu or panel whose
  handler is still running.
- **Every floating surface is a `PopupWindow`:** menus, lists and panels (`PopupPlacement::Docked`,
  whose unused keys still reach the player's shortcuts). Each draws its `Surface` itself, frosted with
  VS Code's material (blur 12, 92% tint) over its owner captured behind it (`BackdropCapture`: kept
  warm at two frames a second, live while a popup is open, so the blur is there from the first frame),
  and opens with VS Code's menu motion, the whole popup scaling and fading together.
- **Windows own their dialogs and popups.** `Window::openWindow` keeps them alive until they close;
  a fixed-size window with an owner is modal.
- **Animation needs no timer.** An `Animation` read during paint schedules the next frame until it
  arrives; presenting waits for the display. `Timer` is for delays (tooltips, caret blink).
- **The window shows the player while the queue plays, the library otherwise.** Stopping, and the
  queue playing out, go back to the library (`MainWindow::updateScreen`).
- **Media opens through the `Queue`.** Opening, dropping and the command line all go to `Queue::open`
  or `append`; only the queue calls `Player::open`.
- **Playback starts first.** Seeks collapse into the latest target; duration reads wait until the
  opening media plays. The seek bar's preview comes from a second, silent player decoding into
  memory at keyframes (`Player::previewAt`), drawn in the overlay (`SeekPreview`); the video seeks
  once, exactly, where the bar is let go.
- **libVLC runs off the UI thread.** `Player` queues every command on its engine thread and marshals every
  libVLC event back to the UI thread. Never call libVLC from a view.
- **Sizes are DIPs from Figma.** Views use the Figma numbers; `Window` scales to the monitor's DPI.

## Adding things

- **A preference**: one line in `settings/AppSettings.h`.
- **Stored data**: a migration at the end of the list in `data/Database.cpp`, and a store class in
  `data/` that queries it.
- **A design token**: a field in `Palette` and its value in `makePalette` (`style/Theme.cpp`).
- **An icon or image**: drop the SVG under `src/assets/`; it's embedded automatically. Draw it with
  `drawIcon(canvas, L"name", ...)` or `canvas.drawSvg(L"images/name.svg", ...)`. Export icons from
  `design/components/Icon [42-356]/`.
- **A command**: an id in `commands/Command.h` and its definition (shortcuts, `run`, optional
  `enabled`) in `maininterface/MainCommands.cpp`; leave `run` empty until the feature exists.
- **A menu item**: a line in `maininterface/MainMenus.cpp`; its name, icon, shortcut and state come
  from its command.
- **A bar item**: a `BarItem` and its case in `player/BarItems.cpp`; place it in `defaultBarLayout`
  (`player/BarLayout.h`).
- **A widget**: a `View` subclass in `widgets/` with the Figma numbers, and a sample on the gallery
  page (`gallery/GalleryWindow.cpp`).
- **A playback capability**: a method on `Player` that `post`s the libVLC call; state that the UI shows
  becomes a field updated in `Player::sync` or `handleEvent`.
- **A screen or panel**: a folder under `src/` with `View` subclasses, laid out with the Figma numbers.
  A separate top-level window derives from `Window` (see `onboarding/OnboardingWindow`).

## Roadmap

What is still to build (every screen of the Figma file, the system pieces it needs, how each feature maps to
libVLC 3, and the order to build it in) is in `docs/ROADMAP.md`.

When libVLC 4 is released, only `playback/` changes: its media library, frame callbacks and new player API
can replace the gaps listed in the roadmap (section 6.9).

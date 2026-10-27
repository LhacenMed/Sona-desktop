# Sona roadmap

The plan for building Sona from where it is now to the full Figma "VLC Redesign". It covers the UI
(screen by screen), the design system, the system pieces still missing, and the order to build them
in.

- **How the code is organized and the rules it follows:** `docs/ARCHITECTURE.md`. This file doesn't
  repeat it.
- **What the design looks like:** the Figma export in `design/` (see [Reading the design](#1-reading-the-design)).
- **Why it looks that way:** the redesign video's reasoning, summarized in [Product principles](#2-product-principles).

Sona is the product name. The Figma file says "VLC" everywhere (title, "Quit VLC", "About VLC",
the cone logo); Sona uses its own name and logo (`src/assets/logo/sona-logo.svg`) in those places
and keeps everything else.

---

## Contents

1. [Reading the design](#1-reading-the-design)
2. [Product principles](#2-product-principles)
3. [Where the project is](#3-where-the-project-is)
4. [Design system](#4-design-system)
5. [UI specification](#5-ui-specification)
6. [System architecture to add](#6-system-architecture-to-add)
7. [Phased plan](#7-phased-plan)
8. [Open decisions](#8-open-decisions)
9. [Working agreement for agents](#9-working-agreement-for-agents)

---

## 1. Reading the design

`design/` is the whole Figma file exported offline (gitignored; regenerate with
`python tools/figma_export.py`, which reads `FIGMA_TOKEN` from the environment). Never use the Figma
MCP for this file; read the export.

| Path | What it is | Use it for |
| --- | --- | --- |
| `design/index.md` | Every top-level frame and component with its node id and size, and every icon variant. | Finding a node id. |
| `design/frames/Page 1/<Name> [<id>].png` | A rendered PNG of each frame (2× scale). | How a screen looks. Open it before building it. |
| `design/figma.json` | The full node tree (REST API format): `document.children[0].children` are the frames; each node has `id`, `name`, `type`, `absoluteBoundingBox`, `fills`, `strokes`, `cornerRadius`, `style` (text), `characters`, `children`. | Exact numbers: positions, sizes, radii, colours, font sizes, text. |
| `design/components/<Name> [<id>]/<variant>.svg` | Every variant of every component as SVG. | Icons (`Icon [42-356]`) and states of controls. |
| `design/fills/` | Image fills (thumbnails, cover art). | Placeholder art only. |

Node ids appear as `970:3308` in Figma and `970-3308` in file names.

**Rule:** sizes in code are the Figma numbers in DIPs (the frames are 1920×1080 at 1×). Measure in
`figma.json`, not by eye from the PNGs. Coordinates there are absolute; subtract the frame's
`absoluteBoundingBox` origin.

---

## 2. Product principles

These come from the redesign video and decide every ambiguous case:

1. **Keep VLC's identity, modernized.** Orange accent, neutral greys and black. Rectangular shapes with
   slightly rounded corners (not pill-shaped). The volume control is a cone.
2. **Two icon styles.**
   - Two-tone icons (grey plus orange) only where an icon isn't clickable: menu items and context menus.
   - Monochrome icons everywhere else, turning accent-coloured when active.
3. **A quiet UI.** The video is the point. Bars are thin and auto-hide, appearing when the pointer
   comes near the top or bottom edge and disappearing as soon as it leaves, in fullscreen too. Hide UI
   (Ctrl+\\) removes them entirely.
4. **Things where people look for them.**
   - Play/pause stays at the bottom left, slightly larger, between previous and next. It is not centred.
   - Fullscreen sits at the top right, next to the window buttons.
   - Subtitles, audio, settings and queue are at the bottom right.
5. **One or two clicks for common tasks.**
   - Subtitle and audio track switching (plus the audio output device) are each one click.
   - The settings people need while watching (delays, styles, speed, filters) are a panel, not a
     window that covers the video.
6. **Queue ≠ playlist.**
   - The queue is what plays next: drag things in.
   - Playlists are saved collections.
7. **Never leave the video.**
   - The library opens as a compact sidebar beside the video. The folder name in the title bar is its
     entry point.
   - The full library view keeps playing in a miniplayer.
8. **No duplicated menus.**
   - One main menu (⋯) holds open/convert/stream, the profile, shortcuts, customize, settings and about.
   - The right-click menu holds playback actions.
9. **Settings in human words.**
   - Basic options first, with "Advanced Options" at the bottom of the same page.
   - Every option has an info icon.
   - No numeric codes for choices; use words (for example, subtitle file-name matching).
10. **Remember per show.**
    - Subtitle and audio settings, delays and default tracks can be saved as presets.
    - Subtitles added to a folder or playlist apply to all its episodes.

---

## 3. Where the project is

**Stack:** C++20, Win32, Direct2D/DirectWrite/DirectComposition, libVLC 3.0.24, MSVC + CMake, Inno
Setup.

**Branches:** `main` is stable; work happens on `dev`.

### Built

| Area | What exists | Where |
| --- | --- | --- |
| Window | Title bar drawn as views with minimize/maximize/close at Windows' size acting as Windows' own (Snap Layouts, animations), stand-in measurement of the button size, maximized/fullscreen handling, DPI, open animation, no white flash | `platform/Window`, `widgets/WindowButtons`, `CaptionButton` |
| Rendering | One shared D3D11/D2D/DComp device, per-window swap chains drawn above the libVLC child window, SVG icons with colour swaps, Inter from resources | `graphics/` |
| Toolkit | `View` (tree, hit testing, hover/press, focus, keys, wheel, tooltips), `Row`/`Column` (padding, alignment), `ScrollView`, overlay layer with tooltips, frosted popup windows (menus, lists, panels) opening with VS Code's motion, `Animation` and `Timer`, `Label`, `Icon` (mono and two-tone), `IconButton`/`IconWrapper`, `Button` (primary, secondary, neutral), `Toggle`, `Segmented`, `Dropdown`, `Slider`, `Tabs`, `ListRow`, `TextField` (selection, clipboard, undo, IME), `DialogWindow` (modal, owned) | `widgets/`, `platform/` |
| Commands | Registry of every action and its shortcuts; keyboard handling and the onboarding guide read it (keys decided in D3) | `commands/`, `maininterface/MainCommands` |
| Widget gallery | Every widget on one page, Debug builds only: `sona.exe --gallery` | `gallery/` |
| Design tokens | Dark/Light palettes, 9 accent colours, theme mode (Light/Dark/System) | `style/Theme` |
| Settings | Registry store; 5 settings (onboardingCompleted, theme, accentIndex, mediaFolder, showRemainingTime); the preferences panel's settings, declared once with their control, range and when they apply (`PlaybackPreferences`) | `settings/` |
| Data | SQLite (vendored in `third_party/sqlite`) in `%LOCALAPPDATA%\Sona\sona.db` with versioned migrations; bookmarks per media; the playback history (positions, play counts); presets (the preferences panel's values by name) and the preset each folder plays with | `data/` |
| Playback | libVLC on an engine thread: open, play/pause, seek, volume 0–200, mute, next frame, cycle subtitle/audio track, choose any video/audio/subtitle track, add subtitle files, list and choose the audio output device, the preferences panel's settings (live where libVLC 3 allows; otherwise libVLC restarts with them once changes settle and reopens the media where it was, its tracks and subtitle files kept), the preset in force following the media's folder (`ActivePreset`), snapshot, stop on close, chapters, durations read ahead once the media plays; seeks coalesced; the seek bar's preview decoded by a second, silent player at keyframes, the video seeking once, exactly, where the bar is let go; the queue (play order, repeat off/all/one, shuffle, move, sort, remove), the one place media is opened from; folders expand to their media in Explorer's order | `playback/Player`, `Queue`, `MediaFiles` |
| Player UI | Video in its rounded frame; title and control bars built from a layout definition (`BarLayout`), fixed with the video between them or auto-hiding over it (Hide UI / Show UI; on whenever media is opened), each sliding in near its edge and out when the pointer leaves (windowed and full screen); the pointer hides over playing video; main menu and right-click menu with submenus; duration/time-left toggle; volume value while dragging; on-screen message for each shortcut; keyboard shortcuts, drag and drop (into the queue while it's open), "Open with", Open File(s) and Open Folder; the queue panel (Queue, Bookmarks, Chapters tabs) docked over the video, its button active while it's open; a click on a row plays it or jumps to it; bookmarks marked on the seek bar (hover for the name, click to jump); hovering or dragging the seek bar previews that moment and its time above the pointer (`thumbnail 2151:9673`); the tracks panel (Video, Audio, Subtitles tabs; the Audio and Subtitles buttons open it at theirs, or switch to it, in one click): the media's tracks with the current one checked, a click switching to another, subtitle files added from the computer (External Tracks), the audio output device with a refresh; the preferences panel (Playback, Video, Audio, Subtitles tabs; the gear opens it, "Show Preferences" opens it at the tracks' tab): speed, flip, rotate, zoom, aspect ratio, crop, the Essential, Crop and Colors filter groups, track languages, stereo mode, delays, the equalizer with its presets and bands, 2 Pass, subtitle speed, duration and appearance; presets chosen per folder in its footer | `player/`, `maininterface/` |
| Library | Shown while nothing plays (Figma Home `6261:15078`), stopping and the queue's end return to it: navigation (Home; the other sections dimmed until built), back/forward, breadcrumb, item count, sort (Figma's options; those needing metadata disabled), grid/list, search (Ctrl+E), more (Add File, Add Folder); Home's Continue Watching (resumes), Listen Again and Frequently Visited Folders from the history, each opening in full; thumbnails from Windows' cache | `library/` |
| Onboarding | Welcome + media folder, theme + accent with live preview, shortcuts guide | `onboarding/` |
| Packaging | Release build, libVLC deploy with plugin cache, per-user installer | `cmake/`, `platform/windows/` |

### Not built yet

Everything in [section 5](#5-ui-specification) beyond the above:

- the library's other sections (Videos, Music, Network, Playlists,
  Recent, pins), its indexer, the compact sidebar and miniplayer, all dialogs and the settings window
- music view, miniplayer, customize player, profiles, light-theme review

Toolkit pieces still missing ([section 6](#6-system-architecture-to-add)): `Menu`, `InfoIcon`,
`ColorField`, `Thumbnail`, drag and drop, images, Tab key focus traversal, and the slider's paired
value field (a `Slider` next to a `TextField`, composed where P5 needs it).

### Known deviations from Figma (intentional)

- The window's minimize, maximize and close look and act as Windows' own, at Windows' size, so the
  title bar is 37 px restored and 28 px maximized at 125% scale, not Figma's 40. The bar's other
  buttons use the system icon font to match.
- The bars sit over the video rather than beside it, so showing and hiding them never resizes the
  video.
- The right-click menu's submenus aren't drawn in Figma; each holds the existing commands of its kind.
- Popups (menus, submenus, dropdown lists and panels) are frosted with VS Code's material rather than
  Figma's background blur 50: the window behind them is captured (Windows.Graphics.Capture) and
  blurred at full resolution (blur 12) under their Figma color at 92%. Dropdown lists are frosted too
  (opaque in Figma). They open with VS Code's menu motion, faster: fading in while growing from 97%
  over 167 ms (ease-out quint) out of the corner nearest what they open from. Before Windows 11
  they're opaque. Their highlighted rows take the shared hover overlay rather than Figma's opaque
  BG/A2, which barely shows on a frosted Dark surface.
- The right-click menu shows Record as Shift+R (decision D3), not R.
- The preferences panel:
  - Settings libVLC 3 can't change (Reverse, HDR, Resolution, Compressor, Spatializer, Stereo
    Widener, Adjust Pitch, Stereo Mode's Test) and those of later phases (subtitle timeline,
    statistics, sleep timer, audio visualizer, Render, Open Advanced Settings) are drawn dimmed,
    their tooltip saying why (decision D4). The Geometry, Overlay and Advanced filter groups are
    drawn closed only in Figma, so they're dimmed too.
  - Zoom adds "Fit" (libVLC's default) before Figma's 1:1 Original; Subtitle Speed 0 fps and
    Duration Factor 0 mean the subtitles' own; text colour defaults to libVLC's white.
  - Track and language fields list their choices in a menu; the subtitle font is chosen from
    Windows' own families.
  - Settings libVLC reads only when it starts restart it, a short blink where the video was.
- The tracks panel's Audio tab has no External Tracks, as in Figma (5.3 lists it for Audio too); its
  track names are libVLC's ("Track 1 - [English]").
- "VLC" becomes "Sona" (title, menu items, onboarding text).
- The onboarding welcome text is shortened, and the logo is Sona's.

---

## 4. Design system

### 4.1 Tokens

These already live in `style/Theme` (`Palette`).

| Group | Tokens |
| --- | --- |
| Accent | `accentMain`, `bgAccentBright` (the elapsed seek fill) |
| Backgrounds | `bgA3` (bars/chrome), `bgA4` (field borders), `bgB1` (dialogs), `bgB2` (video/fields) |
| Foregrounds | `fg1`, `fg2`, `fgA1`, `fgA2` (icons, secondary text), `fgA3` (disabled/hint), `fgB1` (text on accent) |
| Lines | `dividerBright`, `volumeTrack` |
| States | `hoverOverlay`, `pressedOverlay`, close button colours |

New tokens are needed as screens arrive: sidebar background, list selection, menu background and
shadow, toggle track, slider track, input background. For each one, read the colour from `figma.json`
`fills` and add it to `Palette` with both Dark and Light values (look at `Player - Light 2425:13624`).

### 4.2 Type

Inter (Light 300, Regular 400, SemiBold 600) from `assets/fonts`. Sizes seen in the file:

| Size | Used for |
| --- | --- |
| 12 | Captions, counts ("15 items"), menu shortcuts, chip labels |
| 14 | List rows, menu items, settings labels, title bar text |
| 16 | Dialog section titles, onboarding body |
| 24 / 32 | Onboarding headings |

Confirm each one in `figma.json` (`style.fontSize`, `fontWeight`, `lineHeightPx`).

### 4.3 Icons

- **Source:** `design/components/Icon [42-356]/`, 24×24.
- **Variant axes:** `name`, `direction` (arrows, chevrons, sidebar), `state` (default / active / selected),
  `two-tone` (yes/no).
- **Monochrome icons** are drawn grey (`#8F8F8F`, swapped to `fgA2`). Their active variant uses the accent.
- **Two-tone icons** (48 of them) are grey plus orange, for menus. Export them as
  `src/assets/icons/<name>-duo.svg` and give them their own draw helper, which swaps both colours (grey
  to `fgA2`, orange to `accentMain`).
- **Coverage:** about 100 names cover every screen (see the list in `design/index.md`). Export each
  icon when the screen that uses it is built. Strip `style="…"` attributes and inline any `<use>`,
  because Direct2D's SVG renderer doesn't support them.

### 4.4 Components to build

Each one is one widget in `widgets/`, built once and reused everywhere. Figma ids are in brackets.

| Widget | Figma | Notes |
| --- | --- | --- |
| `Column` | — | Vertical twin of `Row`; most panels and settings pages need it. |
| `ScrollView` | — | Vertical scrolling with wheel/touchpad, a thin scrollbar, clipping. |
| `Toggle` | `Toggle 536:1014` | Square track with an "ON"/"OFF" label; the ON state is accent-coloured. |
| `Dropdown` | `Dropdown 2372:8300`, `EQ presets 2372:8487`, `HDR Options 2372:8733`, `sub filename 6456:13892`, `Setting Preset 2372:8834` | A field with a chevron; opens a list popup with a check on the selected item. |
| `Segmented` | Settings > Theme, Flip, "Relative/Pixels" `Values 804:1208` | Joined buttons; the selected one is accent-filled. |
| `Slider` | Speed (0.25–2 with ticks), delays (−10 to +10 s), volume step | Track, accent fill, thumb, optional ticks and labels, paired value field. |
| `TextField` | Settings fields, dialog inputs, search | Caret, selection, IME, clipboard, placeholder. |
| `ColorField` | Subtitle colours | Hex text + swatch; opens the Windows colour picker. |
| `Tabs` | Panel tab strips | Icon + label tabs; the active one is accent-coloured with an underline. |
| `ListRow`, `ListView` | Library rows, queue rows, tracks | Icon/number/thumbnail, text, trailing text, hover, selected and current. `ListView`: click selects, double click activates, Delete removes, drag reorders. |
| Panels | Control-bar panels | A `PopupWindow` docked over the video (`PopupPlacement::Docked`), frosted. |
| `Menu` | `menu 6062:13354`, `right-click menu 6065:13616`, context menus | Top-level popup window: two-tone icon, label, shortcut, chevron submenus, dividers, keyboard navigation. |
| `DialogFrame` | Settings, shortcuts, subtitles, network, playlist | 1048 wide; title + ✕, content, footer with Cancel/Save (Save is accent-coloured when enabled). |
| `InfoIcon` | Settings rows | ⓘ with a tooltip showing the option's explanation. |
| `Tooltip` | Everywhere | Delayed hover text. |
| `Thumbnail` | `media 2404:9167`, `playlist 2459:9352`, `thumbnail 2151:9673` | Rounded image with a progress strip and a duration/time label. |
| `Button` | `Button 500:3063` | Wide neutral button ("Show Preferences", "Advanced Options", "Show Advanced Settings"). |

---

## 5. UI specification

Each surface below lists what it contains, how it behaves and the Figma frames to build it from.

### 5.1 Main window

**Frames:** `Player 247:445` (base), `187:328` (with compact library), `Player - hide UI 3328:11554`,
`Player - Light 2425:13624`.

```
┌ Title bar ───────────────────────────────────────────────────────────────┐
│ [logo] [⋯] [sidebar] │ [📁 folder]          title           [⤢][─][▢][✕] │
├──────────────────────────────────────────────────────────────────────────┤
│ [library sidebar, optional] │            video (rounded frame)           │
│                             │                       [panel, optional] ──┐│
├──────────────────────────────────────────────────────────────────────────┤
│ [⏮][▶][⏭] 00:02:10 ━━━━━━━━thick seek━━━━━━━ 00:25:40 [🔊◢] [♪][CC][⚙][☰] │
└ Control bar ─────────────────────────────────────────────────────────────┘
```

**Title bar** (built):

- **logo:** opens the system menu.
- **⋯:** opens the main menu (5.6).
- **sidebar:** toggles the compact library (5.5).
- **📁 folder:** the current media's folder name. Clicking it opens the compact library at that folder.
- **title:** the media title.
- **⤢:** fullscreen.
- **Windows buttons:** minimize, maximize, close.

**Auto-hide** (built): one switch. Hide UI (Ctrl+\\, right-click menu) turns it on and Show UI turns
it off. It's off for an empty launch and turns on whenever media is opened ("Open with", Open File(s),
Open Folder, a drop), and works the same windowed, maximized and in full screen.

- Off, the bars stay fixed with the video between them (`Player 247:445`).
- On, the video fills the window and the bars sit over it, each auto-hiding on its own edge
  (`player/AutoHideBar`): it shows as soon as the pointer is where the bar is, stays while the
  pointer is on it or within 24 px past it, and hides 400 ms after the pointer has gone farther
  unless it comes back (the hysteresis mpv's OSC and docking panels use, so brushing past never
  flickers). Settings > Interface will also allow hiding after 0–10 s (P8).
- Neither moves while a press or a popup is under way; both stay while there's no video to watch.
- The video's rounded frame runs between the shown bars, so its corners stay rounded as they slide.
- In fullscreen they work the same way (VLC hides the title; Sona doesn't).
- The pointer hides over playing video after 2 s at rest with the bars out.

**Resume prompt** (moved to P6, with the stored positions): "You've watched till 00:45:08 already.
Continue Watching" (hidden layer in `247:445`).

- It appears when you reopen a partly watched file.
- Place it near the playback controls, where the video notes people will see it, not in a far corner.

### 5.2 Control bar

**Component:** `Controls 491:1780`, with 15 variants:

- default, queue, bookmarks, chapters, video, audio, subtitles, playback
- video settings, audio settings, subtitle settings
- sub timeline, waveform, music, Variant10

Height 54. Left to right:

| Element | Behaviour |
| --- | --- |
| Previous / Play / Next | Play is the larger accent button (`play 848:1712`), grey while playing. Previous/next move through the queue. |
| Current time | `Current Time 1904:7238`, e.g. "00:02:10". |
| Seek bar | Thick bar with an accent-dim elapsed fill and an accent thumb. Hover shows a thumbnail preview with its time (`thumbnail 2151:9673`). Optional overlays: subtitle cue marks ("sub timeline" variant) and an audio waveform ("waveform" variant). |
| Duration | `Duration 1905:7312`; click toggles total / remaining ("-00:23:30"). |
| Volume | Speaker icon (click to mute) and the cone (`volume 244:184`, 0–200% in 10% steps). Dragging shows a value bubble (`audio value 2245:9805`, "100 %"). |
| Audio | Opens the tracks popover on its Audio tab. |
| Subtitles | Opens the tracks popover on its Subtitles tab. |
| Playback settings (gear) | Opens the preferences panel. |
| Queue (lists) | Opens the queue panel. |

- While a button's panel is open, the button shows its active (accent) state. Opening one panel
  closes any other.
- **Music variant:** adds −10 s / +10 s buttons around play, and uses the waveform seek bar.
- **Customizable** (5.10): the order and content of both bars come from a layout definition. Build the
  bars from that definition from the start, not hard-coded.

### 5.3 Control-bar panels

These are frosted popup windows docked above the control bar's right side, about 400 wide
(`PopupPlacement::Docked`). Esc or a click outside closes them.

**Tracks popover.** Tabs: Video · Audio · Subtitles.

- **Embedded Tracks:** a list with a check on the current track ("None" first).
- **External Tracks** (Subtitles, Audio):
  - files added for this media, with edit (✎) and add (+)
  - add opens Add Subtitles (5.7)
- **Audio tab:** also lists Output Device (Speakers, Headphones, …) with a refresh icon.
- **Footer:** "Show Preferences", which switches to the preferences panel on the same tab.

**Preferences panel** (the "sidebar" in the video). Tabs: Playback · Video · Audio · Subtitles.

- **Playback:**
  - toggles: Show Subtitle Timeline on Seekbar, Show Statistics, Reverse (not possible with libVLC 3;
    hide it or disable it with an explanation)
  - Sleep Timer (dropdown) and Audio Visualizer (Disabled, Spectrometer, Scope, Spectrum, Waveform)
  - Speed: value field and a slider from 0.25 to 2 with ticks
- **Video:**
  - Defaults:
    - Video Track
    - Flip (None / Horizontal / Vertical)
    - Rotate (0 deg)
    - Zoom (1:1 Original)
    - HDR (Auto, Off, HDR10, HDR10+, Dolby Vision, HLG)
    - Resolution (Best Available)
    - Aspect Ratio
    - Crop
    - Render (Local, or a cast target)
  - Filters, each a collapsible "Expand Settings" group:
    - Essential: hue, brightness, contrast, saturation, gamma, sharpen, banding removal, film grain
    - Colors: color extraction, threshold, saturation, similarity, negate, posterize, gradient,
      cartoon, sepia
    - Geometry
    - Overlay
    - Advanced
- **Audio:**
  - Defaults: Audio Track, Audio Language, Stereo Mode (Mono…, with a Test button)
  - Synchronization: Audio Track Delay (value plus a −10 to +10 s slider)
  - Filters:
    - Equalizer: preset dropdown, then "Expand Settings" for the 10-band EQ with Preamp
    - 2 Pass
    - Compressor
    - Spatializer
    - Stereo Widener
    - Adjust Pitch (semitones)
- **Subtitles:**
  - Defaults: Subtitle Track, Subtitle Language
  - Synchronization: Subtitle Track Delay (slider), Subtitle Speed (fps), Subtitle Duration Factor
  - Appearance:
    - Embedded Sub Format
    - Subtitle Font, Text Color, Outline Color, Subtitle Offset
    - Size Values (Relative / Pixels) and Font Size
    - Outline Thickness
    - Background Opacity
- **Bottom of every tab:**
  - a Preset selector ("Default Preset", "New Preset", saved ones such as "Anime"). Presets are named
    bundles of these values, saved per show.
  - "Open Advanced Settings", which opens Settings at the matching category.
- **Filter groups:** components `Video - Essential 536:1855`, `Video - Crop 543:990`,
  `Video - Colors 614:1430`, `Audio - Equalizer 6114:13610`, `Audio - Compressor Settings 536:1513`,
  `Audio - Spatializer Settings 536:1703`, `Audio - Stereo Widener 612:1223`. Each has a collapsed
  and an expanded variant.

**Queue panel.** Tabs: Queue · Bookmarks · Chapters.

- **Queue:**
  - "4 items - 2h 3m"
  - actions: sort, repeat, shuffle, remove (−), add (+)
  - rows: drag handle, name, duration; the current item is highlighted
  - drag to reorder; drop files or library items in
- **Bookmarks:**
  - "2 Bookmarks", with − and +
  - rows: name, time (hh:mm:ss:ms); click to jump
  - Ctrl+B adds a bookmark
- **Chapters:** numbered (accent number), name, start time; click to jump.

### 5.4 Music playback

**Frames:** `Player - Music 4767:23726`, `4778:25123`.

- When the media has no video, the video area shows large square cover art (rounded) over a blurred
  enlargement of the same art, with "title - artist" under it.
- The control bar uses the music variant (5.2).
- The compact library works the same way.

### 5.5 Library

**Compact sidebar** (inside the player). Frames: `Player 187:328`, `Player - Light 2425:13624`; all
states in `sidebar 848:1996`.

- **Rail:** an icon column with Home, Videos, Music, Network, Playlists, Recent, and + (add/pin).
  The active one is accent-coloured.
- **Header:**
  - back, forward and a breadcrumb ("… › The Wand › S01"); clicking a crumb goes to that level
  - ⋯ (the section's menu: Add File, Add Folder, …)
  - pop-out (opens the full library)
- **Subheader:** "15 items", sort (`Sorting Options 856:2215`), view options (`View Options 858:3290`:
  Grid / List), search.
- **Content:** rows with thumbnail and name. Watched items are dimmed and partly watched ones show
  progress.

**Full library** (replaces the video while browsing). Frames: `Player 970:2703` (grid), `970:3308`
and `970:3792` (list), `970:4028` (nothing playing), `6261:15078` (Home).

- **Empty launch:** opening Sona without media shows the full library on Home (`6261:15078`, or
  `970:4028` with nothing playing) instead of the empty player.
- **Left navigation:** Home, Videos, Music, Network, Playlists, Recent, "Add item" (pins).
- **Top row:** back/forward, breadcrumb, count, sort, view, search field, +, ⋯.
- **Grid:** 16:9 thumbnails with a progress strip.
- **Miniplayer** (`Miniplayer 970:3091`, 5 variants):
  - docked bottom-right (or bottom-left in `970:3792`)
  - shows the thumbnail and progress, the title, folder and playlist chips, and a queue toggle
  - on hover: ✕, pop-out (back to the player) and previous/play/next (replay when finished)
  - the queue toggle expands an attached Queue list

**Sections** (content in `sidebar 848:1996`):

- **Home:** Continue Watching, Listen Again, Frequently Visited Folders, Frequently Visited
  Playlists, Frequently Listened Artists, Latest Streams. Each has a › to see all.
- **Videos:** the folders added by the user (Shows, Films, Anime…), browsable down to files.
- **Music:**
  - Songs, Albums, Artists, Genres (by metadata)
  - folders under Songs
- **Network:**
  - Add Network Stream, then saved streams and discovered sources (Podcasts, Radio, UPnP/SMB)
  - an M3U URL becomes a stream playlist
- **Playlists:** Create playlist, then the playlists.
- **Recent:** recently played items.
- **Pins:** anything pinned with "+" or the context menu appears in the rail and navigation.

**Context menus:**

- **File** (`2404:13340`): Play, Mark as Watched, Favorite, Stream, Convert, Add to Queue, Add to
  Playlist, Show in File Explorer, Go to Source, Add Subtitle Tracks, Add Audio Tracks, Properties &
  Metadata.
- **Folder** (`2139:8935`): Add File, Add Folder, Edit Subtitle Tracks, Edit Audio Tracks, Resync,
  Info.
- **Playlist** (`2459:9389`): Play, Favorite, Shuffle, Export Playlist, Add to Queue, Add Subtitle
  Tracks, Add Audio Tracks, Properties & Metadata.

**Sorting options:** Unwatched First, Watched First, Newest/Oldest First, Title A–Z/Z–A, Artist,
Album, Track Number, URI, Disc Number (ascending/descending).

### 5.6 Main menu and right-click menu

**Main menu** (`menu 6062:13354`), opened by ⋯. Two-tone icons and shortcuts:

| Item | Shortcut | Notes |
| --- | --- | --- |
| Open File(s) | Ctrl+O | |
| Open Folder | Ctrl+F | |
| Open Network Stream | Ctrl+N | |
| Open Disc | Ctrl+D | |
| Open Capture Device | Ctrl+C | |
| Convert | Ctrl+R | |
| Open Recent | › | |
| Profile (Movies) | › | Submenu: profiles with a check, then New (`Profiles 6648:20666`). |
| Keyboard Shortcuts | | |
| Customize Player | | |
| Settings | Ctrl+P | |
| About Sona | Shift+F1 | |

**Right-click menu** (`right-click menu 6065:13616`), on the video:

| Item | Shortcut |
| --- | --- |
| Play | Space |
| Stop | S |
| Previous Item | P |
| Next Item | N |
| Record | R |
| Hide UI | Ctrl+\\ |
| Playback | › |
| Video | › |
| Audio | › |
| Subtitle | › |
| More Settings | › |
| Playlist | › |
| Open Media | › |
| Quit Sona | Ctrl+Q |

The keys follow decision D3: one map for the menus, the keyboard and the guide.

**On-screen message:** when a shortcut runs, show the action and the key that triggered it
(Settings > Subtitles > On-Screen Messages).

### 5.7 Dialogs

All use `DialogFrame` (4.4). The width is 1048 unless noted.

| Dialog | Frame | Content |
| --- | --- | --- |
| Add Subtitles | `1911:7545`, `3336:11929`, `1919:8707`, `1919:9278` | Header dropdowns: target (`target 1959:5325`: To This File / Folder / Playlist) and origin (`origin 1963:5465`: From Computer / From OpenSubtitles / From Link…), plus a location or search field and a language. **Folder/playlist:** two columns, videos on the left matched to subtitle files on the right by dashed lines; drag to fix pairs; saved for future episodes. **File:** a list of candidates with a check. **OpenSubtitles:** search results. |
| Keyboard Shortcuts | `2437:14832` | Search, field filter (All Fields), columns Action / Hotkey / Global Hotkey with edit buttons, grouped (Basic, Playback, …). |
| Open Network Stream | `6216:13614` | Network URL (multi-line, several links) and an optional Playlist Name. |
| New Playlist (713×589) | `6259:14954` | Name and thumbnail (+), then Select Files (a drop zone with +). |
| Customize Player | `Player - customize 2107:5604` … `2160:8732`, `6587:19923` | See 5.10. |
| Open Disc / Capture Device / Convert / Stream | not drawn | Same frame and style; contents from VLC's open dialog, split into separate dialogs. |
| About | not drawn | Logo, version, credits, libVLC version and licence. |

### 5.8 Settings window

**Frames:** `Settings - global 999:5047` (and its advanced variant `3046:9282`), then one frame per
category, each with an advanced variant.

Layout: 1048×865.

- **Left:** a Search field, then categories grouped under headings with icons.
- **Right:** the category title, then sections. Each row is a label with an ⓘ on the left and a
  control on the right (toggle, dropdown, segmented, slider, path field with Browse, button).
- **Bottom:** an "Advanced Options ›" button expands the advanced rows in place.

| Group | Category | Basic sections and options | Advanced |
| --- | --- | --- | --- |
| App | **General** | Appearance: Interface language, Theme (Dark/Light/System), Accent color (9 swatches). Startup: Show confirmation and error dialogs, Check for updates automatically. Diagnostics: Developer mode, Save a log, Log file, Log details. | Show minor warnings, Additional command line options, Log format, Collect playback statistics, Open configurations folder, Reset all settings; Developer: Write to the system log, debug messages, identity, facility |
| | **Interface** | Skin (custom skin, file). Window: Play video inside main window, Resize window to fit video, Remember window size and position, Keep window above other windows, Show window decorations, Show the file name in the title bar, Open fullscreen on. Playback Controls: Start in compact mode, Auto-hide the controls, Hide after (0–10 s), Show controls in fullscreen. Notifications and System Tray: tray icon, Start minimized, Track-change popup, Popup duration. Playlist: Show the playlist as a tree. | Window opacity, Pause when minimized, Bring to front on playback, Cover art for audio, File browser in open dialog, App icon changes, Fullscreen controls opacity, Fullscreen controls sensitivity |
| | **Privacy** | History: Save recently played items, Remember the playback position, Never remember items matching, Clear history now. Fetch titles and cover art from the internet. last.fm (submit, username, URL). | — |
| | **Controls** | Keyboard Shortcuts (Defaults, Edit, which opens the shortcuts dialog; Reset all). Skip Amounts: very short / short / medium / long. Mouse: single click, double click, scroll wheel, horizontal scroll, reverse direction. Remotes: Web remote control, Port. | — |
| Media | **Playback** | Startup: Continue where you left off, Start playing as soon as a file opens, Default playback speed, Pause when another app uses audio. Tracks: Preferred video quality, menu language, Default audio track, Default subtitle track. Order: Repeat, Shuffle, Stop after each item, When playback finishes. Seeking: Seek precisely, Show thumbnail while scrubbing. Files & Discs: Default optical drive, Repair damaged AVI files, Load matching .mkv files. | Time-shift buffer folder and size |
| | **Video** | Output: Enable video, Start in fullscreen, Stop the screensaver, Play as desktop wallpaper. Picture: Hue, Brightness, Contrast, Saturation, Gamma, Reset. Deinterlacing (mode, method). Framing: Aspect ratio, Crop, Scale to the window, Zoom, Position. Snapshots: Save to, prefix, Format, Size, sequential numbering, preview. | Custom aspect/crop ratios, Monitor pixel aspect ratio |
| | **Audio** | Output: Enable audio, Output device, Speaker setup. Volume: Maximum volume (100/200/300%), Startup volume (remember / fixed), Volume step. Loudness: Normalize, Target level, Night mode, Replay gain, Preamp. Effects: Equalizer (preset; Open Equalizer expands the 10-band EQ in place), Two-pass, Spatial audio, Headphone virtualization, Stereo widening, Compressor, Pitch shift, Chorus/flanger, Karaoke. Tracks: Preferred audio language, Audio delay. Visualization: Visualizer. | Pass compressed audio to the receiver, Detect Dolby Surround, Resampling quality, Media role, Visualization size |
| | **Subtitles** | Tracks & Languages: Enable, Preferred language, Text encoding. Loading: Load automatically, File name matching (in words: Off / Any title / Contains title / Title with extra characters / Exact title match). Appearance: Font, Size values (Relative/Pixels), Font size, Bold, Text color and opacity, Outline thickness and colour, Shadow (colour, distance), Background (colour, opacity), Vertical position. Timing: Subtitle delay (−5 to +5 s). On-Screen Messages: feedback for shortcuts and its duration, file name at start and its duration. | Load subtitles from these folders, Ignore ASS/SSA styling, Shadow angle, Text direction, Monospace font |
| | **Library** | Opening Files: Only one window, Apply to files from the file manager, Add to playlist instead of replacing. When opening a folder: subfolders, hidden files. Metadata & Artwork: fetch source, Use the media library. | — |
| System | **Network** | Buffering (per source: local, discs, network, live capture). Proxy: HTTP proxy, password. Security: Verify certificates, Cipher preference. | — |
| | **Performance** | Decoding: Hardware-accelerated decoding, post-processing, skip H.264 loop filter, decoding threads. Smooth Playback: drop frames, real-time priority, process priority. Rescan plugins. | Developer: encoder, decoder, clock synchronisation, jitter, reference averaging |
| | **Recording** | Save recordings to, record without re-encoding. Casting: discover devices, quality. Streaming: play locally, include audio/video/subtitles, encoding preset, tuning. | — |
| | **Extensions** | Add-ons list (Find more, Add, Remove), Load add-ons on startup. | Plugins (libVLC modules, read-only) |

Every option must say whether it's an app preference (stored in `SettingsStore`), a libVLC option
(applied at `libvlc_new` or per media), or both. Options that need a restart say so on the row.

### 5.9 Onboarding (built)

**Frames:** `Onboarding 1899:3055` (welcome + folder), `1902:3670` (theme + accent with preview;
`1902:4020`…`6465` are the accent variants), `1902:6770` (shortcuts).

**Remaining:** keep the preview in step with the final player layout; read the shortcuts guide from
the commands registry.

### 5.10 Customize Player and profiles

**Customize** (`Player - customize 2107:5604` and variants):

- A dialog over the player titled "Customize Player": "Drag items around and out of the box below to
  re-order them. Drag items into the box to remove them from the player."
- While it's open, both bars show dashed outlines around each slot.
- The item palette: Spacer, Divider, Playback Speed, Forward/Backward 10 s and 15 s, Stop, Repeat,
  Shuffle, Record, Snapshot, Picture in Picture, HDR, Zoom, Right Sidebar, Quit, Settings, Go Home,
  Thumbnail, File Name, Artist Name, Folder Name, Playlist Name, Properties, Replay, Chapters,
  Favorite, Bookmark, Bookmarks, Playlists.
- Footer: Cancel / Save.
- Variants `2160:6137`…`8732` show an item being dragged and alternative layouts.

**Profiles** (`Profiles 6648:20666`):

- Named sets of all settings plus the layout (Movies, Shows, Music, Work), switched from the main
  menu's Profile submenu.
- New creates one from the current settings.

### 5.11 Light theme

`Player - Light 2425:13624` shows the light palette on the player and the compact library. Every new
token needs a Light value; check each screen in both themes.

---

## 6. System architecture to add

These are the pieces the screens above need, in the order they unblock work. Each follows
`docs/ARCHITECTURE.md` (modules, one source of truth, libVLC only in `playback/`).

### 6.1 Commands registry (`commands/`)

Every user action has a single entry:

- `id`, label, monochrome and two-tone icon names
- default shortcut(s), and `run`/`enabled`/`checked` callbacks

Menus, the right-click menu, keyboard handling, the shortcuts dialog, the onboarding guide, customize
player and the control-bar slots all read this registry. It replaces the current
`maininterface/KeyboardShortcuts` list. Shortcut overrides are stored per profile.

### 6.2 Toolkit additions (`widgets/`, `platform/`)

- **Layout:** `Column`, plus alignment, padding and gap on `Row`/`Column`.
- **`ScrollView`.**
- **Overlay layer:** a top layer in each window for popovers, tooltips, drag previews and toasts.
  Hit-tested first; click-outside dismisses.
- **Popup windows:** a frameless, owned top-level `Window` for menus and dropdown lists, so they can
  extend past the main window like native menus. Keyboard navigation, submenus, dismissal on focus
  loss.
- **Text input:** caret, selection, IME (`WM_IME_*` / TSF), clipboard, undo. Needed by search,
  settings and dialogs.
- **Timers and animation:** a frame clock driven by the window (`SetTimer` or composition), for
  auto-hide, fades and smooth scroll.
- **Drag and drop:** internal drag (queue reorder, library to queue, customize), plus OLE drop
  targets for files from Explorer, which replace `WM_DROPFILES`.
- **Images:** WIC decoding to D2D bitmaps with a size-keyed cache. Thumbnails via
  `IShellItemImageFactory` (Windows' own thumbnail cache: fast, native), with a libVLC snapshot as
  the fallback.

### 6.3 Playback (`playback/`)

- **Queue** (built): our own `Queue` model on the UI thread (items, current, repeat, shuffle, move, sort,
  add, remove, a `changed` signal) playing one item at a time through `Player`'s media player and moving
  on when it reports `ended`. Not `libvlc_media_list_player`: in libVLC 3 it has no shuffle and no move,
  tracks its current item by index (reordering breaks it) and can't be called from its own events.
- **Tracks and devices:** list and select video/audio/subtitle tracks; add external
  subtitles/audio (`libvlc_media_player_add_slave`); audio output devices.
- **Effects:** rate, audio/subtitle delay, equalizer (`libvlc_audio_equalizer_*`), video adjust,
  aspect/crop/zoom/flip/rotate (where libVLC 3 allows; otherwise video filter options), deinterlace.
- **Chapters and titles; resume position; snapshot/record; sleep timer.**
- **Thumbnails at a timestamp** for the seek preview: a second, muted `libvlc_media_player` decoding
  to memory (`libvlc_video_set_callbacks`), rate-limited.

Everything keeps the engine-thread rule: commands are `post`ed, and state is mirrored on the UI thread.

### 6.4 Data (`data/`)

App data that isn't a preference lives in `%LOCALAPPDATA%\Sona\`:

- library folders and index, watched state and positions
- playlists, pins, recents, bookmarks
- subtitle associations, presets, profiles, saved streams

**Decided (D1):** SQLite (vendored amalgamation in `third_party/sqlite`, one `Database` class,
versioned migrations), built in P3 for bookmarks.

### 6.5 Library (`library/`)

- **Folder indexer** on a background thread:
  - scans the folders the user added, watches them for changes (`ReadDirectoryChangesW`)
  - reads metadata with `libvlc_media_parse_with_options` and stores it in `data/`
- **Queries** for each section and sort option.
- **The views** (compact sidebar and full library) are UI over these queries. They share one
  navigation model: a location stack with back, forward and breadcrumbs.

### 6.6 Settings (`settings/` → `preferences/` UI)

- **Settings schema:** extend `AppSettings` into a table where each setting declares:
  - key, type, default
  - category/section/advanced flag, label, info text
  - whether it maps to a libVLC option
- **The settings window** is generated from that table.
- **Search** filters by label and info text.
- **Profiles** are registry subkeys under `Software\Sona\Sona\Profiles\<name>`.

### 6.7 Subtitles (`subtitles/`)

- **Matching rules** in words (5.8): pair videos with subtitle files in a folder.
- **Stored associations**, applied when an item plays (per file, folder or playlist).
- **OpenSubtitles client:** REST, needs an API key (decision D2).
- **Cue parser** (SRT/ASS) for the subtitle timeline on the seek bar.

### 6.8 Streaming and conversion (`streaming/`)

- **Renderer discovery** (Chromecast) for "Render" and Stream.
- **`:sout`** chains for convert, stream and record.
- **Separate dialogs** (5.7).

### 6.9 Mapping to libVLC 3

| Feature | VLC reference (`AndroidStudioProjects/VLC`, `modules/gui/qt`) | libVLC 3 API |
| --- | --- | --- |
| Queue | `playlist/` | Our own `Queue` over `libvlc_media_player` (see 6.3) |
| Library | `medialibrary/` | Not in libVLC 3; index ourselves (`libvlc_media_parse_with_options`) |
| Subtitles | `dialogs/` | `libvlc_media_player_add_slave`, `libvlc_video_set_spu_delay`, `libvlc_video_set_spu` |
| Audio | `player/`, `dialogs/extended` | `libvlc_audio_equalizer_*`, `libvlc_audio_output_device_*`, `libvlc_audio_set_delay` |
| Video | `dialogs/extended` | `libvlc_video_set_adjust_*`, `libvlc_video_set_crop_geometry`, `libvlc_video_set_aspect_ratio` |
| Speed, chapters | `dialogs/bookmarks` | `libvlc_media_player_set_rate`, `libvlc_media_player_*_chapter*` (bookmarks need our own store) |
| Open dialogs | `dialogs/open` | `libvlc_media_new_location` (`dshow://`, `dvd://`, …) |
| Network discovery | `network/` | `libvlc_media_discoverer_*` |
| Stream, convert, record | `dialogs/sout` | `libvlc_renderer_discoverer_*`, `:sout=` media options |
| Media info, messages | `dialogs/mediainfo`, `dialogs/messages` | `libvlc_media_get_meta`, `libvlc_media_tracks_get`, `libvlc_log_set` |
| Extensions | `dialogs/extensions` | Not exposed by libVLC 3 |

Not libVLC features (our own work):

- seek-bar thumbnails (a second decoder)
- the subtitle timeline (parse the subtitle file)
- the waveform (decode the audio)

Reverse playback and the previous frame aren't possible with libVLC 3.

---

## 7. Phased plan

Each phase ends with something visible and checkable.

- **Verify by:** building debug and release with zero warnings, running the app, and comparing
  screenshots against the Figma PNG of the same frame, in both themes, at 100% and 125% scale.
- **Commit:** each phase to `dev` when it works.

| Phase | Goal | Steps | Done when |
| --- | --- | --- | --- |
| **P1 Foundation** | Toolkit and commands | 6.1 commands registry; `Column`, alignment and padding; `ScrollView`; overlay layer; popup window; `Tooltip`; frame clock; `Toggle`, `Dropdown`, `Segmented`, `Slider`, `Tabs`, `ListRow`, `Button`, `DialogFrame`; `TextField` | A test page shows every widget in both themes; keyboard shortcuts run through the registry. |
| **P2 Player chrome** | 5.1, 5.2, 5.6 | Auto-hide bars (windowed and fullscreen), Hide UI, main menu, right-click menu, duration toggle, volume bubble, on-screen shortcut messages, bars built from a layout definition | Matches `Player 247:445` and `3328:11554`; menus match `6062:13354` and `6065:13616`. |
| **P3 Queue** | 6.3 queue, 5.3 queue panel | Queue model, previous/next, add/drag files, reorder, repeat, shuffle, sort, remove; Bookmarks (`data/`) and Chapters tabs; Q and Ctrl+B | Plays a folder through; the panel matches the `Controls` queue, bookmarks and chapters variants. |
| **P4 Tracks** | 5.3 tracks popover | Track lists, external subtitles/audio, output devices, "Show Preferences" | One click switches subtitle and audio track and the output device. |
| **P5 Preferences panel** | 5.3 preferences | Four tabs with live libVLC effects, sliders, EQ and filter groups, presets saved per show | Changes apply live and survive restarts through presets. |
| **P6 Data and library** | 6.4, 6.5, 5.5 | Database; indexer; resume prompt (5.1); compact sidebar; full library (grid/list, sort, search, view); Home; Music categories; Network streams; Playlists; Recent; pins; miniplayer; context menus; watched/progress | Add a folder and browse it in both modes; play from the library into the queue without leaving the video. |
| **P7 Dialogs** | 5.7 | Open File/Folder/Network/Disc/Capture, New Playlist, Add Subtitles (computer, folder matching, OpenSubtitles), Keyboard Shortcuts editor, About | Each matches its frame; subtitles added to a folder apply to every episode. |
| **P8 Settings** | 6.6, 5.8 | Settings schema; generated window; search; advanced options; libVLC option mapping; Profiles | All 13 categories match their frames, basic and advanced; changing an option takes effect. |
| **P9 Media extras** | 5.4, 5.2 | Music view with blurred cover art; seek thumbnails; subtitle timeline; waveform; visualizer; sleep timer; statistics | Matches `Player - Music 4767:23726`; seeking shows previews. |
| **P10 Customize and streaming** | 5.10, 6.8 | Customize Player drag-and-drop editor; profiles with layouts; convert, stream, record, casting | A rearranged layout persists per profile; a file casts to a device. |
| **P11 Polish** | all | Light theme pass, accessibility (UI Automation names, keyboard focus), performance (startup < 300 ms, idle memory), installer associations | Both themes match Figma; Narrator reads the controls. |

Progress: P0 (foundation, playback, onboarding, packaging), P1 (toolkit and commands), P2 (player
chrome), P3 (queue), P4 (tracks) and P5 (preferences panel) are done; P6 has its library window and Home. Next: the rest of **P6**.

---

## 8. Open decisions

Ask the user before building anything that depends on these:

| # | Decision | Options | Recommendation |
| --- | --- | --- | --- |
| D1 | App data store | **Decided:** SQLite (vendored), built in P3 | — |
| D2 | OpenSubtitles | Official REST API (needs a key and user login) · leave out | Ask the user for an API key |
| D3 | Keyboard map conflicts (Ctrl+C capture vs copy, Ctrl+F folder vs find, B bookmarks vs audio cycle) | **Decided:** the menus' keys everywhere: Previous/Next Item P / N (the guide shows them), Record Shift+R, Ctrl+F Open Folder and Ctrl+E library search; a focused text field keeps Ctrl+C/V/X/A/Z, so Ctrl+C is Capture Device elsewhere; B cycles audio and Ctrl+B adds a bookmark | — |
| D4 | Features libVLC 3 can't do (reverse playback, previous frame) | Hide · show disabled with a reason | Show disabled with a reason |
| D5 | Fullscreen button position | Title bar (as built and as Figma) · control bar | Keep it in the title bar; customize can move it |
| D6 | libVLC 4 | Stay on 3.0.x · move when stable | Stay on 3; keep everything behind `playback/` |

---

## 9. Working agreement for agents

- **Read first:** this file, `docs/ARCHITECTURE.md`, then the Figma PNGs for the screen at hand.
  Take numbers from `design/figma.json`.
- **Branches:** work on `dev`. Merge to `main` only when asked.
- **Build and verify:**
  ```powershell
  cmake --build --preset debug          # run .\build\msvc\src\Debug\sona.exe
  cmake --build --preset release
  cmake --build --preset release --target installer
  .\build\msvc\Sona-0.1.0-setup.exe /VERYSILENT /SUPPRESSMSGBOXES /NORESTART /CURRENTUSER
  ```
  Close Sona first (`taskkill /IM sona.exe /F`). Debug and installed are separate copies; rebuild the
  one being tested.
- **New source files** go in `src/CMakeLists.txt`; assets under `src/assets/` are embedded
  automatically.
- **Keep the existing patterns:**
  - no UI framework
  - views paint from shared state
  - libVLC calls only through `Player`'s engine thread
  - one place per rule or value
  - Figma numbers in DIPs
- **Scope:**
  - one phase or screen at a time
  - match the frame before adding behaviour beyond it
  - don't build speculative options
- **Mark progress here:** when a phase or surface is done, update [section 3](#3-where-the-project-is)
  and the progress line in [section 7](#7-phased-plan).

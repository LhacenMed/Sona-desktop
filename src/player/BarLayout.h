#pragma once

#include <vector>

// Everything the player's bars can hold. The bars are built from a BarLayout of these, so Customize
// Player (P10) only edits the layout.
enum class BarItem {
    // Title bar (Figma "topbar" 247:447).
    Logo,
    MainMenu,
    FileBrowser,
    Divider,
    FolderName,
    FullScreen,
    // Control bar (Figma "Controls" 491:1779).
    Previous,
    PlayPause,
    Next,
    CurrentTime,
    SeekBar,
    Duration,
    Volume,
    AudioTracks,
    Subtitles,
    PlaybackSettings,
    Queue,
};

// Items laid out together, `spacing` apart. A group with an item that fills (the seek bar, the
// folder name) fills its bar's free width.
struct BarGroup
{
    float spacing;
    std::vector<BarItem> items;
};

// What the bars hold: the title bar's groups before and after its centred title (Windows' buttons
// always end it), and the control bar's groups.
struct BarLayout
{
    std::vector<BarGroup> titleLeading;
    std::vector<BarGroup> titleTrailing;
    std::vector<BarGroup> controls;
};

inline const BarLayout defaultBarLayout{
    .titleLeading = {{0, {BarItem::Logo, BarItem::MainMenu, BarItem::FileBrowser, BarItem::Divider, BarItem::FolderName}}},
    .titleTrailing = {{0, {BarItem::FullScreen}}},
    .controls = {
        {8, {BarItem::Previous, BarItem::PlayPause, BarItem::Next}},
        {8, {BarItem::CurrentTime, BarItem::SeekBar, BarItem::Duration}},
        {8, {BarItem::Volume}},
        {12, {BarItem::AudioTracks, BarItem::Subtitles, BarItem::PlaybackSettings}},
        {0, {BarItem::Queue}},
    },
};

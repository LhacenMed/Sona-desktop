#pragma once

#include <functional>
#include <string>
#include <vector>

#include "platform/KeyChord.h"

// Every user action. Keyboard handling, menus, the bars' buttons and the shortcut guide refer to
// actions by these ids and read the rest from the CommandRegistry.
enum class CommandId {
    PlayPause,
    Stop,
    SeekBackward,
    SeekForward,
    PreviousItem,
    NextItem,
    PreviousFrame,
    NextFrame,
    CycleSubtitleTrack,
    CycleAudioTrack,
    ToggleFullScreen,
    ExitFullScreen,
    ToggleAutoHide,
    VolumeUp,
    VolumeDown,
    ToggleMute,
    ShowAudioTracks,
    ShowSubtitleTracks,
    ShowPlaybackSettings,
    ToggleQueue,
    AddBookmark,
    ToggleFileBrowser,
    TakeSnapshot,
    Record,
    OpenFile,
    OpenFolder,
    OpenNetworkStream,
    OpenDisc,
    OpenCaptureDevice,
    Convert,
    KeyboardShortcuts,
    CustomizePlayer,
    Settings,
    About,
    Quit,
};

// A user action, declared once: its name, icon, default shortcuts and what it does.
struct Command
{
    std::wstring label;
    // The name it goes by while `active`, if it changes (Pause while playing, Show UI while hidden).
    std::wstring activeLabel;
    // A two-tone icon (assets/icons/<icon>-duo.svg), for menus; the bars' buttons use its single-
    // colour version (<icon>.svg). Empty for none.
    std::wstring icon;
    std::vector<KeyChord> shortcuts;
    // Empty until the feature behind the command exists: it's listed (greyed out in menus) but does
    // nothing.
    std::function<void()> run;
    // Whether it can run right now; empty means always.
    std::function<bool()> enabled;
    // Whether the state its active label names holds now.
    std::function<bool()> active;
};

#pragma once

#include <functional>
#include <vector>

#include "platform/KeyChord.h"

// Every user action. Keyboard handling and the shortcut guide refer to actions by these ids and
// read the rest from the CommandRegistry.
enum class CommandId {
    PlayPause,
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
    VolumeUp,
    VolumeDown,
    ToggleQueue,
    ToggleFileBrowser,
    TakeSnapshot,
    Record,
    OpenFile,
};

// A user action, declared once: its default shortcuts and what it does.
struct Command
{
    std::vector<KeyChord> shortcuts;
    // Empty until the feature behind the command exists; it's listed (in the guide) but does nothing.
    std::function<void()> run;
    // Whether it can run right now; empty means always.
    std::function<bool()> enabled;
};

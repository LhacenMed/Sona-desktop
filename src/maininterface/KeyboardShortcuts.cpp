#include "maininterface/KeyboardShortcuts.h"

#include "playback/Player.h"

std::vector<ShortcutGroup> makeKeyboardShortcuts(Player &player, std::function<void()> toggleFullScreen)
{
    return {
        {L"Play / Pause", {{{L"Space"}, KeyChord{VK_SPACE}, [&player] { player.togglePause(); }}}},
        {L"Seek Backward / Forward",
         {{{L"←"}, KeyChord{VK_LEFT}, [&player] { player.seekBy(-10); }},
          {{L"→"}, KeyChord{VK_RIGHT}, [&player] { player.seekBy(10); }}}},
        {L"Previous / Next Item", {{{L"<"}}, {{L">"}}}},
        // libVLC 3 steps frames forward only.
        {L"Previous / Next Frame", {{{L","}}, {{L"."}, KeyChord{VK_OEM_PERIOD}, [&player] { player.nextFrame(); }}}},
        {L"Cycle Subtitle Track", {{{L"V"}, KeyChord{'V'}, [&player] { player.cycleSubtitleTrack(); }}}},
        {L"Cycle Audio Track", {{{L"B"}, KeyChord{'B'}, [&player] { player.cycleAudioTrack(); }}}},
        {L"Toggle Fullscreen", {{{L"F"}, KeyChord{'F'}, toggleFullScreen}}},
        {L"Increase / Decrease Volume",
         {{{L"↑"}, KeyChord{VK_UP}, [&player] { player.setVolume(player.volume() + 5); }},
          {{L"↓"}, KeyChord{VK_DOWN}, [&player] { player.setVolume(player.volume() - 5); }}}},
        {L"Show / Hide Queue", {{{L"Q"}}}},
        {L"Show / Hide File Browser", {{{L"L"}}}},
        {L"Take a Snapshot", {{{L"SHIFT", L"S"}, KeyChord{'S', true}, [&player] { player.takeSnapshot(); }}}},
        {L"Record", {{{L"SHIFT", L"R"}}}},
    };
}

bool runShortcut(const std::vector<ShortcutGroup> &groups, KeyChord chord)
{
    for (const ShortcutGroup &group : groups) {
        for (const ShortcutBinding &binding : group.bindings) {
            if (binding.chord == chord && binding.run) {
                binding.run();
                return true;
            }
        }
    }
    return false;
}

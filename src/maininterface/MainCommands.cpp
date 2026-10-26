#include "maininterface/MainCommands.h"

#include "commands/CommandRegistry.h"
#include "maininterface/MainWindow.h"
#include "playback/Player.h"

void defineMainCommands(CommandRegistry &commands, MainWindow &window, Player &player)
{
    commands.define(CommandId::PlayPause, {.label = L"Play",
                                           .activeLabel = L"Pause",
                                           .icon = L"play",
                                           .shortcuts = {{VK_SPACE}},
                                           .run = [&player] { player.togglePause(); },
                                           .active = [&player] { return player.isPlaying(); }});
    commands.define(CommandId::Stop,
                    {.label = L"Stop", .icon = L"stop", .shortcuts = {{'S'}}, .run = [&player] { player.stop([] {}); }});
    commands.define(CommandId::SeekBackward,
                    {.label = L"Seek Backward", .shortcuts = {{VK_LEFT}}, .run = [&player] { player.seekBy(-10); }});
    commands.define(CommandId::SeekForward,
                    {.label = L"Seek Forward", .shortcuts = {{VK_RIGHT}}, .run = [&player] { player.seekBy(10); }});
    commands.define(CommandId::PreviousItem, {.label = L"Previous Item", .icon = L"previous", .shortcuts = {{'P'}}});
    commands.define(CommandId::NextItem, {.label = L"Next Item", .icon = L"next", .shortcuts = {{'N'}}});
    // libVLC 3 steps frames forward only.
    commands.define(CommandId::PreviousFrame, {.label = L"Previous Frame", .shortcuts = {{VK_OEM_COMMA}}});
    commands.define(CommandId::NextFrame,
                    {.label = L"Next Frame", .shortcuts = {{VK_OEM_PERIOD}}, .run = [&player] { player.nextFrame(); }});
    commands.define(CommandId::CycleSubtitleTrack, {.label = L"Cycle Subtitle Track",
                                                    .shortcuts = {{'V'}},
                                                    .run = [&player] { player.cycleSubtitleTrack(); }});
    commands.define(CommandId::CycleAudioTrack,
                    {.label = L"Cycle Audio Track", .shortcuts = {{'B'}}, .run = [&player] { player.cycleAudioTrack(); }});
    commands.define(CommandId::ToggleFullScreen, {.label = L"Full Screen",
                                                  .activeLabel = L"Exit Full Screen",
                                                  .shortcuts = {{'F'}},
                                                  .run = [&window] { window.toggleFullScreen(); },
                                                  .active = [&window] { return window.isFullScreen(); }});
    commands.define(CommandId::ExitFullScreen, {.label = L"Exit Full Screen",
                                                .shortcuts = {{VK_ESCAPE}},
                                                .run = [&window] { window.toggleFullScreen(); },
                                                .enabled = [&window] { return window.isFullScreen(); }});
    // Hide UI turns the bars' auto-hide on; Show UI fixes them in place.
    commands.define(CommandId::ToggleAutoHide, {.label = L"Hide UI",
                                                .activeLabel = L"Show UI",
                                                .icon = L"ui",
                                                .shortcuts = {{.key = VK_OEM_5, .control = true}},
                                                .run = [&window] { window.setBarsAutoHide(!window.barsAutoHide()); },
                                                .active = [&window] { return window.barsAutoHide(); }});
    commands.define(CommandId::VolumeUp, {.label = L"Increase Volume",
                                          .shortcuts = {{VK_UP}},
                                          .run = [&player] { player.setVolume(player.volume() + 5); }});
    commands.define(CommandId::VolumeDown, {.label = L"Decrease Volume",
                                            .shortcuts = {{VK_DOWN}},
                                            .run = [&player] { player.setVolume(player.volume() - 5); }});
    commands.define(CommandId::ToggleMute, {.label = L"Mute",
                                            .activeLabel = L"Unmute",
                                            .run = [&player] { player.toggleMute(); },
                                            .active = [&player] { return player.isMuted(); }});
    commands.define(CommandId::ShowAudioTracks, {.label = L"Audio Tracks", .icon = L"audio"});
    commands.define(CommandId::ShowSubtitleTracks, {.label = L"Subtitle Tracks", .icon = L"subtitles"});
    commands.define(CommandId::ShowPlaybackSettings, {.label = L"Playback Settings", .icon = L"playback-settings"});
    commands.define(CommandId::ToggleQueue, {.label = L"Queue", .icon = L"lists", .shortcuts = {{'Q'}}});
    commands.define(CommandId::ToggleFileBrowser, {.label = L"File Browser", .shortcuts = {{'L'}}});
    commands.define(CommandId::TakeSnapshot, {.label = L"Take a Snapshot",
                                              .shortcuts = {{.key = 'S', .shift = true}},
                                              .run = [&player] { player.takeSnapshot(); }});
    commands.define(CommandId::Record, {.label = L"Record", .icon = L"record", .shortcuts = {{.key = 'R', .shift = true}}});
    commands.define(CommandId::OpenFile, {.label = L"Open File(s)",
                                          .icon = L"file",
                                          .shortcuts = {{.key = 'O', .control = true}},
                                          .run = [&window] { window.openFile(); }});
    commands.define(CommandId::OpenFolder,
                    {.label = L"Open Folder", .icon = L"folder", .shortcuts = {{.key = 'F', .control = true}}});
    commands.define(CommandId::OpenNetworkStream,
                    {.label = L"Open Network Stream", .icon = L"streaming", .shortcuts = {{.key = 'N', .control = true}}});
    commands.define(CommandId::OpenDisc, {.label = L"Open Disc", .icon = L"disc", .shortcuts = {{.key = 'D', .control = true}}});
    commands.define(CommandId::OpenCaptureDevice,
                    {.label = L"Open Capture Device", .icon = L"capture-device", .shortcuts = {{.key = 'C', .control = true}}});
    commands.define(CommandId::Convert, {.label = L"Convert", .icon = L"swap", .shortcuts = {{.key = 'R', .control = true}}});
    commands.define(CommandId::KeyboardShortcuts, {.label = L"Keyboard Shortcuts", .icon = L"hotkeys"});
    commands.define(CommandId::CustomizePlayer, {.label = L"Customize Player", .icon = L"paint"});
    commands.define(CommandId::Settings, {.label = L"Settings", .icon = L"settings", .shortcuts = {{.key = 'P', .control = true}}});
    commands.define(CommandId::About,
                    {.label = L"About " SONA_APP_NAME, .icon = L"info", .shortcuts = {{.key = VK_F1, .shift = true}}});
    commands.define(CommandId::Quit, {.label = L"Quit " SONA_APP_NAME,
                                      .icon = L"quit",
                                      .shortcuts = {{.key = 'Q', .control = true}},
                                      .run = [&window] { window.close(); }});
}

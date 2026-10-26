#include "maininterface/MainCommands.h"

#include "commands/CommandRegistry.h"
#include "maininterface/MainWindow.h"
#include "playback/Player.h"

void defineMainCommands(CommandRegistry &commands, MainWindow &window, Player &player)
{
    commands.define(CommandId::PlayPause, {{{VK_SPACE}}, [&player] { player.togglePause(); }});
    commands.define(CommandId::SeekBackward, {{{VK_LEFT}}, [&player] { player.seekBy(-10); }});
    commands.define(CommandId::SeekForward, {{{VK_RIGHT}}, [&player] { player.seekBy(10); }});
    commands.define(CommandId::PreviousItem, {{{'P'}}});
    commands.define(CommandId::NextItem, {{{'N'}}});
    // libVLC 3 steps frames forward only.
    commands.define(CommandId::PreviousFrame, {{{VK_OEM_COMMA}}});
    commands.define(CommandId::NextFrame, {{{VK_OEM_PERIOD}}, [&player] { player.nextFrame(); }});
    commands.define(CommandId::CycleSubtitleTrack, {{{'V'}}, [&player] { player.cycleSubtitleTrack(); }});
    commands.define(CommandId::CycleAudioTrack, {{{'B'}}, [&player] { player.cycleAudioTrack(); }});
    commands.define(CommandId::ToggleFullScreen, {{{'F'}}, [&window] { window.toggleFullScreen(); }});
    commands.define(CommandId::ExitFullScreen, {{{VK_ESCAPE}}, [&window] { window.toggleFullScreen(); },
                                                [&window] { return window.isFullScreen(); }});
    commands.define(CommandId::VolumeUp, {{{VK_UP}}, [&player] { player.setVolume(player.volume() + 5); }});
    commands.define(CommandId::VolumeDown, {{{VK_DOWN}}, [&player] { player.setVolume(player.volume() - 5); }});
    commands.define(CommandId::ToggleQueue, {{{'Q'}}});
    commands.define(CommandId::ToggleFileBrowser, {{{'L'}}});
    commands.define(CommandId::TakeSnapshot, {{{.key = 'S', .shift = true}}, [&player] { player.takeSnapshot(); }});
    commands.define(CommandId::Record, {{{.key = 'R', .shift = true}}});
    commands.define(CommandId::OpenFile, {{{.key = 'O', .control = true}}, [&window] { window.openFile(); }});
}

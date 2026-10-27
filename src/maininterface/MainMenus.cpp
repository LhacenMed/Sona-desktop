#include "maininterface/MainMenus.h"

namespace {

std::vector<MenuItem> openMediaItems()
{
    return {
        {.command = CommandId::OpenFile},
        {.command = CommandId::OpenFolder},
        {.command = CommandId::OpenNetworkStream},
        {.command = CommandId::OpenDisc},
        {.command = CommandId::OpenCaptureDevice},
    };
}

} // namespace

std::vector<MenuItem> mainMenuItems()
{
    std::vector<MenuItem> items = openMediaItems();
    items.insert(items.end(), {
        {.command = CommandId::Convert},
        // Recent items come with the library (P6), profiles with Customize Player (P10).
        {.label = L"Open Recent", .icon = L"recent"},
        {.label = L"Profile", .icon = L"profile"},
        {.command = CommandId::KeyboardShortcuts},
        {.command = CommandId::CustomizePlayer},
        {.command = CommandId::Settings},
        {.command = CommandId::About},
    });
    return items;
}

std::vector<MenuItem> contextMenuItems()
{
    return {
        {.command = CommandId::PlayPause},
        {.command = CommandId::Stop},
        {.command = CommandId::PreviousItem},
        {.command = CommandId::NextItem},
        {.command = CommandId::Record},
        {.command = CommandId::ToggleAutoHide},
        {.label = L"Playback",
         .icon = L"playback",
         .submenu = {{.command = CommandId::SeekBackward},
                     {.command = CommandId::SeekForward},
                     {.command = CommandId::PreviousFrame},
                     {.command = CommandId::NextFrame}}},
        {.label = L"Video",
         .icon = L"video",
         .submenu = {{.command = CommandId::ToggleFullScreen}, {.command = CommandId::TakeSnapshot}}},
        {.label = L"Audio",
         .icon = L"audio",
         .submenu = {{.command = CommandId::ShowAudioTracks},
                     {.command = CommandId::CycleAudioTrack},
                     {.command = CommandId::ToggleMute},
                     {.command = CommandId::VolumeUp},
                     {.command = CommandId::VolumeDown}}},
        {.label = L"Subtitle",
         .icon = L"subtitles",
         .submenu = {{.command = CommandId::ShowSubtitleTracks}, {.command = CommandId::CycleSubtitleTrack}}},
        {.label = L"More Settings", .icon = L"settings", .submenu = {{.command = CommandId::Settings}}},
        {.label = L"Playlist", .icon = L"playlist", .submenu = {{.command = CommandId::ToggleQueue}}},
        {.label = L"Open Media", .icon = L"file", .submenu = openMediaItems()},
        {.command = CommandId::Quit},
    };
}

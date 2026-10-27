#include <windows.h>
#include <objbase.h>
#include <shellapi.h>
#include <ShObjIdl.h>

#include <exception>
#include <string>

#include "data/Bookmarks.h"
#include "data/Database.h"
#include "data/History.h"
#include "data/Presets.h"
#include "graphics/GraphicsDevice.h"
#include "library/HistoryRecorder.h"
#include "library/Thumbnails.h"
#include "maininterface/MainWindow.h"
#include "playback/ActivePreset.h"
#include "playback/Player.h"
#include "playback/Queue.h"
#include "settings/AppSettings.h"
#include "style/Theme.h"

#ifdef SONA_GALLERY
#include "gallery/GalleryWindow.h"
#endif

namespace {

// The first command-line argument: media ("Open with", a double-clicked file), or a flag.
std::wstring launchArgument()
{
    int count = 0;
    LPWSTR *arguments = CommandLineToArgvW(GetCommandLineW(), &count);
    std::wstring argument = count > 1 ? arguments[1] : L"";
    LocalFree(arguments);
    return argument;
}

void runMessageLoop()
{
    MSG message;
    while (GetMessageW(&message, nullptr, 0, 0)) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
}

// The player, playing `media` (a path, a folder or a URL) if there is one.
void runPlayer(const std::wstring &media)
{
    // Created first, so libVLC loads while the window is being set up.
    Player player;
    SettingsStore settings;
    Theme theme(settings);
    Database database;
    Bookmarks bookmarks(database);
    Queue queue(player);
    History history(database);
    const HistoryRecorder historyRecorder(player, history);
    Presets presets(database);
    ActivePreset preset(player, presets);
    Thumbnails thumbnails;
    // Bookmarks are the playing item's.
    const Subscription playingBookmarks = queue.changed.connect([&queue, &bookmarks] {
        const QueueItem *playing = queue.currentItem();
        bookmarks.setMedia(playing ? playing->location : std::wstring());
    });
    GraphicsDevice graphics;
    MainWindow window(graphics, theme, settings, player, queue, bookmarks, history, thumbnails, preset);

    // Launching with media opens a player, maximized. An empty launch opens the library at the
    // default size.
    if (!media.empty())
        window.openMedia({media});
    window.show(!media.empty());
    if (!settings.get(AppSettings::onboardingCompleted))
        window.showOnboarding();
    runMessageLoop();
}

#ifdef SONA_GALLERY
void runGallery()
{
    SettingsStore settings;
    Theme theme(settings);
    GraphicsDevice graphics;
    GalleryWindow gallery(graphics, theme, settings);
    runMessageLoop();
}
#endif

} // namespace

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    // Groups the window, taskbar pins and installer shortcuts under one taskbar identity.
    SetCurrentProcessExplicitAppUserModelID(SONA_APP_USER_MODEL_ID);
    // The file dialogs are COM objects that expect a single-threaded apartment.
    if (FAILED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE)))
        return 1;

    int exitCode = 0;
    try {
        const std::wstring argument = launchArgument();
#ifdef SONA_GALLERY
        if (argument == L"--gallery")
            runGallery();
        else
#endif
            runPlayer(argument);
    } catch (const std::exception &error) {
        MessageBoxA(nullptr, error.what(), nullptr, MB_ICONERROR);
        exitCode = 1;
    }
    CoUninitialize();
    return exitCode;
}

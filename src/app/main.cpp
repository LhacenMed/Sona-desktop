#include <windows.h>
#include <objbase.h>
#include <shellapi.h>
#include <ShObjIdl.h>

#include <exception>
#include <filesystem>
#include <string>

#include "graphics/GraphicsDevice.h"
#include "maininterface/MainWindow.h"
#include "playback/Player.h"
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

// The player, playing `media` (a path or a URL) if there is one.
void runPlayer(std::wstring media)
{
    if (!media.empty() && media.find(L"://") == std::wstring::npos)
        media = std::filesystem::absolute(media).wstring();

    // Created first, so libVLC loads while the window is being set up.
    Player player;
    SettingsStore settings;
    Theme theme(settings);
    GraphicsDevice graphics;
    MainWindow window(graphics, theme, settings, player);

    // Launching with media opens maximized; an empty launch opens at the default size.
    window.show(!media.empty());
    if (!media.empty())
        player.open(media);
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

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

namespace {

// The media passed on the command line ("Open with", a double-clicked file), as a full path or a URL.
std::wstring launchMedia()
{
    int count = 0;
    LPWSTR *arguments = CommandLineToArgvW(GetCommandLineW(), &count);
    std::wstring media = count > 1 ? arguments[1] : L"";
    LocalFree(arguments);
    if (!media.empty() && media.find(L"://") == std::wstring::npos)
        media = std::filesystem::absolute(media).wstring();
    return media;
}

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
        // Created first, so libVLC loads while the window is being set up.
        Player player;
        SettingsStore settings;
        Theme theme(settings);
        GraphicsDevice graphics;
        MainWindow window(graphics, theme, settings, player);

        const std::wstring media = launchMedia();
        // Launching with media opens maximized; an empty launch opens at the default size.
        window.show(!media.empty());
        if (!media.empty())
            player.open(media);
        if (!settings.get(AppSettings::onboardingCompleted))
            window.showOnboarding();

        MSG message;
        while (GetMessageW(&message, nullptr, 0, 0)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    } catch (const std::exception &error) {
        MessageBoxA(nullptr, error.what(), nullptr, MB_ICONERROR);
        exitCode = 1;
    }
    CoUninitialize();
    return exitCode;
}

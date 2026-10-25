#include "platform/SystemTheme.h"

#include <windows.h>

bool systemPrefersLightTheme()
{
    DWORD light = 0;
    DWORD size = sizeof(light);
    RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                 L"AppsUseLightTheme", RRF_RT_REG_DWORD, nullptr, &light, &size);
    return light != 0;
}

#include "settings/SettingsStore.h"

#include <windows.h>

namespace {

constexpr wchar_t settingsKey[] = L"Software\\" SONA_PUBLISHER L"\\" SONA_APP_NAME;

} // namespace

std::optional<std::uint32_t> SettingsStore::readNumber(const wchar_t *key)
{
    DWORD value = 0;
    DWORD size = sizeof(value);
    if (RegGetValueW(HKEY_CURRENT_USER, settingsKey, key, RRF_RT_REG_DWORD, nullptr, &value, &size) != ERROR_SUCCESS)
        return {};
    return value;
}

std::optional<std::wstring> SettingsStore::readString(const wchar_t *key)
{
    DWORD size = 0;
    if (RegGetValueW(HKEY_CURRENT_USER, settingsKey, key, RRF_RT_REG_SZ, nullptr, nullptr, &size) != ERROR_SUCCESS)
        return {};
    std::wstring value(size / sizeof(wchar_t), L'\0');
    if (RegGetValueW(HKEY_CURRENT_USER, settingsKey, key, RRF_RT_REG_SZ, nullptr, value.data(), &size) != ERROR_SUCCESS)
        return {};
    // The size includes the terminating null.
    value.resize(size / sizeof(wchar_t) - 1);
    return value;
}

void SettingsStore::writeNumber(const wchar_t *key, std::uint32_t value)
{
    const DWORD data = value;
    RegSetKeyValueW(HKEY_CURRENT_USER, settingsKey, key, REG_DWORD, &data, sizeof(data));
}

void SettingsStore::writeString(const wchar_t *key, const std::wstring &value)
{
    RegSetKeyValueW(HKEY_CURRENT_USER, settingsKey, key, REG_SZ, value.c_str(),
                    static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t)));
}

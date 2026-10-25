#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <type_traits>

#include "core/Signal.h"

// A persisted preference: its registry value name and the value used until the user changes it.
// Every one is declared in AppSettings.h.
template <typename T>
struct Setting
{
    const wchar_t *key;
    T fallback;
};

// Reads and writes preferences under HKCU\Software\<publisher>\<app name>, where the uninstaller
// cleans up. Values are read straight from the registry, so it is their only copy. Numbers, flags
// and enums are stored as DWORDs, text as strings.
class SettingsStore
{
public:
    template <typename T>
    T get(const Setting<T> &setting) const
    {
        if constexpr (std::is_same_v<T, std::wstring>) {
            return readString(setting.key).value_or(setting.fallback);
        } else {
            const std::optional<std::uint32_t> value = readNumber(setting.key);
            return value ? static_cast<T>(*value) : setting.fallback;
        }
    }

    template <typename T>
    void set(const Setting<T> &setting, const T &value)
    {
        if constexpr (std::is_same_v<T, std::wstring>)
            writeString(setting.key, value);
        else
            writeNumber(setting.key, static_cast<std::uint32_t>(value));
        changed.emit();
    }

    Signal<> changed;

private:
    static std::optional<std::uint32_t> readNumber(const wchar_t *key);
    static std::optional<std::wstring> readString(const wchar_t *key);
    static void writeNumber(const wchar_t *key, std::uint32_t value);
    static void writeString(const wchar_t *key, const std::wstring &value);
};

#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "settings/PlaybackPreferences.h"

class Database;

// The preferences panel's presets (Figma "Setting Preset" 2372:8834): its values under a name, and
// the preset each folder plays with, so a show's episodes all play with the one chosen for it.
class Presets
{
public:
    // The preset everything plays with until another is chosen.
    static constexpr std::wstring_view defaultName = L"Default Preset";

    explicit Presets(Database &database) : m_database(database) {}

    // The default first, then the others in the order they were made.
    std::vector<std::wstring> names() const;
    // A preset's values; a preset never saved has every preference at its fallback.
    PreferenceValues values(const std::wstring &name) const;
    // Saves `values` as `name`'s, making the preset if it's new.
    void save(const std::wstring &name, const PreferenceValues &values);
    // The preset last chosen for a folder, or the default.
    std::wstring presetFor(const std::wstring &folder) const;
    void setPresetFor(const std::wstring &folder, const std::wstring &name);

private:
    Database &m_database;
};

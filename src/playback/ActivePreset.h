#pragma once

#include <string>
#include <vector>

#include "core/Signal.h"
#include "platform/Timer.h"
#include "settings/PlaybackPreferences.h"

class Player;
class Presets;

// The preset in force (Figma "Setting Preset" 2372:8834): the one chosen for the playing media's
// folder, applied to the player as the media opens. The preferences panel shows and changes it; a
// change applies at once and is saved to the preset.
class ActivePreset
{
public:
    ActivePreset(Player &player, Presets &presets);
    ~ActivePreset();
    ActivePreset(const ActivePreset &) = delete;
    ActivePreset &operator=(const ActivePreset &) = delete;

    const std::wstring &name() const { return m_name; }
    const PreferenceValues &values() const { return m_values; }
    // Every preset, to choose from.
    std::vector<std::wstring> names() const;

    // Choosing an equalizer preset sets the bands to it.
    void set(Preference preference, double value);
    // Plays the folder with another preset.
    void choose(const std::wstring &name);
    // Saves the values as a new preset ("Custom Preset 01", then 02…) and chooses it.
    void createPreset();

    Signal<> changed;

private:
    void playerChanged();
    void apply();
    void save();

    Player &m_player;
    Presets &m_presets;
    // The playing media's folder; empty for a URL or nothing.
    std::wstring m_folder;
    std::wstring m_location;
    std::wstring m_name;
    PreferenceValues m_values;
    // Saves once changes settle, so dragging a slider isn't a write per step.
    Timer m_saveTimer;
    Subscription m_playerSubscription;
};

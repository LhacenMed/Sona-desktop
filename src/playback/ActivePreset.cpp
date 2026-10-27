#include "playback/ActivePreset.h"

#include <algorithm>
#include <filesystem>
#include <format>

#include "data/Presets.h"
#include "playback/MediaFiles.h"
#include "playback/Player.h"

namespace {

// How long changes rest before they're saved.
constexpr std::chrono::milliseconds saveDelay{500};

} // namespace

ActivePreset::ActivePreset(Player &player, Presets &presets)
    : m_player(player)
    , m_presets(presets)
    , m_name(Presets::defaultName)
    , m_values(presets.values(m_name))
    , m_saveTimer([this] { save(); })
{
    apply();
    m_playerSubscription = player.changed.connect([this] { playerChanged(); });
}

ActivePreset::~ActivePreset()
{
    if (m_saveTimer.isActive())
        save();
}

std::vector<std::wstring> ActivePreset::names() const
{
    return m_presets.names();
}

void ActivePreset::set(Preference preference, double value)
{
    m_values[preference] = value;
    if (preference == Preference::EqualizerPreset && value > 0) {
        constexpr Preference levels[] = {Preference::Preamp, Preference::Band60,  Preference::Band170, Preference::Band310,
                                         Preference::Band600, Preference::Band1k, Preference::Band3k,  Preference::Band6k,
                                         Preference::Band12k, Preference::Band14k, Preference::Band16k};
        const std::vector<double> preset = Player::equalizerPreset(static_cast<int>(value));
        for (size_t i = 0; i < std::min(preset.size(), std::size(levels)); ++i)
            m_values[levels[i]] = preset[i];
    }
    m_saveTimer.start(saveDelay, false);
    apply();
}

void ActivePreset::choose(const std::wstring &name)
{
    if (m_saveTimer.isActive())
        save();
    m_name = name;
    m_values = m_presets.values(name);
    if (!m_folder.empty())
        m_presets.setPresetFor(m_folder, name);
    apply();
}

void ActivePreset::createPreset()
{
    const std::vector<std::wstring> names = m_presets.names();
    std::wstring name;
    for (int number = 1; name.empty() || std::ranges::find(names, name) != names.end(); ++number)
        name = std::format(L"Custom Preset {:02}", number);
    m_presets.save(name, m_values);
    choose(name);
}

void ActivePreset::playerChanged()
{
    const std::wstring &location = m_player.location();
    if (location == m_location)
        return;
    m_location = location;
    // A show's episodes share a folder; a URL has none and plays with the default.
    m_folder = location.empty() || isUrl(location) ? std::wstring() : std::filesystem::path(location).parent_path().wstring();
    const std::wstring name = m_folder.empty() ? std::wstring(Presets::defaultName) : m_presets.presetFor(m_folder);
    if (name == m_name)
        return;
    if (m_saveTimer.isActive())
        save();
    m_name = name;
    m_values = m_presets.values(name);
    apply();
}

void ActivePreset::apply()
{
    m_player.applyPreferences(m_values);
    changed.emit();
}

void ActivePreset::save()
{
    m_saveTimer.stop();
    m_presets.save(m_name, m_values);
}

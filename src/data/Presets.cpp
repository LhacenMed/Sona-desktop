#include "data/Presets.h"

#include <cwchar>
#include <format>
#include <sstream>

#include "data/Database.h"

std::vector<std::wstring> Presets::names() const
{
    std::vector<std::wstring> names{std::wstring(defaultName)};
    auto query = m_database.prepare("SELECT name FROM presets WHERE name != ? ORDER BY rowid");
    query.bind(1, defaultName);
    while (query.step())
        names.push_back(query.text(0));
    return names;
}

PreferenceValues Presets::values(const std::wstring &name) const
{
    PreferenceValues values;
    auto query = m_database.prepare("SELECT preferences FROM presets WHERE name = ?");
    query.bind(1, name);
    if (!query.step())
        return values;
    // Keys it doesn't know (from a later version) are skipped; preferences it lacks keep their fallback.
    std::wistringstream lines(query.text(0));
    for (std::wstring line; std::getline(lines, line);) {
        const size_t equals = line.find(L'=');
        if (equals == std::wstring::npos)
            continue;
        const std::wstring key = line.substr(0, equals);
        for (size_t i = 0; i < preferenceCount; ++i) {
            const auto preference = static_cast<Preference>(i);
            const std::string_view known = preferenceInfo(preference).key;
            if (key == std::wstring(known.begin(), known.end()))
                values[preference] = std::wcstod(line.c_str() + equals + 1, nullptr);
        }
    }
    return values;
}

void Presets::save(const std::wstring &name, const PreferenceValues &values)
{
    std::wstring text;
    for (size_t i = 0; i < preferenceCount; ++i) {
        const auto preference = static_cast<Preference>(i);
        const std::string_view key = preferenceInfo(preference).key;
        text += std::wstring(key.begin(), key.end()) + std::format(L"={}\n", values[preference]);
    }
    m_database.prepare("INSERT OR REPLACE INTO presets (name, preferences) VALUES (?, ?)").bind(1, name).bind(2, text).step();
}

std::wstring Presets::presetFor(const std::wstring &folder) const
{
    auto query = m_database.prepare("SELECT preset FROM folder_presets WHERE folder = ?");
    query.bind(1, folder);
    return query.step() ? query.text(0) : std::wstring(defaultName);
}

void Presets::setPresetFor(const std::wstring &folder, const std::wstring &name)
{
    m_database.prepare("INSERT OR REPLACE INTO folder_presets (folder, preset) VALUES (?, ?)").bind(1, folder).bind(2, name).step();
}

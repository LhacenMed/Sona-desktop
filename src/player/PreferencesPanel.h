#pragma once

#include <array>

#include "core/Signal.h"
#include "widgets/Stack.h"

class Tabs;
struct BarContext;

// The preferences panel's tabs, in its order.
enum class PreferencesTab { Playback, Video, Audio, Subtitles };

// Figma "Controls" playback, video settings, audio settings and subtitle settings (2238:9432,
// 498:2937, 536:1059, 763:1958): the settings people change while watching, on four tabs, each a
// preference (PlaybackPreferences) of the preset in force. A change applies at once and is saved to
// the preset, which the footer chooses or makes; settings libVLC 3 can't change, or that later
// phases build, are drawn dimmed and say why.
class PreferencesPanel : public Column
{
public:
    static constexpr float width = 400;
    static constexpr float height = 946;

    PreferencesPanel(const BarContext &context, PreferencesTab tab);

    PreferencesTab tab() const;
    void showTab(PreferencesTab tab);

private:
    Tabs *m_tabs;
    // In PreferencesTab's order.
    std::array<View *, 4> m_pages;
    Subscription m_presetSubscription;
};

#pragma once

#include <array>

#include "widgets/Stack.h"

class Tabs;
enum class TrackKind;
struct BarContext;

// Figma "Controls" video, audio and subtitles (498:2674, 496:2495, 491:2216): tabs over the playing
// media's tracks of each kind, a click choosing one. Subtitles also lists the subtitle files added to
// the media and adds more; Audio, where the sound plays. "Show Preferences" opens the preferences
// panel.
class TracksPanel : public Column
{
public:
    static constexpr float width = 400;
    static constexpr float height = 524;

    TracksPanel(const BarContext &context, TrackKind tab);

    TrackKind tab() const;
    void showTab(TrackKind tab);

private:
    Tabs *m_tabs;
    // In TrackKind's order.
    std::array<View *, 3> m_pages;
};

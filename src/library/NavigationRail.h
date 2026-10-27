#pragma once

#include <functional>

#include "widgets/View.h"

// Figma Home's navigation (6261:15094): Home, Videos, Music, Network, Playlists, Recent and Add item
// on a raised panel. The library's place is Home; the other sections aren't built yet, so they're
// dimmed and take no clicks. Home goes back to Home from one of its sections.
class NavigationRail : public View
{
public:
    static constexpr float width = 217;

    explicit NavigationRail(std::function<void()> goHome);

    void layout() override;
    void paint(Canvas &canvas) override;
};

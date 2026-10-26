#pragma once

#include <algorithm>
#include <cmath>

#include "core/Geometry.h"
#include "style/Theme.h"

// A raised surface from Figma: its corner radius, fill and drop shadow. Every popup, menu, callout
// and panel takes its look from one of these, so each kind looks the same wherever it appears.
struct Surface
{
    float radius;
    Color Palette::*fill;
    Point shadowOffset;
    // Figma's blur radius; no shadow when the opacity is 0.
    float shadowBlur;
    float shadowOpacity;

    // How far the shadow reaches past the surface's edges, for windows that draw it themselves.
    float shadowReach() const { return std::ceil(1.5f * shadowBlur + std::max(std::abs(shadowOffset.x), std::abs(shadowOffset.y))); }
};

namespace Surfaces {

// Dropdown lists (2372:8307).
inline constexpr Surface list{6, &Palette::bgA4, {0, 4}, 13.8f, 0.35f};
// Menus (6062:13354, 6065:13616).
inline constexpr Surface menu{10, &Palette::bgA3Transparent, {0, 8}, 15, 0.46f};
// Callouts: tooltips, the volume value (2245:9805) and on-screen messages.
inline constexpr Surface callout{6, &Palette::bgB1Transparent, {0, 8}, 15, 0.46f};
// Control-bar panels (463:831), which float without a shadow.
inline constexpr Surface panel{10, &Palette::bgB1Transparent, {}, 0, 0};

} // namespace Surfaces

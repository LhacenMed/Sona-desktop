#pragma once

#include <algorithm>
#include <cmath>

#include "core/Geometry.h"
#include "style/Theme.h"

// A raised surface from Figma: its corner radius, fill, drop shadow and background blur. Every popup,
// menu, callout and panel takes its look from one of these, so each kind looks the same wherever it
// appears.
struct Surface
{
    float radius;
    // Over a blurred backdrop, the tint laid on it.
    Color Palette::*fill;
    Point shadowOffset;
    // Figma's blur radius; no shadow when the opacity is 0.
    float shadowBlur;
    float shadowOpacity;
    // A frosted surface shows what's behind it blurred by this much (the Gaussian's standard deviation,
    // as CSS's blur() takes it), under its fill (Canvas::fillSurface). 0 for a plain fill.
    float backdropBlur;

    // How far the shadow reaches past the surface's edges, for windows that draw it themselves.
    float shadowReach() const { return std::ceil(1.5f * shadowBlur + std::max(std::abs(shadowOffset.x), std::abs(shadowOffset.y))); }
};

namespace Surfaces {

// How frosted surfaces blur what's behind them: VS Code's frosted menus, backdrop-filter: blur(12px).
inline constexpr float frostedBlur = 12;

// Menus (6062:13354, 6065:13616).
inline constexpr Surface menu{10, &Palette::bgPopup, {0, 8}, 15, 0.46f, frostedBlur};
// Dropdown lists (2372:8307), frosted like the menus they open as.
inline constexpr Surface list{6, &Palette::bgPopup, {0, 4}, 13.8f, 0.35f, frostedBlur};
// Callouts: tooltips, the volume value (2245:9805) and on-screen messages.
inline constexpr Surface callout{6, &Palette::bgB1Transparent, {0, 8}, 15, 0.46f, 0};
// The seek bar's preview (2151:9673), in the window, over the video.
inline constexpr Surface thumbnail{14, &Palette::bgPopup, {}, 0, 0, 0};
// Control-bar panels (463:831), which float without a shadow.
inline constexpr Surface panel{10, &Palette::bgPanel, {}, 0, 0, frostedBlur};

} // namespace Surfaces

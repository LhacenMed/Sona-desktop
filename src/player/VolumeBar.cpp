#include "player/VolumeBar.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <format>

#include "graphics/Canvas.h"
#include "platform/Window.h"
#include "playback/Player.h"
#include "style/Theme.h"
#include "widgets/Overlay.h"

namespace {

// Each designed volume level and the fill width (px) its Figma variant reveals.
constexpr std::array<float, 13> levels{0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 150, 200};
constexpr std::array<float, 13> fillWidths{1, 7, 14, 20, 26, 33, 41, 48, 53, 58, 63, 75, 88};

// Piecewise-linear lookup between two parallel, ascending stop lists.
float interpolate(float value, const std::array<float, 13> &from, const std::array<float, 13> &to)
{
    for (size_t i = 1; i < from.size(); ++i) {
        if (value <= from[i])
            return to[i - 1] + (value - from[i - 1]) / (from[i] - from[i - 1]) * (to[i] - to[i - 1]);
    }
    return to.back();
}

} // namespace

VolumeBar::VolumeBar(Player &player)
    : m_player(player)
{
    setPreferredSize({88, 28});
}

void VolumeBar::paint(Canvas &canvas)
{
    canvas.drawSvg(L"images/volume-track.svg", {0, 0.32f, 89, 27.36f}, {{0x090909, canvas.palette().volumeTrack}});
    canvas.pushClip({0, 0, interpolate(static_cast<float>(m_player.volume()), levels, fillWidths), frame().height});
    canvas.drawSvg(L"images/volume-fill.svg", {1.5f, 1.72f, 86, 24.56f});
    canvas.popClip();
}

void VolumeBar::mousePressed(Point point)
{
    setVolumeAt(point.x);
}

void VolumeBar::mouseDragged(Point point)
{
    setVolumeAt(point.x);
}

void VolumeBar::mouseReleased(Point, bool)
{
    window()->overlay().hideValue();
}

void VolumeBar::setVolumeAt(float x)
{
    const float fill = std::clamp(x, fillWidths.front(), frame().width);
    m_player.setVolume(static_cast<int>(std::lround(interpolate(fill, fillWidths, levels))));
    window()->overlay().showValue(*this, std::format(L"{} %", m_player.volume()));
}

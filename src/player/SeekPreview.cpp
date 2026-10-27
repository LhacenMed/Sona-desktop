#include "player/SeekPreview.h"

#include <algorithm>

#include "core/TimeText.h"
#include "graphics/Canvas.h"
#include "playback/Player.h"
#include "style/Surface.h"
#include "style/Theme.h"

namespace {

// Figma: 272×189 with 10 of padding, a 252×142 picture with corners of 4, then the time.
constexpr Rect pictureBox{10, 10, 252, 142};
constexpr float pictureRadius = 4;
constexpr Rect timeBox{10, 162, 252, 17};
constexpr TextStyle timeStyle{.size = 15, .align = TextAlign::Center};

} // namespace

SeekPreview::SeekPreview(Player &player)
    : m_player(player)
{
    setPreferredSize({272, 189});
    m_previewSubscription = player.previewChanged.connect([this] { invalidate(); });
}

void SeekPreview::show(double seconds)
{
    m_seconds = seconds;
    invalidate();
}

void SeekPreview::paint(Canvas &canvas)
{
    const Palette &palette = canvas.palette();
    canvas.fillSurface(bounds(), Surfaces::thumbnail);
    canvas.fillRoundedRect(pictureBox, pictureRadius, palette.bgB2);
    // The picture fits the box, centred, whatever the video's shape.
    if (const Picture &frame = m_player.preview(); frame.width > 0) {
        const float scale = std::min(pictureBox.width / frame.width, pictureBox.height / frame.height);
        const Size size{frame.width * scale, frame.height * scale};
        canvas.pushRoundedClip(pictureBox, pictureRadius);
        canvas.drawPixels({pictureBox.x + (pictureBox.width - size.width) / 2, pictureBox.y + (pictureBox.height - size.height) / 2,
                           size.width, size.height},
                          frame.pixels.data(), frame.width, frame.height);
        canvas.popRoundedClip();
    }
    canvas.drawText(clockText(m_seconds), timeBox, timeStyle, palette.fgA2);
}

#include "onboarding/KeyCap.h"

#include "graphics/Canvas.h"
#include "graphics/GraphicsDevice.h"
#include "platform/Window.h"
#include "style/Theme.h"

namespace {

constexpr TextStyle labelStyle{.size = 16, .weight = DWRITE_FONT_WEIGHT_LIGHT, .align = TextAlign::Center};
constexpr float capHeight = 42;
constexpr float borderWidth = 3;

} // namespace

KeyCap::KeyCap(std::wstring label, bool compact)
    : m_label(std::move(label))
    , m_compact(compact)
{
}

Size KeyCap::preferredSize() const
{
    const Window *host = window();
    const float labelWidth = host ? host->graphics().measureText(m_label, labelStyle).width : 0;
    return {labelWidth + 2 * (m_compact ? 12 : 20) + 2 * borderWidth, capHeight};
}

void KeyCap::paint(Canvas &canvas)
{
    const Palette &palette = canvas.palette();
    const Rect cap{0, 0, frame().width, capHeight};
    canvas.fillRoundedRect({0, 6, cap.width, capHeight}, 12, Color::hex(0x5c5c5c, 0.25f));
    canvas.fillRoundedRect(cap, 12, palette.bgA3);
    // A 3px inside border.
    canvas.strokeRoundedRect({borderWidth / 2, borderWidth / 2, cap.width - borderWidth, capHeight - borderWidth},
                             12 - borderWidth / 2, borderWidth, palette.dividerBright);
    canvas.drawText(m_label, cap, labelStyle, palette.fg1);
}

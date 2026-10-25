#include "widgets/Label.h"

#include "graphics/Canvas.h"
#include "graphics/GraphicsDevice.h"
#include "platform/Window.h"

Label::Label(std::wstring text, TextStyle style, Color Palette::*color)
    : m_text(std::move(text))
    , m_style(style)
    , m_color(color)
{
}

void Label::setText(std::wstring text)
{
    if (text == m_text)
        return;
    m_text = std::move(text);
    m_elidedWidth = -1;
    invalidate();
}

Size Label::preferredSize() const
{
    const Size fixed = View::preferredSize();
    const Window *host = window();
    if ((fixed.width > 0 && fixed.height > 0) || !host)
        return fixed;
    const Size text = host->graphics().measureText(m_text, m_style);
    return {text.width + m_leftPadding + m_rightPadding, text.height};
}

void Label::paint(Canvas &canvas)
{
    const Rect box{m_leftPadding, 0, frame().width - m_leftPadding - m_rightPadding, frame().height};
    canvas.drawText(m_elidesMiddle ? visibleText(canvas, box.width) : m_text, box, m_style, canvas.palette().*m_color);
}

const std::wstring &Label::visibleText(const Canvas &canvas, float width) const
{
    if (width == m_elidedWidth)
        return m_elidedText;
    m_elidedWidth = width;

    // The most characters that fit, taken evenly from both ends around "…".
    const auto keep = [this](size_t count) {
        return m_text.substr(0, count - count / 2) + L"…" + m_text.substr(m_text.size() - count / 2);
    };
    if (canvas.measureText(m_text, m_style).width <= width) {
        m_elidedText = m_text;
        return m_elidedText;
    }
    size_t low = 0;
    size_t high = m_text.size();
    while (low < high) {
        const size_t middle = (low + high + 1) / 2;
        if (canvas.measureText(keep(middle), m_style).width <= width)
            low = middle;
        else
            high = middle - 1;
    }
    m_elidedText = keep(low);
    return m_elidedText;
}

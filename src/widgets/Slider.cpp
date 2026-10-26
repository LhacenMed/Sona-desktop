#include "widgets/Slider.h"

#include <algorithm>
#include <cmath>

#include "graphics/Canvas.h"
#include "style/Theme.h"

namespace {

// The thumb is as tall as the slider; the track is centred on it.
constexpr float thumbWidth = 8;
constexpr float thumbHeight = 24;
constexpr float trackHeight = 16;
constexpr float trackInset = 2;
constexpr float tickHeight = 6;
constexpr float labelGap = 10;
constexpr float labelHeight = 15;
constexpr std::chrono::milliseconds settleDuration{120};

} // namespace

Slider::Slider(float minimum, float maximum, float step, float value, std::vector<std::wstring> labels)
    : m_minimum(minimum)
    , m_maximum(maximum)
    , m_step(step)
    , m_value(value)
    , m_thumb(fractionOf(value))
    , m_labels(std::move(labels))
{
}

void Slider::setValue(float value)
{
    if (value == m_value)
        return;
    m_value = value;
    if (!isPressed())
        m_thumb.jumpTo(fractionOf(value));
    invalidate();
}

Size Slider::preferredSize() const
{
    return {View::preferredSize().width, m_labels.empty() ? thumbHeight : thumbHeight + labelGap + labelHeight};
}

float Slider::thumbX(float fraction) const
{
    return trackInset + fraction * (frame().width - 2 * trackInset - thumbWidth);
}

void Slider::paint(Canvas &canvas)
{
    const Palette &palette = canvas.palette();
    const float trackY = (thumbHeight - trackHeight) / 2;
    const float innerHeight = trackHeight - 2 * trackInset;
    const float thumb = thumbX(m_thumb.value(*this));
    canvas.fillRoundedRect({0, trackY, frame().width, trackHeight}, 6, palette.bgB2);
    canvas.fillRoundedRect({trackInset, trackY + trackInset, thumb + thumbWidth - trackInset, innerHeight}, 4,
                           palette.bgAccentBright);

    if (m_labels.size() > 1) {
        // Each label sits at its mark: the first from the start, the last up to the end, the others centred.
        const float count = static_cast<float>(m_labels.size() - 1);
        const float span = frame().width - 2 * trackInset;
        const TextStyle labelStyle{.size = 12};
        for (size_t i = 0; i < m_labels.size(); ++i) {
            const float x = trackInset + span * static_cast<float>(i) / count;
            const bool first = i == 0;
            const bool last = i == m_labels.size() - 1;
            if (!first && !last)
                canvas.fillRect({x - 1, (thumbHeight - tickHeight) / 2, 2, tickHeight}, palette.bgAccentDark);
            const float width = canvas.measureText(m_labels[i], labelStyle).width;
            const float left = first ? x : last ? x - width : x - width / 2;
            canvas.drawText(m_labels[i], {left, thumbHeight + labelGap, width, labelHeight}, labelStyle, palette.fgA3);
        }
    }
    canvas.fillRoundedRect({thumb, 0, thumbWidth, thumbHeight}, 4, palette.accentDim);
}

void Slider::mousePressed(Point point)
{
    setValueAt(point.x);
}

void Slider::mouseDragged(Point point)
{
    setValueAt(point.x);
}

void Slider::mouseReleased(Point, bool)
{
    m_thumb.animateTo(fractionOf(m_value), settleDuration);
    invalidate();
}

void Slider::setValueAt(float x)
{
    // The thumb follows the pointer by its centre; the value takes the nearest step.
    const float travel = frame().width - 2 * trackInset - thumbWidth;
    const float fraction = std::clamp((x - trackInset - thumbWidth / 2) / travel, 0.0f, 1.0f);
    m_thumb.jumpTo(fraction);
    invalidate();
    const float steps = std::round(fraction * (m_maximum - m_minimum) / m_step);
    const float value = std::min(m_maximum, m_minimum + steps * m_step);
    if (value == m_value)
        return;
    setValue(value);
    if (m_onChanged)
        m_onChanged(value);
}

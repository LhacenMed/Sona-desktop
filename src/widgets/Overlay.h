#pragma once

#include <string>
#include <vector>

#include "platform/Timer.h"
#include "widgets/View.h"

class Callout;

// A window's top layer, over its content: callouts (tooltips, values and on-screen messages, which
// fade in and out). Only what it shows takes the mouse; everywhere else reaches the
// content below.
class Overlay : public View
{
public:
    Overlay();
    ~Overlay() override;

    // Shows the hovered view's tooltip once the pointer rests on it.
    void hoverChanged(View *hovered);
    void hideTooltip();
    // A value centred above `anchor` while it changes, like the volume while the cone is dragged
    // (Figma "audio value" 2245:9805).
    void showValue(const View &anchor, std::wstring text);
    void hideValue();
    // A short message at the top of the window for a moment (what a shortcut just did).
    void showMessage(std::wstring text);

    void paintTree(Canvas &canvas) override;

private:
    // A view's bounds in this layer's coordinates.
    Rect boundsOf(const View &view) const;
    void showTooltip();
    // Fades a callout out and empties its slot; it's removed once it has faded.
    void dismiss(Callout *&callout);

    View *m_tooltipOwner = nullptr;
    Callout *m_tooltip = nullptr;
    Callout *m_value = nullptr;
    Callout *m_message = nullptr;
    std::vector<Callout *> m_fading;
    Timer m_tooltipTimer;
    Timer m_messageTimer;
};

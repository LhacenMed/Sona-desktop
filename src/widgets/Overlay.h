#pragma once

#include <string>
#include <utility>
#include <vector>

#include "platform/Timer.h"
#include "widgets/View.h"

class Callout;

// A window's top layer, over its content: popovers and callouts (tooltips, values and on-screen
// messages, which fade in and out). Only what it shows takes the mouse; everywhere else reaches the
// content below.
class Overlay : public View
{
public:
    Overlay();
    ~Overlay() override;

    // Opens a popover next to `anchor`: below it when it fits, otherwise above, aligned with its
    // leading edge and kept inside the window. It closes on Escape or a click outside it.
    template <typename T, typename... Args>
    T &openPopover(const View &anchor, Args &&...args)
    {
        T &popover = add<T>(std::forward<Args>(args)...);
        m_popovers.push_back(&popover);
        place(popover, anchor);
        return popover;
    }
    bool hasPopovers() const { return !m_popovers.empty(); }
    void closePopovers();
    // Closes the popovers unless `target`, a view being clicked, is in one; returns whether it did.
    bool closePopoversOutside(const View *target);

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
    void place(View &popover, const View &anchor);
    void showTooltip();
    // Fades a callout out and empties its slot; it's removed once it has faded.
    void dismiss(Callout *&callout);

    std::vector<View *> m_popovers;
    View *m_tooltipOwner = nullptr;
    Callout *m_tooltip = nullptr;
    Callout *m_value = nullptr;
    Callout *m_message = nullptr;
    std::vector<Callout *> m_fading;
    Timer m_tooltipTimer;
    Timer m_messageTimer;
};

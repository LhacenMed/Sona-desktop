#pragma once

#include <utility>
#include <vector>

#include "platform/Timer.h"
#include "widgets/View.h"

// A window's top layer, over its content: popovers and tooltips. Only what it shows takes the mouse;
// everywhere else reaches the content below.
class Overlay : public View
{
public:
    Overlay();

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

private:
    // A view's bounds in this layer's coordinates.
    Rect boundsOf(const View &view) const;
    void place(View &popover, const View &anchor);
    void showTooltip();

    std::vector<View *> m_popovers;
    View *m_tooltipOwner = nullptr;
    View *m_tooltip = nullptr;
    Timer m_tooltipTimer;
};

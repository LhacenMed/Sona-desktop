#pragma once

#include <utility>

#include "widgets/Animation.h"
#include "widgets/View.h"

// Scrolls one content view vertically when it's taller than the viewport: the wheel and touchpad
// scroll it smoothly, and a scrollbar along the right edge shows where it is. Like Windows 11's, the
// bar is a thin line that widens under the pointer; drag its thumb, or click its track to page.
// The content is as wide as the viewport and as tall as it prefers at that width, clipped to the
// viewport.
class ScrollView : public View
{
public:
    template <typename T, typename... Args>
    T &setContent(Args &&...args)
    {
        T &content = add<T>(std::forward<Args>(args)...);
        m_content = &content;
        return content;
    }

    // Only the scrollbar's strip is the scroll view's own; the rest is the content's.
    View *hitTest(Point point) override;
    void layout() override;
    void paintTree(Canvas &canvas) override;
    bool wheelScrolled(float notches) override;
    void mousePressed(Point point) override;
    void mouseDragged(Point point) override;
    void mouseReleased(Point point, bool inside) override;

private:
    float maxOffset() const;
    Rect thumb(float offset) const;
    void scrollTo(float offset);

    View *m_content = nullptr;
    // The content's height, measured when the view is laid out.
    float m_contentHeight = 0;
    Animation m_offset;
    // Where the thumb was grabbed, from its top; negative when it isn't being dragged.
    float m_grab = -1;
};

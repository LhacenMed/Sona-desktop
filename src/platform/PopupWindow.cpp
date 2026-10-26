#include "platform/PopupWindow.h"

#include <ShellScalingApi.h>

#include <algorithm>
#include <cmath>

#include "graphics/Canvas.h"
#include "style/Surface.h"
#include "widgets/View.h"

namespace {

// The surface with its shadow, around the content, in a window with room for the shadow.
class PopupFrame : public View
{
public:
    PopupFrame(const Surface &surface, std::unique_ptr<View> content)
        : m_surface(surface)
        , m_content(add(std::move(content)))
    {
    }

    void layout() override
    {
        const float margin = m_surface.shadowReach();
        m_content.setFrame({margin, margin, frame().width - 2 * margin, frame().height - 2 * margin});
    }

    void paint(Canvas &canvas) override
    {
        canvas.clear(bounds(), Color{});
        canvas.fillSurface(m_content.frame(), m_surface);
    }

private:
    const Surface &m_surface;
    View &m_content;
};

// Where a popup starts along one axis: at `start`, or at `fallback` when it would leave the screen
// there, then moved to stay on screen.
LONG place(LONG start, LONG fallback, LONG length, LONG screenStart, LONG screenEnd)
{
    const LONG position = start + length <= screenEnd ? start : fallback >= screenStart ? fallback : start;
    return std::clamp(position, screenStart, std::max(screenStart, screenEnd - length));
}

} // namespace

PopupWindow::PopupWindow(Window &owner, Rect anchor, PopupPlacement placement, const Surface &surface,
                         std::unique_ptr<View> content)
    : Window(owner.graphics(), owner.theme())
    , m_content(content.get())
{
    owner.closePopup();

    // Sized at the DPI of the monitor it opens on.
    const RECT target = owner.toScreenPixels(anchor);
    const HMONITOR monitor = MonitorFromPoint({target.left, target.top}, MONITOR_DEFAULTTONEAREST);
    MONITORINFO monitorInfo{sizeof(monitorInfo)};
    GetMonitorInfoW(monitor, &monitorInfo);
    UINT dpi = USER_DEFAULT_SCREEN_DPI;
    UINT unusedDpiY = dpi;
    GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpi, &unusedDpiY);
    const float s = static_cast<float>(dpi) / USER_DEFAULT_SCREEN_DPI;
    const Size size = m_content->preferredSize();
    const LONG width = std::lround(size.width * s);
    const LONG height = std::lround(size.height * s);

    const RECT &work = monitorInfo.rcWork;
    const bool beside = placement == PopupPlacement::Beside;
    const bool below = placement == PopupPlacement::Below;
    const LONG left = place(beside ? target.right : target.left, beside ? target.left - width : target.right - width, width,
                            work.left, work.right);
    const LONG top = place(below ? target.bottom : target.top, below ? target.top - height : target.bottom - height, height,
                           work.top, work.bottom);

    // The window reaches past the surface by its shadow.
    const LONG margin = std::lround(surface.shadowReach() * s);
    createPopup(owner.hwnd(), {left - margin, top - margin, left + width + margin, top + height + margin});
    setRoot(std::make_unique<PopupFrame>(surface, std::move(content)));
    owner.m_popup = this;
    show(false);
}

void PopupWindow::closeChain()
{
    Window *outermost = this;
    while (outermost->m_owner->m_frame == WindowFrame::Popup)
        outermost = outermost->m_owner;
    // At once, so whatever runs next (a file dialog) never shows over a menu.
    outermost->m_owner->closePopup();
}

bool PopupWindow::keyPressed(KeyChord chord)
{
    if (chord.key == VK_ESCAPE)
        close();
    else
        m_content->keyPressed(chord);
    return true;
}

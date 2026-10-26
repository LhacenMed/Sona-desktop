#include "platform/PopupWindow.h"

#include <ShellScalingApi.h>

#include <algorithm>
#include <cmath>

#include "graphics/Canvas.h"
#include "style/Theme.h"
#include "widgets/View.h"

namespace {

// Figma's dropdown list surface (2372:8307): 6px corners and a drop shadow 4px down with a 13.8px
// blur, which needs this much transparent room around the surface.
constexpr float shadowMargin = 18;

class PopupFrame : public View
{
public:
    explicit PopupFrame(std::unique_ptr<View> content) : m_content(add(std::move(content))) {}

    void layout() override
    {
        m_content.setFrame({shadowMargin, shadowMargin, frame().width - 2 * shadowMargin, frame().height - 2 * shadowMargin});
    }

    void paint(Canvas &canvas) override
    {
        canvas.clear(bounds(), Color{});
        const Rect surface = m_content.frame();
        canvas.drawShadow(surface, 6, {0, 4}, 13.8f, Color::hex(0x000000, 0.35f));
        canvas.fillRoundedRect(surface, 6, canvas.palette().bgA4);
    }

private:
    View &m_content;
};

} // namespace

PopupWindow::PopupWindow(Window &owner, Point anchor, std::unique_ptr<View> content)
    : Window(owner.graphics(), owner.theme())
    , m_content(content.get())
{
    owner.closePopup();

    // Sized at the DPI of the monitor it opens on.
    const RECT anchorPixels = owner.toScreenPixels({anchor.x, anchor.y, 0, 0});
    const HMONITOR monitor = MonitorFromPoint({anchorPixels.left, anchorPixels.top}, MONITOR_DEFAULTTONEAREST);
    MONITORINFO monitorInfo{sizeof(monitorInfo)};
    GetMonitorInfoW(monitor, &monitorInfo);
    UINT dpi = USER_DEFAULT_SCREEN_DPI;
    UINT unusedDpiY = dpi;
    GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpi, &unusedDpiY);
    const float s = static_cast<float>(dpi) / USER_DEFAULT_SCREEN_DPI;
    const Size size = m_content->preferredSize();
    const LONG margin = std::lround(shadowMargin * s);
    const LONG width = std::lround(size.width * s) + 2 * margin;
    const LONG height = std::lround(size.height * s) + 2 * margin;

    // The surface's top-left at the anchor, moved up or left to stay on the monitor.
    const RECT &work = monitorInfo.rcWork;
    const LONG left = std::clamp(anchorPixels.left - margin, work.left - margin, std::max(work.left - margin, work.right + margin - width));
    const LONG top = std::clamp(anchorPixels.top - margin, work.top - margin, std::max(work.top - margin, work.bottom + margin - height));
    createPopup(owner.hwnd(), {left, top, left + width, top + height});
    setRoot(std::make_unique<PopupFrame>(std::move(content)));
    owner.m_popup = this;
    show(false);
}

bool PopupWindow::keyPressed(KeyChord chord)
{
    if (chord.key == VK_ESCAPE)
        close();
    else
        m_content->keyPressed(chord);
    return true;
}

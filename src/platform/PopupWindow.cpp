#include "platform/PopupWindow.h"

#include <ShellScalingApi.h>

#include <algorithm>
#include <cmath>

#include "graphics/BackdropCapture.h"
#include "graphics/Canvas.h"
#include "style/Surface.h"
#include "widgets/Animation.h"
#include "widgets/View.h"

namespace {

// VS Code's menu motion: the popup fades in while growing from 97% out of the corner nearest what it
// opened from, at Windows' fast pace.
constexpr float openingScale = 0.97f;
constexpr std::chrono::milliseconds openingDuration{167};

// The surface, shadow and frosted backdrop included, with the content on it, in a window with room
// for the shadow; all of it opens with the menu motion.
class PopupFrame : public View
{
public:
    PopupFrame(const Surface &surface, std::unique_ptr<View> content, bool opensRightward, bool opensDownward)
        : m_surface(surface)
        , m_content(add(std::move(content)))
        , m_opensRightward(opensRightward)
        , m_opensDownward(opensDownward)
    {
        m_opening.animateTo(1, openingDuration, Easing::OutQuint);
    }

    void layout() override
    {
        const float margin = m_surface.shadowReach();
        m_content.setFrame({margin, margin, frame().width - 2 * margin, frame().height - 2 * margin});
    }

    // Only the surface takes the mouse; the shadow around it is the window's (Window::hitTest).
    View *hitTest(Point point) override
    {
        if (!m_content.frame().contains(point))
            return nullptr;
        View *hit = View::hitTest(point);
        return hit ? hit : this;
    }

    void paintTree(Canvas &canvas) override
    {
        const float opened = m_opening.value(*this);
        const Rect &surface = m_content.frame();
        const Point origin{frame().x + (m_opensRightward ? surface.x : surface.right()),
                           frame().y + (m_opensDownward ? surface.y : surface.bottom())};
        canvas.clear(frame(), Color{});
        canvas.save();
        canvas.translate(origin.x, origin.y);
        canvas.scale(openingScale + (1 - openingScale) * opened);
        canvas.translate(-origin.x, -origin.y);
        canvas.pushOpacity(opened);
        canvas.save();
        canvas.translate(frame().x, frame().y);
        const std::optional<Backdrop> behind = static_cast<const PopupWindow *>(window())->backdrop();
        canvas.fillSurface(surface, m_surface, behind ? &*behind : nullptr);
        // The content stays within the surface's rounded corners (a panel's tab strip, for one).
        canvas.pushRoundedClip(surface, m_surface.radius);
        canvas.restore();
        View::paintTree(canvas);
        canvas.popRoundedClip();
        canvas.popOpacity();
        canvas.restore();
    }

private:
    const Surface &m_surface;
    View &m_content;
    bool m_opensRightward;
    bool m_opensDownward;
    Animation m_opening;
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
                         std::unique_ptr<View> content, PopupKeys keys)
    : Window(owner.graphics(), owner.theme())
    , m_content(content.get())
    , m_keys(keys)
    , m_backdropCapture(owner.rootWindow().backdropCapture())
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
    const bool docked = placement == PopupPlacement::Docked;
    const Size size = m_content->preferredSize();
    const LONG width = docked ? target.right - target.left : std::lround(size.width * s);
    const LONG height = docked ? target.bottom - target.top : std::lround(size.height * s);

    LONG left = target.left;
    LONG top = target.top;
    if (!docked) {
        const RECT &work = monitorInfo.rcWork;
        const bool beside = placement == PopupPlacement::Beside;
        const bool below = placement == PopupPlacement::Below;
        left = place(beside ? target.right : target.left, beside ? target.left - width : target.right - width, width, work.left,
                     work.right);
        top = place(below ? target.bottom : target.top, below ? target.top - height : target.bottom - height, height, work.top,
                    work.bottom);
    }

    // The window reaches past the surface by its shadow.
    const LONG margin = std::lround(surface.shadowReach() * s);
    createPopup(owner.hwnd(), {left - margin, top - margin, left + width + margin, top + height + margin});
    // It grows away from what it opened from: rightward when it's right of it, downward when below; a
    // docked panel out of its bottom-right corner.
    setRoot(std::make_unique<PopupFrame>(surface, std::move(content), !docked && left + width / 2 >= (target.left + target.right) / 2,
                                         !docked && top + height / 2 >= (target.top + target.bottom) / 2));
    m_backdropCapture.setLive(true);
    m_backdropChanged = m_backdropCapture.changed.connect([this] { invalidate(); });
    owner.m_popup = this;
    show(false);
}

std::optional<Backdrop> PopupWindow::backdrop() const
{
    ID3D11Texture2D *image = m_backdropCapture.image();
    if (!image)
        return {};
    POINT client{};
    ClientToScreen(hwnd(), &client);
    const POINT origin = m_backdropCapture.origin();
    return Backdrop{image, {(origin.x - client.x) / scale(), (origin.y - client.y) / scale()}};
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
    else if (!m_content->keyPressed(chord) && m_keys == PopupKeys::Passed)
        return rootWindow().keyPressed(chord);
    return true;
}

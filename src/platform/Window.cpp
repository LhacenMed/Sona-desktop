#include "platform/Window.h"

#include <dwmapi.h>
#include <imm.h>
#include <ShellScalingApi.h>
#include <windowsx.h>

#include <algorithm>
#include <cmath>
#include <cwchar>
#include <utility>

#include "graphics/BackdropCapture.h"
#include "graphics/WindowSurface.h"
#include "platform/HResult.h"
#include "style/Theme.h"
#include "widgets/Overlay.h"
#include "widgets/View.h"

namespace {

// Posted to an owner when a window it owns is gone, so the owner can delete it.
constexpr UINT ownedWindowClosedMessage = WM_APP;
// Posted to a window with a task to run (runLater), which it takes ownership of.
constexpr UINT laterTaskMessage = WM_APP + 1;

// How long hover feedback takes to fade in or out (Windows' fast control animation).
constexpr std::chrono::milliseconds hoverFade{83};

int frameThicknessForDpi(UINT dpi)
{
    return GetSystemMetricsForDpi(SM_CXFRAME, dpi) + GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
}

bool isCaptionButton(WPARAM hit)
{
    return hit == HTMINBUTTON || hit == HTMAXBUTTON || hit == HTCLOSE;
}

} // namespace

Window::Window(GraphicsDevice &graphics, Theme &theme)
    : m_graphics(graphics)
    , m_theme(theme)
    , m_overlay(std::make_unique<Overlay>())
{
    m_overlay->m_window = this;
}

Window::~Window()
{
    if (m_hwnd)
        DestroyWindow(m_hwnd);
}

void Window::create(const std::wstring &title, Size size, Size minimumSize, WindowFrame frame, HWND owner)
{
    m_frame = frame;
    m_minimumSize = minimumSize;

    // Centred in the work area of the owner's monitor, or of the primary monitor.
    const HMONITOR monitor = owner ? MonitorFromWindow(owner, MONITOR_DEFAULTTONEAREST)
                                   : MonitorFromPoint({0, 0}, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO monitorInfo{sizeof(monitorInfo)};
    GetMonitorInfoW(monitor, &monitorInfo);
    UINT dpi = USER_DEFAULT_SCREEN_DPI;
    UINT unusedDpiY = dpi;
    GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpi, &unusedDpiY);
    // A resizable window keeps invisible resize borders outside its content on the sides and bottom.
    const bool resizable = frame == WindowFrame::Resizable;
    const int border = resizable ? frameThicknessForDpi(dpi) : 0;
    const int width = std::lround(size.width * dpi / USER_DEFAULT_SCREEN_DPI) + 2 * border;
    const int height = std::lround(size.height * dpi / USER_DEFAULT_SCREEN_DPI) + border;
    const RECT &work = monitorInfo.rcWork;
    const LONG left = work.left + (work.right - work.left - width) / 2;
    const LONG top = work.top + (work.bottom - work.top - height) / 2;
    const DWORD style = (resizable ? WS_OVERLAPPEDWINDOW : WS_POPUP | WS_CAPTION | WS_SYSMENU) | WS_CLIPCHILDREN;
    createWindow(title, style, 0, {left, top, left + width, top + height}, owner);
    if (resizable)
        measureCaptionButtons();

    m_modal = frame == WindowFrame::Fixed && owner;
    if (m_modal)
        EnableWindow(owner, FALSE);
}

void Window::createPopup(HWND owner, const RECT &screenPixels)
{
    m_frame = WindowFrame::Popup;
    // No redirection surface: the composition swap chain is all there is, so its transparent pixels
    // (around the drawn shadow) show what's behind. Never activated, so the owner keeps the keyboard.
    createWindow(L"", WS_POPUP, WS_EX_NOREDIRECTIONBITMAP | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE, screenPixels, owner);
}

void Window::createWindow(const std::wstring &title, DWORD style, DWORD extendedStyle, const RECT &bounds, HWND owner)
{
    static const ATOM windowClass = [] {
        const HINSTANCE instance = GetModuleHandleW(nullptr);
        WNDCLASSEXW description{sizeof(description)};
        description.lpfnWndProc = windowProc;
        description.hInstance = instance;
        description.hIcon = static_cast<HICON>(LoadImageW(instance, L"IDI_ICON1", IMAGE_ICON, GetSystemMetrics(SM_CXICON),
                                                          GetSystemMetrics(SM_CYICON), 0));
        description.hIconSm = static_cast<HICON>(LoadImageW(instance, L"IDI_ICON1", IMAGE_ICON,
                                                            GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0));
        description.lpszClassName = L"SonaWindow";
        return RegisterClassExW(&description);
    }();

    CreateWindowExW(extendedStyle, MAKEINTATOM(windowClass), title.c_str(), style, bounds.left, bounds.top,
                    bounds.right - bounds.left, bounds.bottom - bounds.top, owner, nullptr, GetModuleHandleW(nullptr), this);
    if (!m_hwnd)
        check(HRESULT_FROM_WIN32(GetLastError()));
    if (owner)
        m_owner = reinterpret_cast<Window *>(GetWindowLongPtrW(owner, GWLP_USERDATA));

    applyFrameShape();
    applyFrameTheme();
    // Applies the custom frame (WM_NCCALCSIZE).
    SetWindowPos(m_hwnd, nullptr, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

    m_surface = std::make_unique<WindowSurface>(m_graphics, m_theme, m_hwnd, m_dpi);
    m_themeSubscription = m_theme.changed.connect([this] {
        applyFrameTheme();
        invalidate();
    });
}

void Window::setRoot(std::unique_ptr<View> root)
{
    m_root = std::move(root);
    m_root->m_window = this;
    layoutRoot();
}

void Window::measureCaptionButtons()
{
    // Windows reports the caption buttons' size only for a shown window, and showing this one early
    // would spend its open animation, so they are measured on cloaked stand-ins of the same style on
    // the same monitor (so at the same DPI), one restored and one maximized. Owned, they never get a
    // taskbar button; shown without activation, they never take the focus.
    static const ATOM standInClass = [] {
        WNDCLASSEXW description{sizeof(description)};
        description.lpfnWndProc = DefWindowProcW;
        description.hInstance = GetModuleHandleW(nullptr);
        description.lpszClassName = L"SonaCaptionStandIn";
        return RegisterClassExW(&description);
    }();
    RECT window{};
    GetWindowRect(m_hwnd, &window);
    const auto measure = [&](DWORD state) {
        const HWND standIn = CreateWindowExW(0, MAKEINTATOM(standInClass), L"", WS_OVERLAPPEDWINDOW | state, window.left,
                                             window.top, 400, 300, m_hwnd, nullptr, GetModuleHandleW(nullptr), nullptr);
        const BOOL cloaked = TRUE;
        DwmSetWindowAttribute(standIn, DWMWA_CLOAK, &cloaked, sizeof(cloaked));
        ShowWindow(standIn, SW_SHOWNA);
        RECT buttons{};
        DwmGetWindowAttribute(standIn, DWMWA_CAPTION_BUTTON_BOUNDS, &buttons, sizeof(buttons));
        DestroyWindow(standIn);
        return buttons;
    };
    const RECT restored = measure(0);
    m_captionButtons = {restored.right - restored.left, restored.bottom};
    m_maximizedCaptionBottom = measure(WS_MAXIMIZE).bottom;
}

Size Window::captionButtonSize() const
{
    const float s = scale();
    // Unmeasured (a window without a title bar of its own, drawing a picture of one): Windows' sizes.
    if (m_captionButtons.cx == 0)
        return {46, GetSystemMetricsForDpi(SM_CYCAPTION, static_cast<UINT>(m_dpi)) / s};
    const float width = m_captionButtons.cx / 3.0f / s;
    // A maximized window's top edge lies off screen, so only the part below the monitor edge counts.
    if (isMaximized() && m_maximizedCaptionBottom > 0)
        return {width, m_maximizedCaptionBottom / s - contentTop()};
    return {width, m_captionButtons.cy / s};
}

bool Window::isMaximized() const
{
    return !m_fullScreen && IsZoomed(m_hwnd);
}

void Window::hidePointer()
{
    if (m_pointerHidden || !m_pointer)
        return;
    m_pointerHidden = true;
    SetCursor(nullptr);
}

void Window::show(bool maximized)
{
    // Laid out again for views added since the root was set, then a plain first showing, which is
    // what Windows plays its open animation on.
    layoutRoot();
    render();
    if (m_frame == WindowFrame::Popup)
        ShowWindow(m_hwnd, SW_SHOWNOACTIVATE);
    else
        ShowWindow(m_hwnd, maximized ? SW_SHOWMAXIMIZED : SW_SHOWNORMAL);
    // Warm before any popup opens over it, so the first has its backdrop at once; started once it's
    // shown, which starting would hold up.
    if (m_frame == WindowFrame::Resizable)
        runLater([this] { backdropCapture(); });
}

void Window::toggleFullScreen()
{
    // Raymond Chen's full-screen toggle: drop the frame styles and cover the monitor, then restore
    // the previous placement, which brings back a maximized state too.
    const LONG_PTR style = GetWindowLongPtrW(m_hwnd, GWL_STYLE);
    if (!m_fullScreen) {
        MONITORINFO monitorInfo{sizeof(monitorInfo)};
        if (!GetWindowPlacement(m_hwnd, &m_placementBeforeFullScreen)
            || !GetMonitorInfoW(MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTOPRIMARY), &monitorInfo))
            return;
        m_fullScreen = true;
        SetWindowLongPtrW(m_hwnd, GWL_STYLE, style & ~WS_OVERLAPPEDWINDOW);
        const RECT &screen = monitorInfo.rcMonitor;
        applyFrameShape();
        SetWindowPos(m_hwnd, HWND_TOP, screen.left, screen.top, screen.right - screen.left, screen.bottom - screen.top,
                     SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    } else {
        m_fullScreen = false;
        applyFrameShape();
        SetWindowLongPtrW(m_hwnd, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(m_hwnd, &m_placementBeforeFullScreen);
        SetWindowPos(m_hwnd, nullptr, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}

void Window::close()
{
    PostMessageW(m_hwnd, WM_CLOSE, 0, 0);
}

void Window::invalidate()
{
    if (m_hwnd)
        InvalidateRect(m_hwnd, nullptr, FALSE);
}

void Window::setFocus(View *view)
{
    if (view == m_focused)
        return;
    if (View *previous = std::exchange(m_focused, view)) {
        previous->m_focused = false;
        previous->focusChanged();
    }
    if (view) {
        view->m_focused = true;
        view->focusChanged();
    }
    invalidate();
}

RECT Window::toScreenPixels(const Rect &rect) const
{
    const float s = scale();
    POINT topLeft{std::lround(rect.x * s), std::lround(rect.y * s)};
    POINT bottomRight{std::lround(rect.right() * s), std::lround(rect.bottom() * s)};
    ClientToScreen(m_hwnd, &topLeft);
    ClientToScreen(m_hwnd, &bottomRight);
    return {topLeft.x, topLeft.y, bottomRight.x, bottomRight.y};
}

void Window::closeRequested()
{
    // Re-enabled before this window goes, so Windows activates the owner next.
    if (m_modal)
        EnableWindow(GetWindow(m_hwnd, GW_OWNER), TRUE);
    DestroyWindow(m_hwnd);
}

LRESULT CALLBACK Window::windowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_NCCREATE) {
        auto *created = static_cast<Window *>(reinterpret_cast<CREATESTRUCTW *>(lParam)->lpCreateParams);
        created->m_hwnd = hwnd;
        created->m_dpi = static_cast<float>(GetDpiForWindow(hwnd));
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(created));
    }
    auto *self = reinterpret_cast<Window *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (!self)
        return DefWindowProcW(hwnd, message, wParam, lParam);
    if (message == WM_NCDESTROY) {
        self->m_hwnd = nullptr;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
        if (self->m_owner && self->m_owner->m_hwnd)
            PostMessageW(self->m_owner->m_hwnd, ownedWindowClosedMessage, 0, reinterpret_cast<LPARAM>(self));
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }
    return self->handleMessage(message, wParam, lParam);
}

LRESULT Window::handleMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_NCCALCSIZE: {
        if (m_frame != WindowFrame::Resizable)
            return 0; // The whole window is content.
        RECT &client = wParam ? reinterpret_cast<NCCALCSIZE_PARAMS *>(lParam)->rgrc[0] : *reinterpret_cast<RECT *>(lParam);
        const LONG top = client.top;
        DefWindowProcW(m_hwnd, message, wParam, lParam);
        // Keeps the standard resize borders on the sides and bottom, but no title bar: the views draw
        // it. The client area keeps the window's top edge even when maximized, where that edge lies
        // off screen, so the caption buttons reach the monitor's top edge as Windows' own do; the
        // content starts below the off-screen part instead (contentTop).
        client.top = top;
        return 0;
    }
    case WM_NCHITTEST:
        return hitTest(lParam);
    case WM_NCMOUSEMOVE:
        // Over the title bar, its buttons and the resize borders, Windows reports the pointer as over
        // the frame; it's still over the window.
        if (!m_trackingNonClientMouse) {
            TRACKMOUSEEVENT track{sizeof(track), TME_LEAVE | TME_NONCLIENT, m_hwnd, 0};
            TrackMouseEvent(&track);
            m_trackingNonClientMouse = true;
        }
        pointerMoved(screenToDips(lParam));
        // Windows still sees the move, which is what opens Snap Layouts over the maximize button.
        break;
    case WM_NCLBUTTONDOWN:
    case WM_NCLBUTTONDBLCLK:
    case WM_NCLBUTTONUP:
        if (captionButtonMessage(message, wParam, lParam))
            return 0;
        // A click on the frame or title bar dismisses popups as a click on the content does.
        if (message == WM_NCLBUTTONDOWN)
            closePopup();
        break;
    case WM_NCMOUSELEAVE:
        m_trackingNonClientMouse = false;
        if (GetCapture() != m_hwnd)
            pointerLeft();
        return 0;
    case WM_NCRBUTTONDOWN:
    case WM_RBUTTONDOWN:
        closePopup();
        break;
    case WM_CONTEXTMENU: {
        // Over the content only: the title bar's right click is Windows' system menu.
        if (lParam != -1 && hitTest(lParam) != HTCLIENT)
            break;
        RECT client{};
        GetClientRect(m_hwnd, &client);
        // From the keyboard (the menu key, Shift+F10), at the window's centre.
        contextMenuRequested(lParam == -1 ? toDips({client.right / 2, client.bottom / 2}) : screenToDips(lParam));
        return 0;
    }
    case WM_MOUSEACTIVATE:
        if (m_frame == WindowFrame::Popup)
            return MA_NOACTIVATE;
        break;
    case WM_SETCURSOR:
        if (LOWORD(lParam) == HTCLIENT) {
            applyCursor();
            return TRUE;
        }
        break;
    case WM_MOUSEMOVE:
        if (!m_trackingMouse) {
            TRACKMOUSEEVENT track{sizeof(track), TME_LEAVE, m_hwnd, 0};
            TrackMouseEvent(&track);
            m_trackingMouse = true;
        }
        pointerMoved(toDips({GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)}));
        return 0;
    case WM_MOUSELEAVE:
        m_trackingMouse = false;
        // A press keeps the mouse captured, and with it the hover.
        if (GetCapture() != m_hwnd)
            pointerLeft();
        return 0;
    case WM_LBUTTONDOWN:
        SetCapture(m_hwnd);
        pointerPressed(toDips({GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)}));
        return 0;
    case WM_LBUTTONUP:
        pointerReleased(toDips({GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)}));
        ReleaseCapture();
        return 0;
    case WM_CAPTURECHANGED:
        if (reinterpret_cast<HWND>(lParam) != m_hwnd)
            cancelPress();
        return 0;
    case WM_MOUSEWHEEL:
        wheelScrolled(screenToDips(lParam), static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / WHEEL_DELTA);
        return 0;
    case WM_NCACTIVATE:
        m_active = wParam != FALSE;
        invalidate();
        // -1: there is no standard frame for Windows to repaint.
        return DefWindowProcW(m_hwnd, message, wParam, -1);
    case WM_ACTIVATE:
        if (LOWORD(wParam) == WA_INACTIVE)
            closePopup();
        break;
    case WM_WINDOWPOSCHANGED: {
        // A popup stays where it opened, so it closes when its owner moves or resizes.
        const auto *position = reinterpret_cast<const WINDOWPOS *>(lParam);
        if (!(position->flags & SWP_NOMOVE) || !(position->flags & SWP_NOSIZE))
            closePopup();
        break; // Windows sends WM_SIZE from here.
    }
    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED && m_surface) {
            m_surface->resize();
            layoutRoot();
            render();
        }
        return 0;
    case WM_PAINT: {
        PAINTSTRUCT paint;
        BeginPaint(m_hwnd, &paint);
        render();
        EndPaint(m_hwnd, &paint);
        return 0;
    }
    case WM_ERASEBKGND: {
        // The window's GDI surface lies under the views (and holds libVLC's video windows). It starts
        // out white; it's filled black, as the video's background, before the first frame is on screen.
        RECT client{};
        GetClientRect(m_hwnd, &client);
        FillRect(reinterpret_cast<HDC>(wParam), &client, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
        return 1;
    }
    case WM_DPICHANGED: {
        m_dpi = HIWORD(wParam);
        m_surface->setDpi(m_dpi);
        applyFrameShape();
        const RECT *suggested = reinterpret_cast<const RECT *>(lParam);
        SetWindowPos(m_hwnd, nullptr, suggested->left, suggested->top, suggested->right - suggested->left,
                     suggested->bottom - suggested->top, SWP_NOZORDER | SWP_NOACTIVATE);
        if (m_frame == WindowFrame::Resizable)
            measureCaptionButtons();
        layoutRoot();
        invalidate();
        return 0;
    }
    case WM_GETMINMAXINFO: {
        auto *limits = reinterpret_cast<MINMAXINFO *>(lParam);
        const int border = m_frame == WindowFrame::Resizable ? frameThickness() : 0;
        limits->ptMinTrackSize = {std::lround(m_minimumSize.width * scale()) + 2 * border,
                                  std::lround(m_minimumSize.height * scale()) + border};
        return 0;
    }
    case WM_SETTINGCHANGE:
        if (lParam && wcscmp(reinterpret_cast<const wchar_t *>(lParam), L"ImmersiveColorSet") == 0)
            m_theme.refresh();
        break;
    case WM_KEYDOWN:
        if (handleKey({static_cast<UINT>(wParam), GetKeyState(VK_CONTROL) < 0, GetKeyState(VK_SHIFT) < 0,
                       GetKeyState(VK_MENU) < 0}))
            return 0;
        break;
    case WM_CHAR:
        handleCharacter(static_cast<wchar_t>(wParam));
        return 0;
    case WM_IME_STARTCOMPOSITION:
        placeImeWindows();
        break; // Windows draws the composition window.
    case WM_DROPFILES:
        filesDropped(reinterpret_cast<HDROP>(wParam));
        return 0;
    case WM_CLOSE:
        closeRequested();
        return 0;
    case WM_DESTROY:
        if (m_owner && m_owner->m_popup == this) {
            m_owner->m_popup = nullptr;
            m_owner->pointerChanged();
            // The last popup over the window closed.
            if (m_owner->m_frame != WindowFrame::Popup && m_owner->m_backdropCapture)
                m_owner->m_backdropCapture->setLive(false);
        }
        // The surface draws into this window only; its GPU memory goes with it.
        m_surface.reset();
        destroyed();
        return 0;
    case ownedWindowClosedMessage:
        std::erase_if(m_ownedWindows, [lParam](const auto &owned) { return reinterpret_cast<LPARAM>(owned.get()) == lParam; });
        return 0;
    case laterTaskMessage:
        (*std::unique_ptr<std::function<void()>>(reinterpret_cast<std::function<void()> *>(lParam)))();
        return 0;
    }
    return DefWindowProcW(m_hwnd, message, wParam, lParam);
}

bool Window::captionButtonMessage(UINT message, WPARAM hit, LPARAM screenPoint)
{
    if (!isCaptionButton(hit))
        return false;
    const Point point = screenToDips(screenPoint);
    switch (message) {
    case WM_NCLBUTTONDOWN:
    case WM_NCLBUTTONDBLCLK:
        // Pressed like any view, so Windows never draws its classic buttons; the release arrives as
        // a client message, since the press captures the mouse.
        SetCapture(m_hwnd);
        pointerPressed(point);
        return true;
    default:
        pointerReleased(point);
        return true;
    }
}

LRESULT Window::hitTest(LPARAM screenPoint)
{
    // The standard frame answers for the resize borders outside the content.
    const LRESULT frameHit = DefWindowProcW(m_hwnd, WM_NCHITTEST, 0, screenPoint);
    if (frameHit != HTCLIENT)
        return frameHit;
    if (m_fullScreen)
        return HTCLIENT;

    POINT client{GET_X_LPARAM(screenPoint), GET_Y_LPARAM(screenPoint)};
    ScreenToClient(m_hwnd, &client);
    // There's no frame above the content, so the top resize border lies inside it.
    if (m_frame == WindowFrame::Resizable && !isMaximized() && client.y < frameThickness())
        return HTTOP;

    const View *view = viewAt(toDips(client));
    // Around a popup's surface, over its shadow, the mouse is for the window behind it.
    if (!view && m_frame == WindowFrame::Popup)
        return HTTRANSPARENT;
    switch (view ? view->hitRole() : HitRole::Client) {
    case HitRole::Caption:
        return HTCAPTION;
    case HitRole::SystemMenu:
        return HTSYSMENU;
    case HitRole::Minimize:
        return HTMINBUTTON;
    case HitRole::Maximize:
        return HTMAXBUTTON;
    case HitRole::Close:
        return HTCLOSE;
    case HitRole::Client:
        break;
    }
    return HTCLIENT;
}

Point Window::toDips(POINT clientPoint) const
{
    return {clientPoint.x / scale(), clientPoint.y / scale()};
}

Point Window::screenToDips(LPARAM screenPoint) const
{
    POINT client{GET_X_LPARAM(screenPoint), GET_Y_LPARAM(screenPoint)};
    ScreenToClient(m_hwnd, &client);
    return toDips(client);
}

float Window::contentTop() const
{
    // A maximized window extends past the monitor by its frame thickness.
    return isMaximized() ? frameThickness() / scale() : 0;
}

bool Window::isUnderPopup() const
{
    // A menu stays live under its submenu.
    return m_popup && m_frame != WindowFrame::Popup;
}

View *Window::viewAt(Point point) const
{
    if (!m_root)
        return nullptr;
    const Rect &content = m_root->frame();
    const Point local{point.x - content.x, point.y - content.y};
    View *hit = m_overlay->hitTest(local);
    if (!hit)
        hit = m_root->hitTest(local);
    // While something floats over the content, the rest takes no part, as behind native menus and
    // flyouts: it doesn't hover, show its pointer or tooltip, or scroll, and a click there only
    // closes what floats. What acts on popups itself (a panel's button) still does.
    return !isUnderPopup() || (hit && hit->actsOnPopups()) ? hit : nullptr;
}

RECT Window::contentToClientPixels(const Rect &rect) const
{
    // Rounded outwards, so a child window covers every pixel the view touches.
    const float top = contentTop();
    const float s = scale();
    return {std::lround(std::floor(rect.x * s)), std::lround(std::floor((rect.y + top) * s)),
            std::lround(std::ceil(rect.right() * s)), std::lround(std::ceil((rect.bottom() + top) * s))};
}

int Window::frameThickness() const
{
    return frameThicknessForDpi(static_cast<UINT>(m_dpi));
}

void Window::applyFrameShape()
{
    // Full screen covers the monitor edge to edge, and a popup draws its own corners and shadow:
    // square corners and no border for both.
    const bool frameless = m_fullScreen || m_frame == WindowFrame::Popup;
    const DWM_WINDOW_CORNER_PREFERENCE corners = frameless ? DWMWCP_DONOTROUND : DWMWCP_ROUND;
    DwmSetWindowAttribute(m_hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &corners, sizeof(corners));
    const COLORREF border = frameless ? DWMWA_COLOR_NONE : DWMWA_COLOR_DEFAULT;
    DwmSetWindowAttribute(m_hwnd, DWMWA_BORDER_COLOR, &border, sizeof(border));
    updateFrameMargins();
}

void Window::updateFrameMargins()
{
    // A sliver of DWM frame keeps the window's shadow; a popup draws its own, and full screen has none.
    const int top = m_frame == WindowFrame::Popup || m_fullScreen ? 0 : 1;
    if (top == m_frameMarginTop)
        return;
    m_frameMarginTop = top;
    const MARGINS margins{0, 0, top, 0};
    DwmExtendFrameIntoClientArea(m_hwnd, &margins);
}

void Window::applyFrameTheme()
{
    // The window border and system menu follow the app theme.
    const BOOL dark = m_theme.isDark();
    DwmSetWindowAttribute(m_hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
}

void Window::layoutRoot()
{
    if (!m_root)
        return;
    RECT client{};
    GetClientRect(m_hwnd, &client);
    updateFrameMargins();
    const float top = contentTop();
    const Rect content{0, top, client.right / scale(), client.bottom / scale() - top};
    m_root->setFrame(content);
    m_overlay->setFrame(content);
    layoutChanged();
}

void Window::render()
{
    if (m_surface && m_root) {
        m_surface->render([this](Canvas &canvas) {
            m_root->paintTree(canvas);
            m_overlay->paintTree(canvas);
        });
    }
}

bool Window::handleKey(KeyChord chord)
{
    if (m_popup) {
        m_popup->handleKey(chord);
        return true;
    }
    if (m_focused && m_focused->keyPressed(chord))
        return true;
    return keyPressed(chord);
}

void Window::handleCharacter(wchar_t character)
{
    if (m_popup)
        m_popup->handleCharacter(character);
    else if (m_focused)
        m_focused->charTyped(character);
}

void Window::pointerMoved(Point point)
{
    // Windows repeats moves without movement (after a cursor change, for one); only real ones show
    // a hidden pointer again.
    const bool moved = !m_pointer || m_pointer->x != point.x || m_pointer->y != point.y;
    m_pointer = point;
    if (moved && m_pointerHidden) {
        m_pointerHidden = false;
        applyCursor();
    }
    View *target = viewAt(point);
    // While a view is pressed, only it can be hovered.
    setHovered(m_pressed && target != m_pressed ? nullptr : target);
    if (m_pressed)
        m_pressed->mouseDragged(m_pressed->mapFromWindow(point));
    else if (target && moved)
        target->mouseMoved(target->mapFromWindow(point));
    if (moved)
        pointerChanged();
}

void Window::applyCursor()
{
    if (m_pointerHidden) {
        SetCursor(nullptr);
        return;
    }
    POINT cursor{};
    GetCursorPos(&cursor);
    ScreenToClient(m_hwnd, &cursor);
    const View *view = viewAt(toDips(cursor));
    const Cursor shape = view ? view->cursor() : Cursor::Arrow;
    SetCursor(LoadCursorW(nullptr, shape == Cursor::Hand ? IDC_HAND : shape == Cursor::Text ? IDC_IBEAM : IDC_ARROW));
}

void Window::pointerLeft()
{
    setHovered(nullptr);
    // Moving between the content and the frame (the title bar, the resize borders) leaves one and
    // enters the other: the pointer has left only when the window under it is another one.
    POINT cursor{};
    GetCursorPos(&cursor);
    if (GetAncestor(WindowFromPoint(cursor), GA_ROOT) == m_hwnd) {
        ScreenToClient(m_hwnd, &cursor);
        m_pointer = toDips(cursor);
    } else {
        m_pointer.reset();
    }
    pointerChanged();
}

void Window::pointerPressed(Point point)
{
    m_overlay->hideTooltip();
    View *target = viewAt(point);
    // A click outside an open popup only closes it, unless it's on what acts on popups (viewAt). A
    // menu's submenu is left to the menu's rows (Menu): clicking its own row keeps it, another row
    // replaces or closes it.
    if (isUnderPopup() && !target) {
        closePopup();
        return;
    }
    // The clicked view takes the focus, or the nearest view around it that accepts it (a list, for
    // a click on one of its rows).
    View *focus = target;
    while (focus && !focus->acceptsFocus())
        focus = focus->parent();
    setFocus(focus);
    if (!target)
        return;
    m_pressed = target;
    target->m_pressed = true;
    setHovered(target);
    target->mousePressed(target->mapFromWindow(point));
    invalidate();
}

void Window::pointerReleased(Point point)
{
    View *released = std::exchange(m_pressed, nullptr);
    if (!released)
        return;
    released->m_pressed = false;
    const Point local = released->mapFromWindow(point);
    released->mouseReleased(local, released->bounds().contains(local));
    // The release may have removed views (a closed list), so the target is found again.
    pointerMoved(point);
    pointerChanged();
    invalidate();
}

void Window::wheelScrolled(Point point, float notches)
{
    m_overlay->hideTooltip();
    if (!m_root)
        return;
    // Any view can scroll, not only those that take clicks: the deepest one under the pointer is
    // offered the scroll first, then its parents. Content under a popup doesn't scroll (viewAt).
    if (isUnderPopup())
        return;
    const Rect &content = m_root->frame();
    const Point local{point.x - content.x, point.y - content.y};
    View *target = deepestViewAt(*m_overlay, local);
    if (target == m_overlay.get())
        target = deepestViewAt(*m_root, local);
    for (View *view = target; view; view = view->parent()) {
        if (view->wheelScrolled(notches)) {
            // Different content is under the pointer now.
            pointerMoved(point);
            return;
        }
    }
}

View *Window::deepestViewAt(View &view, Point point)
{
    if (!view.m_visible || !view.bounds().contains(point))
        return nullptr;
    for (auto child = view.m_children.rbegin(); child != view.m_children.rend(); ++child) {
        const Rect &frame = (*child)->m_frame;
        if (View *hit = deepestViewAt(**child, {point.x - frame.x, point.y - frame.y}))
            return hit;
    }
    return &view;
}

void Window::cancelPress()
{
    if (!m_pressed)
        return;
    std::exchange(m_pressed, nullptr)->m_pressed = false;
    pointerChanged();
    invalidate();
}

void Window::setHovered(View *view)
{
    if (view == m_hovered)
        return;
    if (View *previous = std::exchange(m_hovered, view)) {
        previous->m_hovered = false;
        previous->m_hover.animateTo(0, hoverFade);
        previous->hoverChanged();
    }
    if (view) {
        view->m_hovered = true;
        view->m_hover.animateTo(1, hoverFade);
        view->hoverChanged();
    }
    m_overlay->hoverChanged(view);
    invalidate();
}

void Window::runLater(std::function<void()> task)
{
    auto *posted = new std::function<void()>(std::move(task));
    if (!PostMessageW(m_hwnd, laterTaskMessage, 0, reinterpret_cast<LPARAM>(posted)))
        delete posted;
}

Window &Window::rootWindow()
{
    Window *root = this;
    while (root->m_frame == WindowFrame::Popup)
        root = root->m_owner;
    return *root;
}

BackdropCapture &Window::backdropCapture()
{
    if (!m_backdropCapture)
        m_backdropCapture = std::make_unique<BackdropCapture>(m_graphics, m_hwnd);
    return *m_backdropCapture;
}

void Window::closePopup()
{
    // Destroying it clears m_popup (WM_DESTROY).
    if (m_popup)
        DestroyWindow(m_popup->hwnd());
}

void Window::placeImeWindows()
{
    if (!m_focused)
        return;
    const Rect bounds = m_focused->mapToWindow(m_focused->caretBounds());
    const float s = scale();
    const RECT caret{std::lround(bounds.x * s), std::lround(bounds.y * s), std::lround(bounds.right() * s),
                     std::lround(bounds.bottom() * s)};
    const HIMC context = ImmGetContext(m_hwnd);
    if (!context)
        return;
    COMPOSITIONFORM composition{CFS_POINT, {caret.left, caret.top}, {}};
    ImmSetCompositionWindow(context, &composition);
    // The candidate list opens below the caret, never over it.
    CANDIDATEFORM candidates{0, CFS_EXCLUDE, {caret.left, caret.top}, caret};
    ImmSetCandidateWindow(context, &candidates);
    ImmReleaseContext(m_hwnd, context);
}

void Window::viewRemoved(View &removed)
{
    const auto inside = [&removed](const View *view) {
        for (; view; view = view->m_parent) {
            if (view == &removed)
                return true;
        }
        return false;
    };
    if (inside(m_hovered)) {
        m_hovered = nullptr;
        m_overlay->hoverChanged(nullptr);
    }
    if (inside(m_pressed))
        m_pressed = nullptr;
    if (inside(m_focused))
        m_focused = nullptr;
}

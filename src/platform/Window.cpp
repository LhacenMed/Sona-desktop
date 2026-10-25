#include "platform/Window.h"

#include <dwmapi.h>
#include <ShellScalingApi.h>
#include <windowsx.h>

#include <cmath>
#include <cwchar>
#include <utility>

#include "graphics/WindowSurface.h"
#include "platform/HResult.h"
#include "style/Theme.h"
#include "widgets/View.h"

namespace {

int frameThicknessForDpi(UINT dpi)
{
    return GetSystemMetricsForDpi(SM_CXFRAME, dpi) + GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
}

} // namespace

Window::Window(GraphicsDevice &graphics, Theme &theme)
    : m_graphics(graphics)
    , m_theme(theme)
{
}

Window::~Window()
{
    if (m_hwnd)
        DestroyWindow(m_hwnd);
}

void Window::create(const std::wstring &title, Size size, Size minimumSize, bool resizable, HWND owner)
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
    m_resizable = resizable;
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
    const int frame = resizable ? frameThicknessForDpi(dpi) : 0;
    const int width = std::lround(size.width * dpi / USER_DEFAULT_SCREEN_DPI) + 2 * frame;
    const int height = std::lround(size.height * dpi / USER_DEFAULT_SCREEN_DPI) + frame;
    const RECT &work = monitorInfo.rcWork;
    const DWORD style = (resizable ? WS_OVERLAPPEDWINDOW : WS_POPUP | WS_CAPTION | WS_SYSMENU) | WS_CLIPCHILDREN;

    CreateWindowExW(0, MAKEINTATOM(windowClass), title.c_str(), style, work.left + (work.right - work.left - width) / 2,
                    work.top + (work.bottom - work.top - height) / 2, width, height, owner, nullptr,
                    GetModuleHandleW(nullptr), this);
    if (!m_hwnd)
        check(HRESULT_FROM_WIN32(GetLastError()));

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

void Window::setNativeCaption(Color Palette::*background)
{
    m_nativeCaption = true;
    m_captionBackground = background;
    measureCaptionButtons();
    applyFrameTheme();
    layoutRoot();
}

Rect Window::captionButtonBounds() const
{
    if (!m_nativeCaption || m_fullScreen)
        return {};
    // Flush with the window's top right corner, which lies off screen when maximized.
    RECT client{};
    GetClientRect(m_hwnd, &client);
    const float width = m_captionButtons.cx / scale();
    return {client.right / scale() - width, -contentTop(), width, m_captionButtons.cy / scale()};
}

void Window::measureCaptionButtons()
{
    // Windows reports the caption buttons' size only for a shown window, and showing this one early
    // would spend its open animation, so they are measured on a cloaked stand-in of the same style on
    // the same monitor (so at the same DPI). Owned, it never gets a taskbar button.
    static const ATOM standInClass = [] {
        WNDCLASSEXW description{sizeof(description)};
        description.lpfnWndProc = DefWindowProcW;
        description.hInstance = GetModuleHandleW(nullptr);
        description.lpszClassName = L"SonaCaptionStandIn";
        return RegisterClassExW(&description);
    }();
    RECT window{};
    GetWindowRect(m_hwnd, &window);
    const HWND standIn = CreateWindowExW(0, MAKEINTATOM(standInClass), L"", WS_OVERLAPPEDWINDOW, window.left, window.top,
                                         400, 300, m_hwnd, nullptr, GetModuleHandleW(nullptr), nullptr);
    const BOOL cloaked = TRUE;
    DwmSetWindowAttribute(standIn, DWMWA_CLOAK, &cloaked, sizeof(cloaked));
    ShowWindow(standIn, SW_SHOWNA);
    RECT buttons{};
    DwmGetWindowAttribute(standIn, DWMWA_CAPTION_BUTTON_BOUNDS, &buttons, sizeof(buttons));
    DestroyWindow(standIn);
    m_captionButtons = {buttons.right - buttons.left, buttons.bottom};
}

float Window::captionHeight() const
{
    // The caption buttons' height, from their top edge, which lies off screen in a maximized window,
    // so only the part below the monitor edge counts. Without them, the system caption height.
    const Rect buttons = captionButtonBounds();
    if (buttons.height > 0)
        return buttons.bottom();
    return GetSystemMetricsForDpi(SM_CYCAPTION, static_cast<UINT>(m_dpi)) / scale();
}

bool Window::isMaximized() const
{
    return !m_fullScreen && IsZoomed(m_hwnd);
}

void Window::show(bool maximized)
{
    // A plain first showing, which is what Windows plays its open animation on.
    render();
    ShowWindow(m_hwnd, maximized ? SW_SHOWMAXIMIZED : SW_SHOWNORMAL);
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

void Window::closeRequested()
{
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
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }
    return self->handleMessage(message, wParam, lParam);
}

LRESULT Window::handleMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
    if (messageReceived(message, wParam, lParam))
        return 0;
    // Windows runs its caption buttons itself: hover, press, Snap Layouts. Hit testing asks it in order.
    if (m_nativeCaption && message != WM_NCHITTEST) {
        LRESULT result = 0;
        if (DwmDefWindowProc(m_hwnd, message, wParam, lParam, &result))
            return result;
    }

    switch (message) {
    case WM_NCCALCSIZE: {
        if (!m_resizable)
            return 0; // The whole window is content.
        RECT &client = wParam ? reinterpret_cast<NCCALCSIZE_PARAMS *>(lParam)->rgrc[0] : *reinterpret_cast<RECT *>(lParam);
        const LONG top = client.top;
        DefWindowProcW(m_hwnd, message, wParam, lParam);
        // Keeps the standard resize borders on the sides and bottom, but no title bar: the views draw
        // it. The client area keeps the window's top edge even when maximized, where that edge lies off
        // screen, because Windows' caption buttons only respond in a client area that starts there; the
        // content starts below the off-screen part instead (contentTop).
        client.top = top;
        return 0;
    }
    case WM_NCHITTEST:
        return hitTest(lParam);
    case WM_SETCURSOR:
        if (LOWORD(lParam) == HTCLIENT) {
            POINT cursor{};
            GetCursorPos(&cursor);
            ScreenToClient(m_hwnd, &cursor);
            const View *view = viewAt(toDips(cursor));
            SetCursor(LoadCursorW(nullptr, view && view->showsHandCursor() ? IDC_HAND : IDC_ARROW));
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
            setHovered(nullptr);
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
    case WM_NCACTIVATE:
        m_active = wParam != FALSE;
        invalidate();
        // -1: there is no standard frame for Windows to repaint.
        return DefWindowProcW(m_hwnd, message, wParam, -1);
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
        // out white; black is transparent over the native caption, so Windows' caption shows where the
        // views are clear. Erasing happens as the window is shown, before its first frame is on screen.
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
        if (m_nativeCaption)
            measureCaptionButtons();
        layoutRoot();
        invalidate();
        return 0;
    }
    case WM_GETMINMAXINFO: {
        auto *limits = reinterpret_cast<MINMAXINFO *>(lParam);
        const int frame = m_resizable ? frameThickness() : 0;
        limits->ptMinTrackSize = {std::lround(m_minimumSize.width * scale()) + 2 * frame,
                                  std::lround(m_minimumSize.height * scale()) + frame};
        return 0;
    }
    case WM_SETTINGCHANGE:
        if (lParam && wcscmp(reinterpret_cast<const wchar_t *>(lParam), L"ImmersiveColorSet") == 0)
            m_theme.refresh();
        break;
    case WM_KEYDOWN:
        if (keyPressed(static_cast<UINT>(wParam), GetKeyState(VK_SHIFT) < 0, GetKeyState(VK_CONTROL) < 0))
            return 0;
        break;
    case WM_DROPFILES:
        filesDropped(reinterpret_cast<HDROP>(wParam));
        return 0;
    case WM_CLOSE:
        closeRequested();
        return 0;
    case WM_DESTROY:
        // The surface draws into this window only; its GPU memory goes with it.
        m_surface.reset();
        destroyed();
        return 0;
    }
    return DefWindowProcW(m_hwnd, message, wParam, lParam);
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
    // The caption buttons come first, so they reach the window's top edge as native ones do.
    LRESULT buttonHit = HTNOWHERE;
    if (m_nativeCaption && DwmDefWindowProc(m_hwnd, WM_NCHITTEST, 0, screenPoint, &buttonHit))
        return buttonHit;
    // There's no frame above the content, so the top resize border lies inside it.
    if (m_resizable && !isMaximized() && client.y < frameThickness())
        return HTTOP;

    const View *view = viewAt(toDips(client));
    switch (view ? view->hitRole() : HitRole::Client) {
    case HitRole::Caption:
        return HTCAPTION;
    case HitRole::SystemMenu:
        return HTSYSMENU;
    case HitRole::Client:
        break;
    }
    return HTCLIENT;
}

Point Window::toDips(POINT clientPoint) const
{
    return {clientPoint.x / scale(), clientPoint.y / scale()};
}

float Window::contentTop() const
{
    // A maximized window extends past the monitor by its frame thickness.
    return isMaximized() ? frameThickness() / scale() : 0;
}

View *Window::viewAt(Point point) const
{
    if (!m_root)
        return nullptr;
    const Rect &content = m_root->frame();
    return m_root->hitTest({point.x - content.x, point.y - content.y});
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
    // Full screen covers the monitor edge to edge: square corners and no border.
    const DWM_WINDOW_CORNER_PREFERENCE corners = m_fullScreen ? DWMWCP_DONOTROUND : DWMWCP_ROUND;
    DwmSetWindowAttribute(m_hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &corners, sizeof(corners));
    const COLORREF border = m_fullScreen ? DWMWA_COLOR_NONE : DWMWA_COLOR_DEFAULT;
    DwmSetWindowAttribute(m_hwnd, DWMWA_BORDER_COLOR, &border, sizeof(border));
    updateFrameMargins();
}

void Window::updateFrameMargins()
{
    // The native caption is DWM frame extended under the title bar: Windows paints it in the caption
    // colour and draws its buttons there, behind the views. Without one, a sliver of frame keeps the
    // shadow.
    int top = 1;
    if (m_nativeCaption)
        top = m_fullScreen ? 0 : static_cast<int>(std::lround((contentTop() + captionHeight()) * scale()));
    if (top == m_frameMarginTop)
        return;
    m_frameMarginTop = top;
    const MARGINS margins{0, 0, top, 0};
    DwmExtendFrameIntoClientArea(m_hwnd, &margins);
}

void Window::applyFrameTheme()
{
    // The window border, caption buttons and system menu follow the app theme.
    const BOOL dark = m_theme.isDark();
    DwmSetWindowAttribute(m_hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
    if (m_captionBackground) {
        const Color color = m_theme.palette().*m_captionBackground;
        const COLORREF caption = RGB(std::lround(color.r * 255), std::lround(color.g * 255), std::lround(color.b * 255));
        DwmSetWindowAttribute(m_hwnd, DWMWA_CAPTION_COLOR, &caption, sizeof(caption));
    }
}

void Window::layoutRoot()
{
    if (!m_root)
        return;
    RECT client{};
    GetClientRect(m_hwnd, &client);
    updateFrameMargins();
    const float top = contentTop();
    m_root->setFrame({0, top, client.right / scale(), client.bottom / scale() - top});
    layoutChanged();
}

void Window::render()
{
    if (m_surface && m_root)
        m_surface->render([this](Canvas &canvas) { m_root->paintTree(canvas); });
}

void Window::pointerMoved(Point point)
{
    View *target = viewAt(point);
    // While a view is pressed, only it can be hovered.
    setHovered(m_pressed && target != m_pressed ? nullptr : target);
    if (m_pressed)
        m_pressed->mouseDragged(m_pressed->mapFromWindow(point));
}

void Window::pointerPressed(Point point)
{
    View *target = viewAt(point);
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
    pointerMoved(point);
    invalidate();
}

void Window::cancelPress()
{
    if (!m_pressed)
        return;
    std::exchange(m_pressed, nullptr)->m_pressed = false;
    invalidate();
}

void Window::setHovered(View *view)
{
    if (view == m_hovered)
        return;
    if (m_hovered)
        m_hovered->m_hovered = false;
    m_hovered = view;
    if (view)
        view->m_hovered = true;
    invalidate();
}

#include "player/VideoHost.h"

VideoHost::VideoHost(HWND parent)
{
    static const ATOM videoClass = [] {
        WNDCLASSW description{};
        description.lpfnWndProc = DefWindowProcW;
        description.hInstance = GetModuleHandleW(nullptr);
        description.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
        description.lpszClassName = L"SonaVideo";
        return RegisterClassW(&description);
    }();
    m_hwnd = CreateWindowExW(0, MAKEINTATOM(videoClass), nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_DISABLED,
                             0, 0, 0, 0, parent, nullptr, GetModuleHandleW(nullptr), nullptr);
}

void VideoHost::setBounds(const RECT &bounds)
{
    SetWindowPos(m_hwnd, nullptr, bounds.left, bounds.top, bounds.right - bounds.left, bounds.bottom - bounds.top,
                 SWP_NOZORDER | SWP_NOACTIVATE);
}

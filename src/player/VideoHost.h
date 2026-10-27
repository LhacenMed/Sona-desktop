#pragma once

#include <windows.h>

// The child window libVLC draws video into. It is disabled, so mouse input goes to the main
// window, whose views (composited above it) handle every click.
class VideoHost
{
public:
    explicit VideoHost(HWND parent);

    HWND hwnd() const { return m_hwnd; }
    // Places the video over `bounds`, in the parent's client pixels.
    void setBounds(const RECT &bounds);
    void setVisible(bool visible);

private:
    HWND m_hwnd;
};

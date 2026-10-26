#pragma once

#include "widgets/Stack.h"

class CaptionButton;

// Minimize, maximize (restore when maximized) and close, at Windows' own size and with Windows' own
// behavior: animations, Snap Layouts on the maximize button. For title bars drawn as views.
class WindowButtons : public Row
{
public:
    WindowButtons();

    Size preferredSize() const override;
    void layout() override;

private:
    CaptionButton &m_minimize;
    CaptionButton &m_maximize;
    CaptionButton &m_close;
};

#pragma once

#include <string>

#include "widgets/Stack.h"

class Label;
class WindowButtons;
struct BarContext;

// Figma "topbar" (247:447): the layout's leading items, the title centred on the window, the
// trailing items, then Windows' minimize, maximize and close (hidden in full screen). It's as tall as
// those buttons (Window::captionHeight); its background drags the window and the logo opens the
// system menu.
class TitleBar : public Row
{
public:
    explicit TitleBar(BarContext &context);

    void setTitle(std::wstring title);

    bool takesMouse() const override { return true; }
    HitRole hitRole() const override { return HitRole::Caption; }
    void layout() override;
    void paint(Canvas &canvas) override;

private:
    Row *m_leading;
    Label *m_titleLabel;
    Row *m_trailing;
    WindowButtons *m_windowButtons;
};

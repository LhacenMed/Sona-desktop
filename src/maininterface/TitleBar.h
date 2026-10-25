#pragma once

#include <string>

#include "core/Signal.h"
#include "widgets/Row.h"

class CaptionButton;
class Label;
class Player;

// Figma "topbar" (247:447), made one with the native caption: it is as tall as Windows' caption
// buttons (Window::captionHeight), Windows paints its background and draws the minimize, maximize
// and close buttons at its right, and its own buttons are drawn the same way (CaptionButton) in the
// system fonts. Its background drags the window; the logo opens the system menu. A preview, whose
// window has no caption buttons of its own, paints its background and pictures of the buttons.
class TitleBar : public Row
{
public:
    explicit TitleBar(Player &player, bool preview = false);

    void setTitle(std::wstring title);

    bool takesMouse() const override { return true; }
    HitRole hitRole() const override { return HitRole::Caption; }
    void layout() override;
    void paint(Canvas &canvas) override;

private:
    bool m_preview;
    // Whether Windows draws the caption under the bar right now (not in previews or full screen).
    bool m_onNativeCaption = false;
    Row *m_leading;
    Label *m_folderLabel;
    Label *m_titleLabel;
    Row *m_trailing;
    CaptionButton *m_fullScreenButton;
    View *m_nativeButtons = nullptr;
    Subscription m_playerSubscription;
};

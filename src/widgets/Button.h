#pragma once

#include <functional>
#include <string>

#include "graphics/TextStyle.h"
#include "widgets/View.h"

// The Figma button kinds: the accent-filled main action (onboarding "Next" 1902:3383, dialog "Save"
// 6216:13752), a grey secondary action ("Back", "Cancel", "Browse"), and the wide neutral button
// with an icon ("Show Advanced Settings", Button 500:3063).
enum class ButtonKind { Primary, Secondary, Neutral };

// A labelled button, optionally with a 20px icon before the label, centred together. Its size comes
// from the call site. A disabled button is drawn at half opacity and ignores the mouse.
class Button : public View
{
public:
    Button(std::wstring text, ButtonKind kind, TextStyle style, std::wstring icon = {});

    void setText(std::wstring text);
    void setEnabled(bool enabled);
    void setOnClick(std::function<void()> onClick) { m_onClick = std::move(onClick); }

    bool takesMouse() const override { return m_enabled; }
    Cursor cursor() const override { return Cursor::Hand; }
    void paint(Canvas &canvas) override;
    void mouseReleased(Point point, bool inside) override;

private:
    std::wstring m_text;
    ButtonKind m_kind;
    TextStyle m_style;
    std::wstring m_icon;
    bool m_enabled = true;
    std::function<void()> m_onClick;
};

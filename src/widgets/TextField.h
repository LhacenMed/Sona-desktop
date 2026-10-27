#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include "platform/Timer.h"
#include "widgets/View.h"

class GraphicsDevice;

// Figma's text input (6251:13904), bordered; its search field (1040:2512), darker with a magnifier;
// and the panels' value field (text_input 4046:12353), a plain well.
enum class TextFieldKind { Input, Search, Value };

// One line of editable text with a placeholder while empty: a blinking caret, selection with the
// mouse and keyboard, word moves (Ctrl), the clipboard, undo, and IME composition at the caret.
// While it has the focus it keeps the keys that type or edit, so they don't run shortcuts.
class TextField : public View
{
public:
    explicit TextField(std::wstring placeholder, TextFieldKind kind = TextFieldKind::Input);

    const std::wstring &text() const { return m_text; }
    void setText(std::wstring text);
    // Called after each edit by the user.
    void setOnChanged(std::function<void()> onChanged) { m_onChanged = std::move(onChanged); }

    bool takesMouse() const override { return true; }
    Cursor cursor() const override { return Cursor::Text; }
    bool acceptsFocus() const override { return true; }
    void focusChanged() override;
    bool keyPressed(KeyChord chord) override;
    void charTyped(wchar_t character) override;
    Rect caretBounds() const override;
    void paint(Canvas &canvas) override;
    void mousePressed(Point point) override;
    void mouseDragged(Point point) override;

private:
    struct Snapshot
    {
        std::wstring text;
        size_t caret;
    };

    const GraphicsDevice &graphics() const;
    // The horizontal span the text shows in.
    float textLeft() const;
    float textRight() const;
    float lineHeight() const;
    // The caret's offset from the start of the text, and the caret position nearest a point's x.
    float caretX(size_t index) const;
    size_t indexAt(float x) const;
    size_t previousWord(size_t from) const;
    size_t nextWord(size_t from) const;
    bool hasSelection() const { return m_caret != m_anchor; }
    std::wstring_view selection() const;
    void moveCaret(size_t index, bool extendSelection);
    // Replaces the selection (or inserts at the caret). Consecutive typing is undone in one step.
    void replaceSelection(std::wstring_view text, bool typing);
    void undo();
    void edited();
    void scrollToCaret();
    void restartBlink();

    std::wstring m_text;
    std::wstring m_placeholder;
    TextFieldKind m_kind;
    // The selection runs from the anchor to the caret; they're equal when nothing is selected.
    size_t m_caret = 0;
    size_t m_anchor = 0;
    // How far the text is scrolled left to keep the caret in view.
    float m_scroll = 0;
    std::vector<Snapshot> m_undo;
    bool m_typing = false;
    bool m_caretShown = true;
    Timer m_blink;
    std::function<void()> m_onChanged;
};

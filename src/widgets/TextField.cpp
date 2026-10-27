#include "widgets/TextField.h"

#include <algorithm>
#include <cwctype>

#include "graphics/Canvas.h"
#include "graphics/GraphicsDevice.h"
#include "platform/Clipboard.h"
#include "platform/Window.h"
#include "style/Theme.h"
#include "widgets/Icon.h"

namespace {

constexpr TextStyle textStyle{.size = 14};
// Input: 12 before the text and 18 after (6251:13904). Search: the magnifier 8 in, then 10 to the
// text (1040:2512).
constexpr float inputLeft = 12;
constexpr float inputRight = 18;
constexpr float searchIconLeft = 8;
constexpr float searchLeft = searchIconLeft + iconSize + 10;
constexpr float searchRight = 8;

} // namespace

TextField::TextField(std::wstring placeholder, TextFieldKind kind)
    : m_placeholder(std::move(placeholder))
    , m_kind(kind)
    , m_blink([this] {
        m_caretShown = !m_caretShown;
        invalidate();
    })
{
    setPreferredSize({0, kind == TextFieldKind::Search ? 34.0f : kind == TextFieldKind::Value ? 36.0f : 37.0f});
}

void TextField::setText(std::wstring text)
{
    m_text = std::move(text);
    m_caret = m_anchor = m_text.size();
    m_undo.clear();
    m_typing = false;
    m_scroll = 0;
    invalidate();
}

const GraphicsDevice &TextField::graphics() const
{
    return window()->graphics();
}

float TextField::textLeft() const
{
    return m_kind == TextFieldKind::Search ? searchLeft : inputLeft;
}

float TextField::textRight() const
{
    return frame().width - (m_kind == TextFieldKind::Search ? searchRight : inputRight);
}

float TextField::lineHeight() const
{
    return graphics().measureText(L"", textStyle).height;
}

float TextField::caretX(size_t index) const
{
    return graphics().caretX(m_text, textStyle, index);
}

size_t TextField::indexAt(float x) const
{
    return graphics().caretIndexAt(m_text, textStyle, x - textLeft() + m_scroll);
}

size_t TextField::previousWord(size_t from) const
{
    while (from > 0 && std::iswspace(m_text[from - 1]))
        --from;
    while (from > 0 && !std::iswspace(m_text[from - 1]))
        --from;
    return from;
}

size_t TextField::nextWord(size_t from) const
{
    while (from < m_text.size() && !std::iswspace(m_text[from]))
        ++from;
    while (from < m_text.size() && std::iswspace(m_text[from]))
        ++from;
    return from;
}

std::wstring_view TextField::selection() const
{
    const size_t start = std::min(m_caret, m_anchor);
    return std::wstring_view(m_text).substr(start, std::max(m_caret, m_anchor) - start);
}

void TextField::focusChanged()
{
    if (hasFocus()) {
        restartBlink();
    } else {
        m_blink.stop();
        m_typing = false;
    }
}

bool TextField::keyPressed(KeyChord chord)
{
    const size_t start = std::min(m_caret, m_anchor);
    const size_t end = std::max(m_caret, m_anchor);
    switch (chord.key) {
    case VK_LEFT:
        if (hasSelection() && !chord.shift)
            moveCaret(start, false);
        else
            moveCaret(chord.control ? previousWord(m_caret) : m_caret - (m_caret > 0), chord.shift);
        return true;
    case VK_RIGHT:
        if (hasSelection() && !chord.shift)
            moveCaret(end, false);
        else
            moveCaret(chord.control ? nextWord(m_caret) : m_caret + (m_caret < m_text.size()), chord.shift);
        return true;
    case VK_HOME:
        moveCaret(0, chord.shift);
        return true;
    case VK_END:
        moveCaret(m_text.size(), chord.shift);
        return true;
    case VK_BACK:
        if (!hasSelection())
            m_anchor = chord.control ? previousWord(m_caret) : m_caret - (m_caret > 0);
        replaceSelection(L"", false);
        return true;
    case VK_DELETE:
        if (!hasSelection())
            m_anchor = chord.control ? nextWord(m_caret) : m_caret + (m_caret < m_text.size());
        replaceSelection(L"", false);
        return true;
    }
    if (chord.control && !chord.alt) {
        switch (chord.key) {
        case 'A':
            m_anchor = 0;
            moveCaret(m_text.size(), true);
            return true;
        case 'C':
            if (hasSelection())
                setClipboardText(window()->hwnd(), selection());
            return true;
        case 'X':
            if (hasSelection()) {
                setClipboardText(window()->hwnd(), selection());
                replaceSelection(L"", false);
            }
            return true;
        case 'V': {
            // One line: line breaks in the pasted text are dropped.
            std::wstring pasted = clipboardText(window()->hwnd());
            std::erase_if(pasted, [](wchar_t character) { return character == L'\r' || character == L'\n'; });
            replaceSelection(pasted, false);
            return true;
        }
        case 'Z':
            undo();
            return true;
        }
        // Other Ctrl shortcuts (Ctrl+O) still reach the window.
        return false;
    }
    // Every other key types (as WM_CHAR) or does nothing here, so none of them runs a shortcut, except
    // the keys dialogs and the window act on.
    return chord.key != VK_ESCAPE && chord.key != VK_RETURN && chord.key != VK_TAB
        && !(chord.key >= VK_F1 && chord.key <= VK_F24);
}

void TextField::charTyped(wchar_t character)
{
    // Control characters arrive here too (Backspace, Ctrl+letters); keyPressed has handled them.
    if (character < L' ' || character == 0x7f)
        return;
    replaceSelection(std::wstring_view(&character, 1), true);
}

Rect TextField::caretBounds() const
{
    const float height = lineHeight();
    return {textLeft() + caretX(m_caret) - m_scroll, (frame().height - height) / 2, 1, height};
}

void TextField::paint(Canvas &canvas)
{
    const Palette &palette = canvas.palette();
    if (m_kind == TextFieldKind::Search) {
        canvas.fillRoundedRect(bounds(), 6, palette.bgA5);
        drawIcon(canvas, L"search", {searchIconLeft, (frame().height - iconSize) / 2}, palette.fgA2);
    } else {
        canvas.fillRoundedRect(bounds(), 6, palette.bgB2);
        // A 2px inside border.
        if (m_kind == TextFieldKind::Input)
            canvas.strokeRoundedRect({1, 1, frame().width - 2, frame().height - 2}, 5, 2, palette.bgA4);
    }

    const Rect box{textLeft(), 0, textRight() - textLeft(), frame().height};
    const float origin = box.x - m_scroll;
    const float height = lineHeight();
    const float lineTop = (frame().height - height) / 2;
    canvas.pushClip(box);
    if (hasFocus() && hasSelection()) {
        const float from = caretX(std::min(m_caret, m_anchor));
        const float to = caretX(std::max(m_caret, m_anchor));
        canvas.fillRect({origin + from, lineTop, to - from, height}, palette.bgAccentBright);
    }
    if (m_text.empty())
        canvas.drawText(m_placeholder, box, textStyle, palette.fgA4);
    else
        canvas.drawText(m_text, {origin, 0, caretX(m_text.size()) + 1, frame().height}, textStyle, palette.fgA2);
    // In a popup, which never takes activation, while the window it opened over is active.
    Window *host = window();
    if (hasFocus() && host && host->rootWindow().isActive() && m_caretShown)
        canvas.fillRect({origin + caretX(m_caret), lineTop, 1, height}, palette.fgA1);
    canvas.popClip();
}

void TextField::mousePressed(Point point)
{
    moveCaret(indexAt(point.x), GetKeyState(VK_SHIFT) < 0);
}

void TextField::mouseDragged(Point point)
{
    moveCaret(indexAt(point.x), true);
}

void TextField::moveCaret(size_t index, bool extendSelection)
{
    m_caret = index;
    if (!extendSelection)
        m_anchor = index;
    m_typing = false;
    scrollToCaret();
    restartBlink();
    invalidate();
}

void TextField::replaceSelection(std::wstring_view text, bool typing)
{
    if (text.empty() && !hasSelection())
        return;
    if (!typing || !m_typing)
        m_undo.push_back({m_text, m_caret});
    const size_t start = std::min(m_caret, m_anchor);
    m_text.replace(start, std::max(m_caret, m_anchor) - start, text);
    m_caret = m_anchor = start + text.size();
    m_typing = typing;
    edited();
}

void TextField::undo()
{
    if (m_undo.empty())
        return;
    Snapshot snapshot = std::move(m_undo.back());
    m_undo.pop_back();
    m_text = std::move(snapshot.text);
    m_caret = m_anchor = std::min(snapshot.caret, m_text.size());
    m_typing = false;
    edited();
}

void TextField::edited()
{
    scrollToCaret();
    restartBlink();
    invalidate();
    if (m_onChanged)
        m_onChanged();
}

void TextField::scrollToCaret()
{
    const float width = textRight() - textLeft();
    const float caret = caretX(m_caret);
    if (caret - m_scroll > width)
        m_scroll = caret - width;
    else if (caret < m_scroll)
        m_scroll = caret;
    // No empty room after the text once it's scrolled.
    m_scroll = std::clamp(m_scroll, 0.0f, std::max(0.0f, caretX(m_text.size()) + 1 - width));
}

void TextField::restartBlink()
{
    m_caretShown = true;
    if (hasFocus())
        m_blink.start(std::chrono::milliseconds(GetCaretBlinkTime()), true);
}

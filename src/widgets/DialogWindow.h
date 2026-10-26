#pragma once

#include <functional>
#include <string>

#include "platform/Window.h"

class Button;
class Column;

// A modal dialog over its owner (Figma "Open network stream" 6216:13614): a title bar with a close
// button, the dialog's own views, and Cancel and Save. Escape, ✕ and Cancel close it; Save runs the
// dialog's action, then closes it. Fill content(), then show it.
class DialogWindow : public Window
{
public:
    DialogWindow(GraphicsDevice &graphics, Theme &theme, HWND owner, std::wstring title, Size size);

    // Between the title bar and the buttons, laid out top to bottom.
    Column &content() const { return *m_content; }
    void setSaveEnabled(bool enabled);
    void setOnSave(std::function<void()> onSave) { m_onSave = std::move(onSave); }

private:
    bool keyPressed(KeyChord chord) override;

    Column *m_content;
    Button *m_save;
    std::function<void()> m_onSave;
};

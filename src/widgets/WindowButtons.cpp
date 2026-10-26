#include "widgets/WindowButtons.h"

#include "platform/Window.h"
#include "style/Typography.h"
#include "widgets/CaptionButton.h"

namespace {

// Through the system command, so Windows plays its minimize and maximize animations.
void runSystemCommand(const View &view, WPARAM command)
{
    PostMessageW(view.window()->hwnd(), WM_SYSCOMMAND, command, 0);
}

} // namespace

WindowButtons::WindowButtons()
    : m_minimize(add<CaptionButton>(Typography::Glyph::minimize, 46.0f, 10.0f, HitRole::Minimize))
    , m_maximize(add<CaptionButton>(Typography::Glyph::maximize, 46.0f, 10.0f, HitRole::Maximize))
    , m_close(add<CaptionButton>(Typography::Glyph::close, 46.0f, 10.0f, HitRole::Close))
{
    setFillsHeight(true);
    m_minimize.setOnClick([this] { runSystemCommand(*this, SC_MINIMIZE); });
    m_maximize.setOnClick([this] { runSystemCommand(*this, window()->isMaximized() ? SC_RESTORE : SC_MAXIMIZE); });
    m_close.setOnClick([this] { runSystemCommand(*this, SC_CLOSE); });
}

Size WindowButtons::preferredSize() const
{
    const Window *host = window();
    return {host ? 3 * host->captionButtonSize().width : 0, 0};
}

void WindowButtons::layout()
{
    // Maximizing, full screen and DPI changes resize the window, which lays the buttons out again.
    if (const Window *host = window()) {
        const float width = host->captionButtonSize().width;
        for (CaptionButton *button : {&m_minimize, &m_maximize, &m_close})
            button->setPreferredSize({width, 0});
        m_maximize.setGlyph(host->isMaximized() ? Typography::Glyph::restore : Typography::Glyph::maximize);
    }
    Row::layout();
}

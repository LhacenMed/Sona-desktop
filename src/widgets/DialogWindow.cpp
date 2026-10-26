#include "widgets/DialogWindow.h"

#include "graphics/Canvas.h"
#include "style/Theme.h"
#include "widgets/Button.h"
#include "widgets/IconButton.h"
#include "widgets/Label.h"
#include "widgets/Stack.h"

namespace {

// Figma 6216:13614: a 50px title bar, content padded 20 by 22, and a 72px footer.
constexpr float headerHeight = 50;
constexpr float footerHeight = 72;
constexpr Insets contentPadding{20, 22, 20, 22};
constexpr Size buttonSize{100, 32};
constexpr TextStyle buttonStyle{.size = 14, .weight = DWRITE_FONT_WEIGHT_SEMI_BOLD};

// The title bar, which moves the window.
class DialogHeader : public View
{
public:
    explicit DialogHeader(std::wstring title)
        : m_title(add<Label>(std::move(title), TextStyle{.size = 14, .elide = true}, &Palette::fgA2))
        , m_close(add<IconButton>(L"close", 4.0f, 4.0f))
    {
    }

    IconButton &closeButton() const { return m_close; }

    bool takesMouse() const override { return true; }
    HitRole hitRole() const override { return HitRole::Caption; }

    void layout() override
    {
        m_close.setFrame({frame().width - 41, 9, 32, 32});
        m_title.setFrame({19, 0, frame().width - 19 - 50, frame().height});
    }

    void paint(Canvas &canvas) override { canvas.fillRect(bounds(), canvas.palette().bgB2); }

private:
    Label &m_title;
    IconButton &m_close;
};

class DialogFrame : public View
{
public:
    explicit DialogFrame(std::wstring title)
        : header(add<DialogHeader>(std::move(title)))
        , content(add<Column>(20.0f))
        , cancel(add<Button>(L"Cancel", ButtonKind::Secondary, buttonStyle))
        , save(add<Button>(L"Save", ButtonKind::Primary, buttonStyle))
    {
        content.setAlignment(Align::Start);
    }

    void layout() override
    {
        const float width = frame().width;
        const float height = frame().height;
        header.setFrame({0, 0, width, headerHeight});
        content.setFrame({contentPadding.left, headerHeight + contentPadding.top, width - contentPadding.left - contentPadding.right,
                          height - headerHeight - footerHeight - contentPadding.top - contentPadding.bottom});
        const float buttonY = height - footerHeight + 20;
        cancel.setFrame({20, buttonY, buttonSize.width, buttonSize.height});
        save.setFrame({width - 20 - buttonSize.width, buttonY, buttonSize.width, buttonSize.height});
    }

    void paint(Canvas &canvas) override { canvas.fillRect(bounds(), canvas.palette().bgA4); }

    DialogHeader &header;
    Column &content;
    Button &cancel;
    Button &save;
};

} // namespace

DialogWindow::DialogWindow(GraphicsDevice &graphics, Theme &theme, HWND owner, std::wstring title, Size size)
    : Window(graphics, theme)
{
    create(title, size, size, WindowFrame::Fixed, owner);
    auto frame = std::make_unique<DialogFrame>(std::move(title));
    m_content = &frame->content;
    m_save = &frame->save;
    frame->header.closeButton().setOnClick([this] { close(); });
    frame->cancel.setOnClick([this] { close(); });
    frame->save.setOnClick([this] {
        if (m_onSave)
            m_onSave();
        close();
    });
    setRoot(std::move(frame));
}

void DialogWindow::setSaveEnabled(bool enabled)
{
    m_save->setEnabled(enabled);
}

bool DialogWindow::keyPressed(KeyChord chord)
{
    if (chord.key != VK_ESCAPE)
        return false;
    close();
    return true;
}

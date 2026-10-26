#include "onboarding/WelcomePage.h"

#include <cwchar>
#include <string>

#include "graphics/Canvas.h"
#include "graphics/GraphicsDevice.h"
#include "platform/FileDialogs.h"
#include "platform/KnownFolders.h"
#include "platform/Window.h"
#include "settings/AppSettings.h"
#include "style/Theme.h"
#include "widgets/Label.h"
#include "widgets/Button.h"

namespace {

constexpr TextStyle titleStyle{.size = 32, .weight = DWRITE_FONT_WEIGHT_LIGHT, .align = TextAlign::Center};
constexpr TextStyle descriptionStyle{.size = 16, .weight = DWRITE_FONT_WEIGHT_LIGHT, .wrap = true, .lineHeight = 28};
constexpr TextStyle promptStyle{.size = 14};
constexpr float contentX = 248;
constexpr float contentWidth = 595;

// A path, with the home folder abbreviated to "~" as the design shows it.
std::wstring displayPath(const std::wstring &path)
{
    const std::wstring home = knownFolderPath(FOLDERID_Profile);
    if (!home.empty() && path.size() >= home.size() && _wcsnicmp(path.c_str(), home.c_str(), home.size()) == 0)
        return L"~" + path.substr(home.size());
    return path;
}

// The read-only box that shows the chosen folder.
class FolderField : public View
{
public:
    FolderField() : path(add<Label>(L"", TextStyle{.size = 16}, &Palette::fg2))
    {
        path.setElidesMiddle(true);
        path.setPadding(12, 18);
    }

    void layout() override { path.setFrame(bounds()); }

    void paint(Canvas &canvas) override
    {
        const Palette &palette = canvas.palette();
        canvas.fillRoundedRect(bounds(), 6, palette.bgB2);
        // A 2px inside border.
        canvas.strokeRoundedRect({1, 1, frame().width - 2, frame().height - 2}, 5, 2, palette.bgA4);
    }

    Label &path;
};

} // namespace

WelcomePage::WelcomePage(SettingsStore &settings)
    : m_settings(settings)
{
    m_title = &add<Label>(L"Welcome to " SONA_APP_NAME L"!", titleStyle, &Palette::fg1);
    m_description = &add<Label>(SONA_APP_NAME L" is a media player that plays most video and audio files.",
                                descriptionStyle, &Palette::fgA1);
    m_prompt = &add<Label>(L"Choose a Media Folder to get started with", promptStyle, &Palette::fgA2);
    auto &folderField = add<FolderField>();
    m_folderField = &folderField;
    m_folderPath = &folderField.path;
    m_browse = &add<Button>(L"Browse", ButtonKind::Secondary, TextStyle{.size = 16});

    m_folderPath->setText(displayPath(settings.get(AppSettings::mediaFolder)));
    m_browse->setOnClick([this] {
        if (const auto folder = chooseFolder(window()->hwnd(), m_settings.get(AppSettings::mediaFolder)))
            m_settings.set(AppSettings::mediaFolder, *folder);
    });
    m_settingsSubscription = settings.changed.connect(
        [this] { m_folderPath->setText(displayPath(m_settings.get(AppSettings::mediaFolder))); });
}

void WelcomePage::layout()
{
    const Window *host = window();
    if (!host)
        return;
    const GraphicsDevice &graphics = host->graphics();
    m_title->setFrame({0, 286, frame().width, graphics.measureText(m_title->text(), titleStyle).height});
    m_description->setFrame({contentX, 362, contentWidth,
                             graphics.measureText(m_description->text(), descriptionStyle, contentWidth).height});
    const float promptHeight = graphics.measureText(m_prompt->text(), promptStyle).height;
    m_prompt->setFrame({contentX, 500, contentWidth, promptHeight});
    // The field and button share a row 16px below the prompt; the 35px button is centred on the 36px field.
    const float rowY = 500 + promptHeight + 16;
    m_folderField->setFrame({contentX, rowY, contentWidth - 10 - 114, 36});
    m_browse->setFrame({contentX + contentWidth - 114, rowY + 0.5f, 114, 35});
}

void WelcomePage::paint(Canvas &canvas)
{
    canvas.drawSvg(L"logo/sona-logo.svg", {(frame().width - 141) / 2 + 0.5f, 116, 141, 141});
}

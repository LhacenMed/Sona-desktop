#include "maininterface/MainWindow.h"

#include <shellapi.h>

#include <algorithm>

#include "graphics/Canvas.h"
#include "maininterface/TitleBar.h"
#include "onboarding/OnboardingWindow.h"
#include "platform/FileDialogs.h"
#include "playback/Player.h"
#include "player/ControlBar.h"
#include "player/VideoHost.h"
#include "player/VideoView.h"
#include "settings/AppSettings.h"
#include "style/Theme.h"

namespace {

constexpr UINT onboardingClosedMessage = WM_APP + 1;

// The player layout: video between the title bar and the control bar. The video is added first so
// the bars paint over the outer half of its rounded frame.
class MainView : public View
{
public:
    explicit MainView(Player &player)
        : video(add<VideoView>(player))
        , titleBar(add<TitleBar>(player))
        , controlBar(add<ControlBar>(player))
    {
    }

    void layout() override
    {
        const float width = frame().width;
        const float height = frame().height;
        // The title bar is as tall as Windows' caption buttons, which change with the window state.
        const float titleHeight = window() ? window()->captionHeight() : 0;
        titleBar.setFrame({0, 0, width, titleHeight});
        controlBar.setFrame({0, height - 54, width, 54});
        video.setFrame({10, titleHeight + 1, width - 20, std::max(0.0f, height - 54 - titleHeight - 1)});
    }

    void paint(Canvas &canvas) override { canvas.fillRect(bounds(), canvas.palette().bgA3); }

    VideoView &video;
    TitleBar &titleBar;
    ControlBar &controlBar;
};

} // namespace

MainWindow::MainWindow(GraphicsDevice &graphics, Theme &theme, SettingsStore &settings, Player &player)
    : Window(graphics, theme)
    , m_settings(settings)
    , m_player(player)
    , m_shortcuts(makeKeyboardShortcuts(player, [this] { toggleFullScreen(); }))
{
    create(SONA_APP_NAME, {1280, 720}, {640, 360}, true, nullptr);
    m_videoHost = std::make_unique<VideoHost>(hwnd());
    player.setVideoWindow(m_videoHost->hwnd());

    auto view = std::make_unique<MainView>(player);
    m_titleBar = &view->titleBar;
    m_videoView = &view->video;
    setRoot(std::move(view));
    setNativeCaption(&Palette::bgA3);
    updateTitle();

    DragAcceptFiles(hwnd(), TRUE);
    m_playerSubscription = player.changed.connect([this] { updateTitle(); });
}

MainWindow::~MainWindow() = default;

void MainWindow::showOnboarding()
{
    EnableWindow(hwnd(), FALSE);
    m_onboarding = std::make_unique<OnboardingWindow>(graphics(), theme(), m_settings, m_player, m_shortcuts, hwnd(), [this] {
        // Re-enabled before the onboarding window goes, so Windows activates this one next.
        EnableWindow(hwnd(), TRUE);
        PostMessageW(hwnd(), onboardingClosedMessage, 0, 0);
    });
}

void MainWindow::layoutChanged()
{
    if (!m_videoHost || !m_videoView)
        return;
    m_videoHost->setBounds(contentToClientPixels(m_videoView->frame()));
}

bool MainWindow::keyPressed(UINT key, bool shift, bool control)
{
    if (control && !shift && key == 'O') {
        openFile();
        return true;
    }
    if (key == VK_ESCAPE && isFullScreen()) {
        toggleFullScreen();
        return true;
    }
    return !control && runShortcut(m_shortcuts, {key, shift});
}

void MainWindow::filesDropped(HDROP drop)
{
    std::wstring path(DragQueryFileW(drop, 0, nullptr, 0), L'\0');
    DragQueryFileW(drop, 0, path.data(), static_cast<UINT>(path.size() + 1));
    DragFinish(drop);
    m_player.open(path);
}

bool MainWindow::messageReceived(UINT message, WPARAM, LPARAM)
{
    if (message != onboardingClosedMessage)
        return false;
    m_onboarding.reset();
    return true;
}

void MainWindow::closeRequested()
{
    // Hidden at once so closing feels instant; destroyed once libVLC has let go of the video window.
    ShowWindow(hwnd(), SW_HIDE);
    m_player.stop([this] { DestroyWindow(hwnd()); });
}

void MainWindow::destroyed()
{
    PostQuitMessage(0);
}

void MainWindow::openFile()
{
    if (const auto path = chooseFile(hwnd(), m_settings.get(AppSettings::mediaFolder)))
        m_player.open(*path);
}

void MainWindow::updateTitle()
{
    const std::wstring title = m_player.title().empty() ? SONA_APP_NAME : m_player.title();
    if (title == m_title)
        return;
    m_title = title;
    SetWindowTextW(hwnd(), title.c_str());
    m_titleBar->setTitle(title);
}

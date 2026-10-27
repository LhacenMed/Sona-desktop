#include "maininterface/MainWindow.h"

#include <shellapi.h>

#include "library/LibraryItems.h"
#include "library/LibraryView.h"
#include "maininterface/MainCommands.h"
#include "maininterface/MainMenus.h"
#include "onboarding/OnboardingWindow.h"
#include "platform/FileDialogs.h"
#include "platform/PopupWindow.h"
#include "playback/MediaFiles.h"
#include "playback/Player.h"
#include "playback/Queue.h"
#include "player/PlayerView.h"
#include "player/TitleBar.h"
#include "player/VideoHost.h"
#include "player/VideoView.h"
#include "settings/AppSettings.h"
#include "widgets/Overlay.h"

namespace {

// Figma's menu widths (6062:13354, 6065:13616).
constexpr float mainMenuWidth = 375;
constexpr float contextMenuWidth = 292;

// The window's content: the player and the library, each as large as the window; one shows.
class Screens : public View
{
public:
    void layout() override
    {
        for (const auto &screen : children())
            screen->setFrame(bounds());
    }
};

} // namespace

MainWindow::MainWindow(GraphicsDevice &graphics, Theme &theme, SettingsStore &settings, Player &player, Queue &queue,
                       Bookmarks &bookmarks, History &history, Thumbnails &thumbnails)
    : Window(graphics, theme)
    , m_settings(settings)
    , m_player(player)
    , m_queue(queue)
{
    defineMainCommands(m_commands, *this, player, queue, bookmarks);
    create(SONA_APP_NAME, {1280, 720}, {640, 360}, WindowFrame::Resizable, nullptr);
    m_videoHost = std::make_unique<VideoHost>(hwnd());
    player.setVideoWindow(m_videoHost->hwnd());

    const BarContext barContext{player, bookmarks, settings, m_commands, [this](View &button) {
        Menu::open(*this, button.mapToWindow(button.bounds()), PopupPlacement::Below, mainMenuItems(), m_commands, mainMenuWidth);
    }};
    auto screens = std::make_unique<Screens>();
    m_playerView = &screens->add<PlayerView>(barContext, queue);
    m_libraryView = &screens->add<LibraryView>(
        barContext, LibraryContext{history, thumbnails, settings, m_commands,
                                   [this](const LibraryItem &item) { openMedia({item.location}, item.resumeAt); }});
    setRoot(std::move(screens));
    updateScreen();

    DragAcceptFiles(hwnd(), TRUE);
    m_playerSubscription = player.changed.connect([this] { updateTitle(); });
    m_queueSubscription = queue.changed.connect([this] { updateScreen(); });
}

MainWindow::~MainWindow() = default;

void MainWindow::showOnboarding()
{
    openWindow<OnboardingWindow>(graphics(), theme(), m_playerView->barContext(), hwnd());
}

void MainWindow::openMedia(const std::vector<std::wstring> &opened, double startTime)
{
    setBarsAutoHide(true);
    m_queue.open(opened, startTime);
}

bool MainWindow::barsAutoHide() const
{
    return m_playerView->barsAutoHide();
}

void MainWindow::setBarsAutoHide(bool autoHide)
{
    m_playerView->setBarsAutoHide(autoHide);
    // The video moves between beside the bars and under them.
    layoutChanged();
}

bool MainWindow::isQueueOpen() const
{
    return m_playerView->isQueueOpen();
}

void MainWindow::toggleQueue()
{
    m_playerView->toggleQueue();
}

bool MainWindow::isLibraryShown() const
{
    return m_libraryView->isVisible();
}

void MainWindow::focusLibrarySearch()
{
    m_libraryView->focusSearch();
}

void MainWindow::updateScreen()
{
    const bool playing = m_queue.current() >= 0;
    if (playing == m_playerView->isVisible() && playing != m_libraryView->isVisible()) {
        updateTitle();
        return;
    }
    // A panel or menu belongs to the screen it opened over.
    closePopup();
    m_playerView->setVisible(playing);
    m_libraryView->setVisible(!playing);
    m_videoHost->setVisible(playing);
    updateTitle();
    invalidate();
}

void MainWindow::layoutChanged()
{
    if (!m_videoHost || !m_playerView)
        return;
    m_videoHost->setBounds(contentToClientPixels(m_playerView->video().frame()));
}

bool MainWindow::keyPressed(KeyChord chord)
{
    const std::optional<CommandId> command = m_commands.commandFor(chord);
    if (!command)
        return false;
    // Named before it runs, for the state it changes (Pause while playing).
    const std::wstring message = m_commands.label(*command) + L"  ·  " + shortcutText(chord);
    m_commands.run(*command);
    overlay().showMessage(message);
    return true;
}

void MainWindow::pointerChanged()
{
    if (m_playerView)
        m_playerView->pointerChanged();
}

void MainWindow::contextMenuRequested(Point point)
{
    Menu::open(*this, {point.x, point.y, 0, 0}, PopupPlacement::Beside, contextMenuItems(), m_commands, contextMenuWidth);
}

void MainWindow::filesDropped(HDROP drop)
{
    std::vector<std::wstring> paths(DragQueryFileW(drop, 0xFFFFFFFF, nullptr, 0));
    for (UINT i = 0; i < paths.size(); ++i) {
        paths[i].resize(DragQueryFileW(drop, i, nullptr, 0));
        DragQueryFileW(drop, i, paths[i].data(), static_cast<UINT>(paths[i].size() + 1));
    }
    DragFinish(drop);
    if (isQueueOpen())
        m_queue.append(paths);
    else
        openMedia(paths);
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

void MainWindow::openFiles()
{
    chooseFiles(*this, m_settings.get(AppSettings::mediaFolder), mediaFileType(),
                [this](std::vector<std::wstring> files) { openMedia(files); });
}

void MainWindow::openFolder()
{
    chooseFolder(*this, m_settings.get(AppSettings::mediaFolder), [this](std::wstring folder) { openMedia({folder}); });
}

void MainWindow::updateTitle()
{
    const std::wstring title = m_queue.current() < 0 || m_player.title().empty() ? SONA_APP_NAME : m_player.title();
    if (title == m_title)
        return;
    m_title = title;
    SetWindowTextW(hwnd(), title.c_str());
    m_playerView->titleBar().setTitle(title);
}

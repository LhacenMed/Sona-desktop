#pragma once

#include <memory>
#include <string>
#include <vector>

#include "commands/CommandRegistry.h"
#include "core/Signal.h"
#include "platform/Window.h"

class Bookmarks;
class History;
class LibraryView;
class Player;
class PlayerView;
class Queue;
class SettingsStore;
class Thumbnails;
class VideoHost;

// Figma "Player" (247:445): the video, with the title bar and controls over it, while something
// plays; otherwise the library (Figma Home 6261:15078), which stopping returns to.
class MainWindow : public Window
{
public:
    MainWindow(GraphicsDevice &graphics, Theme &theme, SettingsStore &settings, Player &player, Queue &queue,
               Bookmarks &bookmarks, History &history, Thumbnails &thumbnails);
    ~MainWindow() override;

    // First-run onboarding, modal over this window.
    void showOnboarding();
    // Plays the media in `opened` in place of the queue, from `startTime` seconds in, the bars
    // auto-hiding over it.
    void openMedia(const std::vector<std::wstring> &opened, double startTime = 0);
    // Plays the chosen files, or the media in the chosen folder, in place of the queue.
    void openFiles();
    void openFolder();
    // Whether the bars auto-hide over the video, or stay fixed beside it (PlayerView).
    bool barsAutoHide() const;
    void setBarsAutoHide(bool autoHide);
    bool isQueueOpen() const;
    void toggleQueue();
    bool isLibraryShown() const;
    void focusLibrarySearch();

private:
    void layoutChanged() override;
    // A shortcut runs its command and says so on screen: its name and its keys.
    bool keyPressed(KeyChord chord) override;
    void pointerChanged() override;
    void contextMenuRequested(Point point) override;
    // Dropped files and folders play in place of the queue, or join it while it's open.
    void filesDropped(HDROP drop) override;
    void closeRequested() override;
    void destroyed() override;

    void updateTitle();
    // The player while the queue plays, the library otherwise.
    void updateScreen();

    SettingsStore &m_settings;
    Player &m_player;
    Queue &m_queue;
    CommandRegistry m_commands;
    std::unique_ptr<VideoHost> m_videoHost;
    PlayerView *m_playerView = nullptr;
    LibraryView *m_libraryView = nullptr;
    std::wstring m_title;
    Subscription m_playerSubscription;
    Subscription m_queueSubscription;
};

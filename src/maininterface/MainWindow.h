#pragma once

#include <memory>
#include <string>

#include "commands/CommandRegistry.h"
#include "core/Signal.h"
#include "platform/Window.h"

class Bookmarks;
class Player;
class PlayerView;
class Queue;
class SettingsStore;
class VideoHost;

// Figma "Player" (247:445): the video, with the title bar and controls over it.
class MainWindow : public Window
{
public:
    MainWindow(GraphicsDevice &graphics, Theme &theme, SettingsStore &settings, Player &player, Queue &queue,
               Bookmarks &bookmarks);
    ~MainWindow() override;

    // First-run onboarding, modal over this window.
    void showOnboarding();
    // Plays the chosen files, or the media in the chosen folder, in place of the queue.
    void openFiles();
    void openFolder();
    // Whether the bars auto-hide over the video, or stay fixed beside it (PlayerView).
    bool barsAutoHide() const;
    void setBarsAutoHide(bool autoHide);
    bool isQueueOpen() const;
    void toggleQueue();

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

    SettingsStore &m_settings;
    Player &m_player;
    Queue &m_queue;
    CommandRegistry m_commands;
    std::unique_ptr<VideoHost> m_videoHost;
    PlayerView *m_playerView = nullptr;
    std::wstring m_title;
    Subscription m_playerSubscription;
};

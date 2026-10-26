#pragma once

#include <memory>
#include <string>

#include "commands/CommandRegistry.h"
#include "core/Signal.h"
#include "platform/Window.h"

class Player;
class PlayerView;
class SettingsStore;
class VideoHost;

// Figma "Player" (247:445): the video, with the title bar and controls over it.
class MainWindow : public Window
{
public:
    MainWindow(GraphicsDevice &graphics, Theme &theme, SettingsStore &settings, Player &player);
    ~MainWindow() override;

    // First-run onboarding, modal over this window.
    void showOnboarding();
    void openFile();
    // Whether the bars auto-hide over the video, or stay fixed beside it (PlayerView).
    bool barsAutoHide() const;
    void setBarsAutoHide(bool autoHide);

private:
    void layoutChanged() override;
    // A shortcut runs its command and says so on screen: its name and its keys.
    bool keyPressed(KeyChord chord) override;
    void pointerChanged() override;
    void contextMenuRequested(Point point) override;
    void filesDropped(HDROP drop) override;
    void closeRequested() override;
    void destroyed() override;

    void updateTitle();

    SettingsStore &m_settings;
    Player &m_player;
    CommandRegistry m_commands;
    std::unique_ptr<VideoHost> m_videoHost;
    PlayerView *m_playerView = nullptr;
    std::wstring m_title;
    Subscription m_playerSubscription;
};

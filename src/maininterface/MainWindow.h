#pragma once

#include <memory>
#include <string>

#include "commands/CommandRegistry.h"
#include "core/Signal.h"
#include "platform/Window.h"

class Player;
class SettingsStore;
class TitleBar;
class VideoHost;
class View;

// Figma "Player" (247:445): the title bar, video and controls.
class MainWindow : public Window
{
public:
    MainWindow(GraphicsDevice &graphics, Theme &theme, SettingsStore &settings, Player &player);
    ~MainWindow() override;

    // First-run onboarding, modal over this window.
    void showOnboarding();
    void openFile();

private:
    void layoutChanged() override;
    bool keyPressed(KeyChord chord) override;
    void filesDropped(HDROP drop) override;
    void closeRequested() override;
    void destroyed() override;

    void updateTitle();

    SettingsStore &m_settings;
    Player &m_player;
    CommandRegistry m_commands;
    std::unique_ptr<VideoHost> m_videoHost;
    TitleBar *m_titleBar = nullptr;
    View *m_videoView = nullptr;
    std::wstring m_title;
    Subscription m_playerSubscription;
};

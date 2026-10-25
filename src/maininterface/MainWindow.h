#pragma once

#include <memory>
#include <string>
#include <vector>

#include "core/Signal.h"
#include "maininterface/KeyboardShortcuts.h"
#include "platform/Window.h"

class OnboardingWindow;
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

private:
    void layoutChanged() override;
    bool keyPressed(UINT key, bool shift, bool control) override;
    void filesDropped(HDROP drop) override;
    bool messageReceived(UINT message, WPARAM wParam, LPARAM lParam) override;
    void closeRequested() override;
    void destroyed() override;

    void openFile();
    void updateTitle();

    SettingsStore &m_settings;
    Player &m_player;
    std::vector<ShortcutGroup> m_shortcuts;
    std::unique_ptr<VideoHost> m_videoHost;
    TitleBar *m_titleBar = nullptr;
    View *m_videoView = nullptr;
    std::wstring m_title;
    std::unique_ptr<OnboardingWindow> m_onboarding;
    Subscription m_playerSubscription;
};

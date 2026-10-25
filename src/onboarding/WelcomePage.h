#pragma once

#include "core/Signal.h"
#include "widgets/View.h"

class Label;
class SettingsStore;
class TextButton;

// Onboarding step 1 (Figma 1899:3055): welcome and the media folder.
class WelcomePage : public View
{
public:
    explicit WelcomePage(SettingsStore &settings);

    void layout() override;
    void paint(Canvas &canvas) override;

private:
    SettingsStore &m_settings;
    Label *m_title;
    Label *m_description;
    Label *m_prompt;
    View *m_folderField;
    Label *m_folderPath;
    TextButton *m_browse;
    Subscription m_settingsSubscription;
};

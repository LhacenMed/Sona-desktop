#pragma once

#include <vector>

#include "widgets/View.h"

class CommandRegistry;
class Label;

// Onboarding step 3 (Figma 1902:6770): the shortcut guide, two columns of six, with each command's
// keys read from the registry.
class ShortcutsPage : public View
{
public:
    explicit ShortcutsPage(const CommandRegistry &commands);

    void layout() override;

private:
    Label *m_heading;
    std::vector<View *> m_rows;
    Label *m_footer;
};

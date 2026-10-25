#pragma once

#include <vector>

#include "maininterface/KeyboardShortcuts.h"
#include "widgets/View.h"

class Label;

// Onboarding step 3 (Figma 1902:6770): the shortcut guide, two columns of six.
class ShortcutsPage : public View
{
public:
    explicit ShortcutsPage(const std::vector<ShortcutGroup> &groups);

    void layout() override;

private:
    Label *m_heading;
    std::vector<View *> m_rows;
    Label *m_footer;
};

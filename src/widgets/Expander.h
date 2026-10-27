#pragma once

#include <functional>

#include "widgets/Stack.h"

// Figma's filter groups ("Video - Essential" 536:1855 and the others): a well titled "Expand
// Settings" that a click opens on its settings, retitled "Collapse Settings". 48 tall while closed.
class Expander : public Column
{
public:
    Expander();

    // Where its settings go, 16 in from its sides.
    Column &content() const { return *m_content; }
    bool isExpanded() const { return m_content->isVisible(); }
    void setExpanded(bool expanded);
    // A click opened or closed it, which changes its height.
    void setOnToggled(std::function<void()> onToggled) { m_onToggled = std::move(onToggled); }

    void paint(Canvas &canvas) override;

private:
    class Header;

    Column *m_content;
    std::function<void()> m_onToggled;
};

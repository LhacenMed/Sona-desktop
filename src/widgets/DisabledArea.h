#pragma once

#include <string>
#include <utility>

#include "widgets/View.h"

// Shows its content dimmed and inert: the content takes no clicks, and resting the pointer on it
// says why (its tooltip). Used for settings that can't work yet, drawn as their frames show them.
class DisabledArea : public View
{
public:
    explicit DisabledArea(std::wstring reason) { setTooltip(std::move(reason)); }

    template <typename T, typename... Args>
    T &setContent(Args &&...args)
    {
        T &content = add<T>(std::forward<Args>(args)...);
        m_content = &content;
        return content;
    }

    Size preferredSize() const override { return m_content->preferredSize(); }
    void layout() override { m_content->setFrame(bounds()); }
    View *hitTest(Point point) override { return isVisible() && bounds().contains(point) ? this : nullptr; }
    bool takesMouse() const override { return true; }
    void paintTree(Canvas &canvas) override;

private:
    View *m_content = nullptr;
};

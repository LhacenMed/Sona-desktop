#include "onboarding/ShortcutsPage.h"

#include "onboarding/KeyCap.h"
#include "style/Theme.h"
#include "widgets/Label.h"
#include "widgets/Row.h"

namespace {

constexpr TextStyle guideStyle{.size = 16, .weight = DWRITE_FONT_WEIGHT_LIGHT};

// A shortcut's name and its keys, with "/" between alternative bindings.
class ShortcutRow : public View
{
public:
    explicit ShortcutRow(const ShortcutGroup &group)
        : m_title(add<Label>(group.title, guideStyle, &Palette::fgA1))
        , m_bindings(add<Row>(10.0f))
    {
        // Pairs of arrow and punctuation keys use narrower caps.
        const bool compact = group.bindings.size() > 1;
        for (size_t i = 0; i < group.bindings.size(); ++i) {
            if (i > 0)
                m_bindings.add<Label>(L"/", guideStyle, &Palette::fgA1);
            auto &keys = m_bindings.add<Row>(4.0f);
            for (const std::wstring &label : group.bindings[i].keyLabels)
                keys.add<KeyCap>(label, compact);
        }
    }

    void layout() override
    {
        m_title.setFrame({0, 0, 220, frame().height});
        m_bindings.setFrame({240, 0, m_bindings.preferredSize().width, frame().height});
    }

private:
    Label &m_title;
    Row &m_bindings;
};

} // namespace

ShortcutsPage::ShortcutsPage(const std::vector<ShortcutGroup> &groups)
{
    m_heading = &add<Label>(L"Learn the basic keyboard shortcuts.", TextStyle{.size = 24, .align = TextAlign::Center},
                            &Palette::fgA1);
    for (const ShortcutGroup &group : groups)
        m_rows.push_back(&add<ShortcutRow>(group));
    m_footer = &add<Label>(L"Find the full list in the menu.", TextStyle{.size = 16, .lineHeight = 28}, &Palette::fgA3);
}

void ShortcutsPage::layout()
{
    m_heading->setFrame({0, 60, frame().width, m_heading->preferredSize().height});
    for (size_t i = 0; i < m_rows.size(); ++i)
        m_rows[i]->setFrame({i < 6 ? 62.0f : 605.0f, 154 + 82.0f * static_cast<float>(i % 6), 393, 42});
    m_footer->setFrame({62, 671, 400, 28});
}

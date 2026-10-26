#include "onboarding/ShortcutsPage.h"

#include <array>
#include <string>

#include "commands/CommandRegistry.h"
#include "onboarding/KeyCap.h"
#include "style/Theme.h"
#include "widgets/Label.h"
#include "widgets/Stack.h"

namespace {

constexpr TextStyle guideStyle{.size = 16, .weight = DWRITE_FONT_WEIGHT_LIGHT};

// A line of the guide: its title and the one or two commands whose keys it shows.
struct GuideLine
{
    const wchar_t *title;
    std::vector<CommandId> commands;
};

const std::array<GuideLine, 12> guideLines{{
    {L"Play / Pause", {CommandId::PlayPause}},
    {L"Seek Backward / Forward", {CommandId::SeekBackward, CommandId::SeekForward}},
    {L"Previous / Next Item", {CommandId::PreviousItem, CommandId::NextItem}},
    {L"Previous / Next Frame", {CommandId::PreviousFrame, CommandId::NextFrame}},
    {L"Cycle Subtitle Track", {CommandId::CycleSubtitleTrack}},
    {L"Cycle Audio Track", {CommandId::CycleAudioTrack}},
    {L"Toggle Fullscreen", {CommandId::ToggleFullScreen}},
    {L"Increase / Decrease Volume", {CommandId::VolumeUp, CommandId::VolumeDown}},
    {L"Show / Hide Queue", {CommandId::ToggleQueue}},
    {L"Show / Hide File Browser", {CommandId::ToggleFileBrowser}},
    {L"Take a Snapshot", {CommandId::TakeSnapshot}},
    {L"Record", {CommandId::Record}},
}};

// A line's title and each command's keys, with "/" between the commands.
class ShortcutRow : public View
{
public:
    ShortcutRow(const GuideLine &line, const CommandRegistry &commands)
        : m_title(add<Label>(line.title, guideStyle, &Palette::fgA1))
        , m_bindings(add<Row>(10.0f))
    {
        // Pairs of keys use narrower caps.
        const bool compact = line.commands.size() > 1;
        for (size_t i = 0; i < line.commands.size(); ++i) {
            if (i > 0)
                m_bindings.add<Label>(L"/", guideStyle, &Palette::fgA1);
            auto &keys = m_bindings.add<Row>(4.0f);
            for (const std::wstring &name : keyNames(commands.command(line.commands[i]).shortcuts.front()))
                keys.add<KeyCap>(name, compact);
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

ShortcutsPage::ShortcutsPage(const CommandRegistry &commands)
{
    m_heading = &add<Label>(L"Learn the basic keyboard shortcuts.", TextStyle{.size = 24, .align = TextAlign::Center},
                            &Palette::fgA1);
    for (const GuideLine &line : guideLines)
        m_rows.push_back(&add<ShortcutRow>(line, commands));
    m_footer = &add<Label>(L"Find the full list in the menu.", TextStyle{.size = 16, .lineHeight = 28}, &Palette::fgA3);
}

void ShortcutsPage::layout()
{
    m_heading->setFrame({0, 60, frame().width, m_heading->preferredSize().height});
    for (size_t i = 0; i < m_rows.size(); ++i)
        m_rows[i]->setFrame({i < 6 ? 62.0f : 605.0f, 154 + 82.0f * static_cast<float>(i % 6), 393, 42});
    m_footer->setFrame({62, 671, 400, 28});
}

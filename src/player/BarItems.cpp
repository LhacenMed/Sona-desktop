#include "player/BarItems.h"

#include <algorithm>
#include <string_view>

#include "commands/CommandRegistry.h"
#include "graphics/Canvas.h"
#include "platform/Window.h"
#include "playback/Player.h"
#include "player/PlayButton.h"
#include "player/SeekBar.h"
#include "player/TimeLabel.h"
#include "player/VolumeBar.h"
#include "settings/AppSettings.h"
#include "style/Theme.h"
#include "style/Typography.h"
#include "widgets/CaptionButton.h"
#include "widgets/IconButton.h"
#include "widgets/Label.h"
#include "widgets/Stack.h"

namespace {

// Title bar text, as Windows sets the window title.
constexpr TextStyle captionText{.size = 12, .elide = true, .font = TextFont::System};
// The title bar's own buttons: narrower than the 46px caption buttons, with slightly larger glyphs,
// as toolbar icons in native title bars are.
constexpr float toolButtonWidth = 40;
constexpr float toolGlyphSize = 12;

template <typename Button>
Button &bindCommand(Button &button, CommandId id, const CommandRegistry &commands)
{
    const Command &command = commands.command(id);
    button.setEnabled(static_cast<bool>(command.run));
    button.setTooltip(command.shortcuts.empty() ? commands.label(id)
                                                : commands.label(id) + L" (" + shortcutText(command.shortcuts.front()) + L")");
    button.setOnClick([&commands, id] { commands.run(id); });
    return button;
}

// A button that opens a panel: while one is open, a click on it closes the panel or swaps it for its
// own, and it shows its active state while its panel is open.
class PanelButton : public IconWrapper
{
public:
    PanelButton(CommandId id, const CommandRegistry &commands)
        : IconWrapper(commands.command(id).icon)
    {
        setActiveWhen([&commands, id] { return commands.isActive(id); });
        bindCommand(*this, id, commands);
    }

    bool actsOnPopups() const override { return true; }
};

// The app logo at the size of a native window icon; it opens the system menu like one.
class WindowIcon : public View
{
public:
    WindowIcon()
    {
        setPreferredSize({36, 0});
        setFillsHeight(true);
    }

    bool takesMouse() const override { return true; }
    HitRole hitRole() const override { return HitRole::SystemMenu; }
    void paint(Canvas &canvas) override
    {
        constexpr float size = 16;
        canvas.drawSvg(L"logo/sona-logo.svg", {(frame().width - size) / 2, (frame().height - size) / 2, size, size});
    }
};

// A thin vertical rule between button groups.
class Divider : public View
{
public:
    Divider()
    {
        setPreferredSize({9, 0});
        setFillsHeight(true);
    }

    void paint(Canvas &canvas) override
    {
        constexpr float height = 16;
        canvas.fillRect({4, (frame().height - height) / 2, 1, height}, canvas.palette().dividerBright);
    }
};

// The playing file's folder, after a folder glyph.
class FolderName : public Row
{
public:
    explicit FolderName(Player &player)
    {
        setFillsWidth(true);
        setFillsHeight(true);
        add<Glyph>().setFillsHeight(true);
        m_label = &add<Label>(player.folderName(), captionText, &Palette::fgA2);
        m_label->setFillsWidth(true);
        m_label->setFillsHeight(true);
        m_label->setPadding(2, 8);
        m_playerSubscription = player.changed.connect([this, &player] { m_label->setText(player.folderName()); });
    }

private:
    class Glyph : public View
    {
    public:
        Glyph() { setPreferredSize({toolGlyphSize + 8, 0}); }

        void paint(Canvas &canvas) override
        {
            const wchar_t glyph = Typography::Glyph::folder;
            const TextStyle style{.size = toolGlyphSize, .align = TextAlign::Center, .font = TextFont::Icons};
            canvas.drawText(std::wstring_view(&glyph, 1), bounds(), style, canvas.palette().fgA2);
        }
    };

    Label *m_label;
    Subscription m_playerSubscription;
};

// Full screen and back, as wide as each of Windows' caption buttons beside it.
class FullScreenButton : public CaptionButton
{
public:
    FullScreenButton() : CaptionButton(Typography::Glyph::fullScreen) {}

    Size preferredSize() const override
    {
        const Window *host = window();
        return {host ? host->captionButtonSize().width : 0, 0};
    }

    // Entering and leaving full screen resize the window, which lays the button out again.
    void layout() override
    {
        if (const Window *host = window())
            setGlyph(host->isFullScreen() ? Typography::Glyph::exitFullScreen : Typography::Glyph::fullScreen);
    }
};

// The speaker, which mutes, and the volume cone.
class VolumeControl : public Row
{
public:
    VolumeControl(Player &player, const CommandRegistry &commands)
        : Row(8)
        , m_player(player)
    {
        m_button = &bindCommand(add<IconButton>(icon()), CommandId::ToggleMute, commands);
        add<VolumeBar>(player);
        m_playerSubscription = player.changed.connect([this] { m_button->setIconName(icon()); });
    }

private:
    const wchar_t *icon() const
    {
        if (m_player.isMuted() || m_player.volume() == 0)
            return L"volume-mute";
        return m_player.volume() < 50 ? L"volume-medium" : L"volume-high";
    }

    Player &m_player;
    IconButton *m_button;
    Subscription m_playerSubscription;
};

View &addItem(Row &group, BarItem item, BarContext &context)
{
    Player &player = context.player;
    const CommandRegistry &commands = context.commands;
    const auto commandIcon = [&commands](CommandId id) { return commands.command(id).icon; };
    switch (item) {
    case BarItem::Logo:
        return group.add<WindowIcon>();
    case BarItem::MainMenu: {
        auto &button = group.add<CaptionButton>(Typography::Glyph::more, toolButtonWidth, toolGlyphSize);
        button.setTooltip(L"Menu");
        button.setOnClick([&context, &button] { context.openMainMenu(button); });
        return button;
    }
    case BarItem::FileBrowser:
        return bindCommand(group.add<CaptionButton>(Typography::Glyph::sidebar, toolButtonWidth, toolGlyphSize),
                           CommandId::ToggleFileBrowser, commands);
    case BarItem::Divider:
        return group.add<Divider>();
    case BarItem::FolderName:
        return group.add<FolderName>(player);
    case BarItem::FullScreen:
        return bindCommand(group.add<FullScreenButton>(), CommandId::ToggleFullScreen, commands);
    case BarItem::Previous:
        return bindCommand(group.add<IconButton>(commandIcon(CommandId::PreviousItem), 3.0f, 2.0f), CommandId::PreviousItem, commands);
    case BarItem::PlayPause:
        return group.add<PlayButton>(player);
    case BarItem::Next:
        return bindCommand(group.add<IconButton>(commandIcon(CommandId::NextItem), 3.0f, 2.0f), CommandId::NextItem, commands);
    case BarItem::CurrentTime:
        return group.add<TimeLabel>([&player] { return player.position(); }, true);
    case BarItem::SeekBar: {
        auto &seekBar = group.add<SeekBar>(player, context.bookmarks);
        seekBar.setFillsWidth(true);
        return seekBar;
    }
    case BarItem::Duration: {
        // The duration, or the time left (negative) once clicked; the choice is remembered.
        SettingsStore &settings = context.settings;
        auto &duration = group.add<TimeLabel>(
            [&player, &settings] {
                return settings.get(AppSettings::showRemainingTime) ? player.position() - player.duration() : player.duration();
            },
            true);
        duration.setOnClick([&settings, &duration] {
            settings.set(AppSettings::showRemainingTime, !settings.get(AppSettings::showRemainingTime));
            duration.invalidate();
        });
        return duration;
    }
    case BarItem::Volume:
        return group.add<VolumeControl>(player, commands);
    case BarItem::AudioTracks:
        return group.add<PanelButton>(CommandId::ShowAudioTracks, commands);
    case BarItem::Subtitles:
        return group.add<PanelButton>(CommandId::ShowSubtitleTracks, commands);
    case BarItem::PlaybackSettings:
        return group.add<PanelButton>(CommandId::ShowPlaybackSettings, commands);
    case BarItem::Queue:
        return group.add<PanelButton>(CommandId::ToggleQueue, commands);
    }
    return group;
}

} // namespace

void addBarGroups(Row &bar, const std::vector<BarGroup> &groups, BarContext &context)
{
    for (const BarGroup &group : groups) {
        auto &row = bar.add<Row>(group.spacing);
        row.setFillsHeight(true);
        bool fills = false;
        for (BarItem item : group.items)
            fills = addItem(row, item, context).fillsWidth() || fills;
        row.setFillsWidth(fills);
    }
}

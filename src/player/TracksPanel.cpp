#include "player/TracksPanel.h"

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "commands/CommandRegistry.h"
#include "graphics/Canvas.h"
#include "platform/FileDialogs.h"
#include "platform/Window.h"
#include "playback/MediaFiles.h"
#include "playback/Player.h"
#include "player/BarItems.h"
#include "settings/AppSettings.h"
#include "settings/SettingsStore.h"
#include "style/Theme.h"
#include "widgets/Button.h"
#include "widgets/IconButton.h"
#include "widgets/Label.h"
#include "widgets/ListView.h"
#include "widgets/ScrollView.h"
#include "widgets/Tabs.h"

namespace {

constexpr float dividerHeight = 1;

// What a section lists, and which entry is current (-1 for none).
struct Entries
{
    std::vector<ListItem> items;
    int current = -1;

    bool operator==(const Entries &) const = default;
};

Entries trackEntries(const std::vector<Track> &tracks, int current)
{
    Entries entries;
    for (const Track &track : tracks) {
        if (track.id == current)
            entries.current = static_cast<int>(entries.items.size());
        entries.items.push_back({track.name});
    }
    return entries;
}

Entries deviceEntries(const Player &player)
{
    Entries entries;
    for (const AudioDevice &device : player.audioDevices()) {
        if (device.id == player.audioDevice())
            entries.current = static_cast<int>(entries.items.size());
        entries.items.push_back({device.name});
    }
    return entries;
}

// Figma "Embedded Tracks", "External Tracks" and "Output Device" (496:2535): a title 60 tall, 28 in
// from the sides, with its actions at its end as bare icons 20 apart; then the entries, 20 in, the
// current one checked, and 18 under them. A section starting a group of its own has a divider above.
class Section : public Column
{
public:
    Section(const wchar_t *title, bool dividerAbove)
        : m_dividerAbove(dividerAbove)
    {
        setPadding({.top = dividerAbove ? dividerHeight : 0, .bottom = 18});
        m_header = &add<Row>(20.0f);
        m_header->setPadding({.left = 28, .right = 28});
        m_header->setPreferredSize({0, 60});
        m_header->setFillsWidth(true);
        m_header->add<Label>(title, TextStyle{.size = 14}, &Palette::fgA3).setFillsWidth(true);
        auto &entries = add<Column>();
        entries.setPadding({.left = 20, .right = 20});
        entries.setFillsWidth(true);
        m_list = &entries.add<ListView>(L"", ListMark::Check);
        m_list->setFillsWidth(true);
    }

    ListView &list() const { return *m_list; }

    IconButton &addAction(const wchar_t *icon, std::wstring tooltip, std::function<void()> onClick)
    {
        auto &action = m_header->add<IconButton>(icon);
        action.setIconColor(&Palette::fgA3);
        action.setTooltip(std::move(tooltip));
        action.setOnClick(std::move(onClick));
        return action;
    }

    void paint(Canvas &canvas) override
    {
        if (m_dividerAbove)
            canvas.fillRect({0, 0, frame().width, dividerHeight}, canvas.palette().dividerDim);
    }

private:
    bool m_dividerAbove;
    Row *m_header;
    ListView *m_list;
};

// A tab's sections, 1 apart, scrolling when they don't fit, kept in step with the player.
class TracksPage : public ScrollView
{
public:
    explicit TracksPage(Player &player)
    {
        m_sections = &setContent<Column>(1.0f);
        m_playerSubscription = player.changed.connect([this] { sync(); });
    }

    // A section listing what `read` gives; a click on an entry gives its index to `choose`.
    Section &addSection(const wchar_t *title, std::function<Entries()> read, std::function<void(int index)> choose,
                        bool dividerAbove = false)
    {
        auto &section = m_sections->add<Section>(title, dividerAbove);
        section.setFillsWidth(true);
        section.list().setOnActivated(std::move(choose));
        m_sources.push_back({&section, std::move(read), std::nullopt});
        sync();
        return section;
    }

private:
    struct Source
    {
        Section *section;
        std::function<Entries()> read;
        std::optional<Entries> shown;
    };

    void sync()
    {
        // The player changes many times a second; what the sections list only now and then.
        bool changed = false;
        for (Source &source : m_sources) {
            Entries entries = source.read();
            if (entries == source.shown)
                continue;
            source.section->list().setItems(entries.items, entries.current);
            source.shown = std::move(entries);
            changed = true;
        }
        // The content's height changed with its entries.
        if (changed)
            layout();
    }

    Column *m_sections;
    std::vector<Source> m_sources;
    Subscription m_playerSubscription;
};

void addEmbeddedTracks(TracksPage &page, Player &player, TrackKind kind)
{
    page.addSection(
        L"Embedded Tracks", [&player, kind] { return trackEntries(player.tracks(kind), player.currentTrack(kind)); },
        [&player, kind](int index) { player.selectTrack(kind, player.tracks(kind)[index].id); });
}

} // namespace

TracksPanel::TracksPanel(const BarContext &context, TrackKind tab)
    : Column(1)
{
    Player &player = context.player;
    const CommandRegistry &commands = context.commands;
    SettingsStore &settings = context.settings;

    m_tabs = &add<Tabs>(std::vector<Tab>{{L"video", L"Video"}, {L"audio", L"Audio"}, {L"subtitles", L"Subtitles"}},
                        static_cast<int>(tab));
    m_tabs->setFillsWidth(true);
    m_tabs->setOnChanged([this](int index) { showTab(static_cast<TrackKind>(index)); });

    auto &video = add<TracksPage>(player);
    addEmbeddedTracks(video, player, TrackKind::Video);

    auto &audio = add<TracksPage>(player);
    addEmbeddedTracks(audio, player, TrackKind::Audio);
    audio
        .addSection(
            L"Output Device", [&player] { return deviceEntries(player); },
            [&player](int index) { player.setAudioDevice(player.audioDevices()[index].id); }, true)
        .addAction(L"refresh", L"Refresh", [&player] { player.refreshAudioDevices(); });

    auto &subtitles = add<TracksPage>(player);
    addEmbeddedTracks(subtitles, player, TrackKind::Subtitle);
    Section &external = subtitles.addSection(
        L"External Tracks",
        [&player] { return trackEntries(player.externalSubtitles(), player.currentTrack(TrackKind::Subtitle)); },
        [&player](int index) { player.selectTrack(TrackKind::Subtitle, player.externalSubtitles()[index].id); });
    external
        .addAction(L"edit", commands.label(CommandId::EditSubtitleTracks),
                   [&commands] { commands.run(CommandId::EditSubtitleTracks); })
        .setEnabled(commands.canRun(CommandId::EditSubtitleTracks));
    external.addAction(L"add", L"Add Subtitle File", [&external, &player, &settings] {
        // Next to the media, where its subtitle files usually are.
        const std::wstring &location = player.location();
        const std::wstring folder = isUrl(location) ? settings.get(AppSettings::mediaFolder)
                                                    : std::filesystem::path(location).parent_path().wstring();
        // Over the player: the panel closes as the dialog takes the activation.
        chooseFiles(external.window()->rootWindow(), folder, subtitleFileType(), [&player](std::vector<std::wstring> files) {
            for (const std::wstring &file : files)
                player.addSubtitleFile(file);
        });
    });

    m_pages = {&video, &audio, &subtitles};
    for (View *page : m_pages) {
        page->setFillsWidth(true);
        page->setFillsHeight(true);
    }

    auto &footer = add<Column>();
    footer.setPadding({20, 20, 20, 20});
    footer.setFillsWidth(true);
    auto &preferences = footer.add<Button>(L"Show Preferences", ButtonKind::Neutral, TextStyle{.size = 14});
    preferences.setPreferredSize({0, 32});
    preferences.setFillsWidth(true);
    preferences.setEnabled(commands.canRun(CommandId::ShowPlaybackSettings));
    preferences.setOnClick([&commands] { commands.run(CommandId::ShowPlaybackSettings); });

    showTab(tab);
}

TrackKind TracksPanel::tab() const
{
    return static_cast<TrackKind>(m_tabs->active());
}

void TracksPanel::showTab(TrackKind tab)
{
    m_tabs->setActive(static_cast<int>(tab));
    for (size_t i = 0; i < m_pages.size(); ++i)
        m_pages[i]->setVisible(i == static_cast<size_t>(tab));
    layout();
    // The control bar's Audio and Subtitles buttons show which tab is open.
    if (Window *host = window())
        host->rootWindow().invalidate();
}

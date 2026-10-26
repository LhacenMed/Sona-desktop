#include "player/QueuePanel.h"

#include <array>

#include "commands/CommandRegistry.h"
#include "core/TimeText.h"
#include "data/Bookmarks.h"
#include "platform/FileDialogs.h"
#include "platform/Window.h"
#include "playback/MediaFiles.h"
#include "playback/Player.h"
#include "playback/Queue.h"
#include "player/BarItems.h"
#include "settings/AppSettings.h"
#include "widgets/IconButton.h"
#include "widgets/Label.h"
#include "widgets/ListView.h"
#include "widgets/ScrollView.h"
#include "widgets/Tabs.h"

namespace {

// "4 items", "1 Bookmark".
std::wstring countText(size_t count, const wchar_t *singular, const wchar_t *plural)
{
    return std::to_wstring(count) + L" " + (count == 1 ? singular : plural);
}

// A page's header (Figma 463:840): what the page holds, then its actions, 12 apart.
class Header : public Row
{
public:
    Header()
        : Row(10)
    {
        setPadding({.left = 28, .top = 20, .right = 20, .bottom = 20});
        setFillsWidth(true);
        m_count = &add<Label>(L"", TextStyle{.size = 14}, &Palette::fgA3);
        m_count->setFillsWidth(true);
        m_actions = &add<Row>(12.0f);
    }

    void setCount(std::wstring text) { m_count->setText(std::move(text)); }

    // Figma's actions are bare 24px icons, but repeat and shuffle sit in 30×28 frames (`framed`).
    IconButton &addAction(const wchar_t *icon, std::wstring tooltip, bool framed, std::function<void()> onClick)
    {
        auto &action = framed ? m_actions->add<IconButton>(icon, 3.0f, 2.0f) : m_actions->add<IconButton>(icon);
        action.setIconColor(&Palette::fgA3);
        action.setTooltip(std::move(tooltip));
        action.setOnClick(std::move(onClick));
        return action;
    }

private:
    Label *m_count;
    Row *m_actions;
};

// A page's list, scrolling under the header (Figma 463:846: 20 in from the sides).
class PageList : public ScrollView
{
public:
    PageList(const wchar_t *icon, float top)
    {
        auto &content = setContent<Column>();
        content.setPadding({.left = 20, .top = top, .right = 20});
        m_list = &content.add<ListView>(icon);
        m_list->setFillsWidth(true);
    }

    ListView &list() const { return *m_list; }

    void setItems(std::vector<ListItem> items, int current)
    {
        m_list->setItems(std::move(items), current);
        // The content's height changed with its rows.
        layout();
    }

private:
    ListView *m_list;
};

// Figma "Controls" queue (491:1781): the queue's length, sort, repeat, shuffle, remove and add, then
// its items with the playing one highlighted.
class QueuePage : public Column
{
public:
    QueuePage(Queue &queue, SettingsStore &settings)
        : Column(1)
        , m_queue(queue)
    {
        m_header = &add<Header>();
        m_header->addAction(L"sort", L"Sort by Name", false, [&queue] { queue.sortByName(); });
        m_repeat = &m_header->addAction(L"repeat", L"Repeat", true, [&queue] { queue.cycleRepeat(); });
        m_repeat->setActiveWhen([&queue] { return queue.repeat() != Repeat::Off; });
        m_header->addAction(L"shuffle", L"Shuffle", true, [&queue] { queue.toggleShuffle(); })
            .setActiveWhen([&queue] { return queue.shuffles(); });
        m_header->addAction(L"remove", L"Remove", false, [this, &queue] { queue.remove(m_list->list().selected()); });
        m_header->addAction(L"add", L"Add Files", false, [this, &queue, &settings] {
            chooseFiles(*window(), settings.get(AppSettings::mediaFolder), mediaFileType(),
                        [&queue](std::vector<std::wstring> files) { queue.append(files); });
        });

        m_list = &add<PageList>(L"move", 0.0f);
        m_list->setFillsWidth(true);
        m_list->setFillsHeight(true);
        ListView &list = m_list->list();
        list.setOnActivated([&queue](int index) { queue.play(index); });
        list.setOnRemoved([&queue](int index) { queue.remove(index); });
        list.setOnMoved([&queue](int from, int to) { queue.move(from, to); });

        m_queueSubscription = queue.changed.connect([this] { sync(); });
        sync();
    }

private:
    void sync()
    {
        std::wstring count = countText(m_queue.items().size(), L"item", L"items");
        if (const double total = m_queue.totalDuration(); total > 0)
            count += L" - " + totalLengthText(total);
        m_header->setCount(std::move(count));
        m_repeat->setIconName(m_queue.repeat() == Repeat::One ? L"repeat-one" : L"repeat");
        m_repeat->setTooltip(m_queue.repeat() == Repeat::Off ? L"Repeat"
                             : m_queue.repeat() == Repeat::All ? L"Repeat: All"
                                                               : L"Repeat: One");
        std::vector<ListItem> items;
        for (const QueueItem &item : m_queue.items())
            items.push_back({item.name, item.duration > 0 ? lengthText(item.duration) : L""});
        m_list->setItems(std::move(items), m_queue.current());
        // Repeat and shuffle show their state when they paint.
        invalidate();
    }

    const Queue &m_queue;
    Header *m_header;
    IconButton *m_repeat;
    PageList *m_list;
    Subscription m_queueSubscription;
};

// Figma "Controls" bookmarks (6320:13763): the playing media's bookmarks, with remove and add.
class BookmarksPage : public Column
{
public:
    BookmarksPage(Bookmarks &bookmarks, Player &player, const CommandRegistry &commands)
        : Column(1)
        , m_bookmarks(bookmarks)
    {
        m_header = &add<Header>();
        m_header->addAction(L"remove", L"Remove", false, [this] { removeBookmark(m_list->list().selected()); });
        m_header->addAction(L"add", commands.label(CommandId::AddBookmark), false,
                            [&commands] { commands.run(CommandId::AddBookmark); });

        m_list = &add<PageList>(L"", 0.0f);
        m_list->setFillsWidth(true);
        m_list->setFillsHeight(true);
        ListView &list = m_list->list();
        list.setOnActivated([this, &player](int index) { player.seek(m_bookmarks.list()[index].time); });
        list.setOnRemoved([this](int index) { removeBookmark(index); });

        m_bookmarksSubscription = m_bookmarks.changed.connect([this] { sync(); });
        sync();
    }

private:
    void removeBookmark(int index)
    {
        if (index >= 0)
            m_bookmarks.remove(m_bookmarks.list()[index].id);
    }

    void sync()
    {
        m_header->setCount(countText(m_bookmarks.list().size(), L"Bookmark", L"Bookmarks"));
        std::vector<ListItem> items;
        for (const Bookmark &bookmark : m_bookmarks.list())
            items.push_back({bookmark.name, preciseClockText(bookmark.time)});
        m_list->setItems(std::move(items), -1);
    }

    Bookmarks &m_bookmarks;
    Header *m_header;
    PageList *m_list;
    Subscription m_bookmarksSubscription;
};

// Figma "Controls" chapters (6321:14024): the playing media's chapters, numbered, from 20 below the
// tabs.
class ChaptersPage : public PageList
{
public:
    explicit ChaptersPage(Player &player)
        : PageList(L"", 20.0f)
        , m_player(player)
    {
        list().setOnActivated([this](int index) { m_player.seek(m_shown[index].time); });
        m_playerSubscription = player.changed.connect([this] { sync(); });
        sync();
    }

private:
    void sync()
    {
        // The player changes many times a second; its chapters only with the media.
        if (m_player.chapters() == m_shown)
            return;
        m_shown = m_player.chapters();
        std::vector<ListItem> items;
        for (size_t i = 0; i < m_shown.size(); ++i)
            items.push_back({m_shown[i].name, clockText(m_shown[i].time), static_cast<int>(i + 1)});
        setItems(std::move(items), -1);
    }

    Player &m_player;
    std::vector<Chapter> m_shown;
    Subscription m_playerSubscription;
};

} // namespace

QueuePanel::QueuePanel(const BarContext &context, Queue &queue)
    : Popover(1)
{
    auto &tabs = add<Tabs>(std::vector<Tab>{{L"queue", L"Queue"}, {L"bookmarks", L"Bookmarks"}, {L"chapters", L"Chapters"}}, 0);
    tabs.setFillsWidth(true);
    const std::array<View *, 3> pages{&add<QueuePage>(queue, context.settings),
                                      &add<BookmarksPage>(context.bookmarks, context.player, context.commands),
                                      &add<ChaptersPage>(context.player)};
    for (size_t i = 0; i < pages.size(); ++i) {
        pages[i]->setFillsWidth(true);
        pages[i]->setFillsHeight(true);
        pages[i]->setVisible(i == 0);
    }
    tabs.setOnChanged([this, pages](int active) {
        for (size_t i = 0; i < pages.size(); ++i)
            pages[i]->setVisible(static_cast<int>(i) == active);
        layout();
    });
}

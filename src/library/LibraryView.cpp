#include "library/LibraryView.h"

#include <algorithm>
#include <string>
#include <utility>

#include "data/History.h"
#include "graphics/Canvas.h"
#include "library/HomePage.h"
#include "library/NavigationRail.h"
#include "library/Thumbnails.h"
#include "platform/PopupWindow.h"
#include "platform/Window.h"
#include "player/TitleBar.h"
#include "settings/SettingsStore.h"
#include "style/Theme.h"
#include "widgets/IconButton.h"
#include "widgets/Label.h"
#include "widgets/Menu.h"
#include "widgets/ScrollView.h"
#include "widgets/Stack.h"
#include "widgets/TextField.h"

namespace {

// Figma: the content has 20 around it; the header is 34 tall, its divider 17 under it. In the
// header, the arrows are 12 apart and 10 before the breadcrumb, whose crumbs are 6 apart; the rest
// is 20 apart, the search field 325 wide.
constexpr float contentPadding = 20;
constexpr float headerHeight = 34;
constexpr float dividerGap = 17;
constexpr float searchWidth = 325;
constexpr TextStyle crumbStyle{.size = 16, .elide = true};
constexpr TextStyle currentCrumbStyle{.size = 16, .weight = DWRITE_FONT_WEIGHT_SEMI_BOLD, .elide = true};
constexpr TextStyle separatorStyle{.size = 8};
constexpr TextStyle countStyle{.size = 14};
// Figma's sorting, view and folder menus (856:2215, 858:3290, 2139:8935).
constexpr float menuWidth = 265;

// A place in the breadcrumb; one before the last goes there when clicked.
class Crumb : public Label
{
public:
    Crumb(std::wstring text, bool current, bool alone, std::function<void()> onClick)
        : Label(std::move(text), current && !alone ? currentCrumbStyle : crumbStyle, current && !alone ? &Palette::fgA1 : &Palette::fgA2)
        , m_onClick(std::move(onClick))
    {
    }

    bool takesMouse() const override { return static_cast<bool>(m_onClick); }
    Cursor cursor() const override { return Cursor::Hand; }

    void mouseReleased(Point, bool inside) override
    {
        if (inside)
            m_onClick();
    }

private:
    std::function<void()> m_onClick;
};

IconButton &headerButton(Row &row, const wchar_t *icon, Color Palette::*color, std::wstring tooltip, std::function<void()> onClick)
{
    auto &button = row.add<IconButton>(icon);
    button.setIconColor(color);
    button.setTooltip(std::move(tooltip));
    button.setOnClick(std::move(onClick));
    return button;
}

} // namespace

LibraryView::LibraryView(BarContext barContext, LibraryContext context)
    : m_barContext(std::move(barContext))
    , m_context(std::move(context))
{
    m_titleBar = &add<TitleBar>(m_barContext, libraryBarLayout);
    m_titleBar->setTitle(SONA_APP_NAME);
    m_rail = &add<NavigationRail>([this] { go({}); });

    m_header = &add<Row>(contentPadding);
    auto &leading = m_header->add<Row>(10.0f);
    leading.setFillsWidth(true);
    leading.setFillsHeight(true);
    auto &arrows = leading.add<Row>(12.0f);
    m_backButton = &headerButton(arrows, L"arrow-left", &Palette::fgA2, L"Back", [this] { goBack(); });
    m_forwardButton = &headerButton(arrows, L"arrow-right", &Palette::fgA2, L"Forward", [this] { goForward(); });
    m_breadcrumb = &leading.add<Row>(6.0f);
    m_breadcrumb->setFillsWidth(true);
    m_breadcrumb->setFillsHeight(true);
    m_count = &m_header->add<Label>(L"", countStyle, &Palette::fgA3);
    m_sortButton = &headerButton(*m_header, L"sort", &Palette::fgA3, L"Sort", [this] { openSortMenu(); });
    m_viewButton = &headerButton(*m_header, L"grid", &Palette::fgA3, L"View", [this] { openViewMenu(); });
    m_search = &m_header->add<TextField>(L"Search", TextFieldKind::Search);
    m_search->setPreferredSize({searchWidth, headerHeight});
    m_search->setOnChanged([this] { scheduleRefresh(); });
    m_moreButton = &headerButton(*m_header, L"more", &Palette::fgA3, L"More", [this] { openMoreMenu(); });

    m_scroll = &add<ScrollView>();
    m_page = &m_scroll->setContent<HomePage>(m_context, [this](HomeSection section) { go(section); });

    m_historySubscription = m_context.history.changed.connect([this] { scheduleRefresh(); });
    m_settingsSubscription = m_context.settings.changed.connect([this] { scheduleRefresh(); });
    m_thumbnailsSubscription = m_context.thumbnails.changed.connect([this] { invalidate(); });
    refresh();
}

LibraryView::~LibraryView() = default;

void LibraryView::focusSearch()
{
    window()->setFocus(m_search);
}

void LibraryView::go(Place place)
{
    if (place == m_places[m_placeIndex])
        return;
    m_places.erase(m_places.begin() + static_cast<std::ptrdiff_t>(m_placeIndex) + 1, m_places.end());
    m_places.push_back(place);
    ++m_placeIndex;
    scheduleRefresh();
}

void LibraryView::goBack()
{
    if (m_placeIndex == 0)
        return;
    --m_placeIndex;
    scheduleRefresh();
}

void LibraryView::goForward()
{
    if (m_placeIndex + 1 >= m_places.size())
        return;
    ++m_placeIndex;
    scheduleRefresh();
}

void LibraryView::scheduleRefresh()
{
    // After the click, key or change that asked for it is handled: what it rebuilds may be what's
    // handling it (a tile, a crumb).
    Window *host = window();
    if (!host) {
        refresh();
        return;
    }
    if (std::exchange(m_refreshPending, true))
        return;
    host->runLater([this] {
        m_refreshPending = false;
        refresh();
    });
}

void LibraryView::refresh()
{
    const Place place = m_places[m_placeIndex];
    const int count = m_page->show(place, m_search->text());
    m_count->setText(std::to_wstring(count) + (count == 1 ? L" item" : L" items"));
    m_backButton->setEnabled(m_placeIndex > 0);
    m_forwardButton->setEnabled(m_placeIndex + 1 < m_places.size());
    m_viewButton->setIconName(m_context.settings.get(AppSettings::libraryLayout) == LibraryLayout::Grid ? L"grid" : L"list");
    updateBreadcrumb();
    m_header->layout();
    m_scroll->layout();
    invalidate();
}

void LibraryView::updateBreadcrumb()
{
    for (View *crumb : std::exchange(m_crumbs, {}))
        m_breadcrumb->remove(*crumb);
    const Place place = m_places[m_placeIndex];
    m_crumbs.push_back(&m_breadcrumb->add<Crumb>(L"Home", !place, !place, place ? std::function<void()>([this] { go({}); }) : nullptr));
    if (place) {
        m_crumbs.push_back(&m_breadcrumb->add<Label>(L"▶", separatorStyle, &Palette::fgA4));
        auto &section = m_breadcrumb->add<Crumb>(sectionTitle(*place), true, false, nullptr);
        section.setFillsWidth(true);
        m_crumbs.push_back(&section);
    }
}

void LibraryView::openSortMenu()
{
    const LibrarySort current = m_context.settings.get(AppSettings::librarySort);
    std::vector<MenuItem> items;
    for (const SortOption &option : sortOptions) {
        items.push_back({.label = option.label,
                         .run = [this, sort = option.sort] { m_context.settings.set(AppSettings::librarySort, sort); },
                         .checked = option.sort == current,
                         .enabled = option.available});
    }
    Menu::open(*window(), m_sortButton->mapToWindow(m_sortButton->bounds()), PopupPlacement::Below, std::move(items),
               m_context.commands, menuWidth);
}

void LibraryView::openViewMenu()
{
    const LibraryLayout current = m_context.settings.get(AppSettings::libraryLayout);
    const auto choice = [this, current](const wchar_t *label, const wchar_t *icon, LibraryLayout layout) {
        return MenuItem{.label = label,
                        .icon = icon,
                        .run = [this, layout] { m_context.settings.set(AppSettings::libraryLayout, layout); },
                        .checked = layout == current};
    };
    Menu::open(*window(), m_viewButton->mapToWindow(m_viewButton->bounds()), PopupPlacement::Below,
               {choice(L"Grid View", L"grid", LibraryLayout::Grid), choice(L"List View", L"list", LibraryLayout::List)},
               m_context.commands, menuWidth);
}

void LibraryView::openMoreMenu()
{
    // Until the library keeps folders, adding plays what's added.
    Menu::open(*window(), m_moreButton->mapToWindow(m_moreButton->bounds()), PopupPlacement::Below,
               {
                   {.command = CommandId::OpenFile, .label = L"Add File", .icon = L"add"},
                   {.command = CommandId::OpenFolder, .label = L"Add Folder", .icon = L"new-folder"},
                   {.command = CommandId::EditSubtitleTracks},
                   {.command = CommandId::EditAudioTracks},
                   {.command = CommandId::Resync},
                   {.command = CommandId::MediaInfo},
               },
               m_context.commands, menuWidth);
}

void LibraryView::layout()
{
    const float width = frame().width;
    const float height = frame().height;
    const float titleHeight = window() ? window()->captionHeight() : 0;
    m_titleBar->setFrame({0, 0, width, titleHeight});
    m_rail->setFrame({0, titleHeight, NavigationRail::width, height - titleHeight});
    const float left = NavigationRail::width;
    m_header->setFrame({left + contentPadding, titleHeight + contentPadding, width - left - 2 * contentPadding, headerHeight});
    const float dividerBottom = m_header->frame().bottom() + dividerGap + 1;
    m_scroll->setFrame({left, dividerBottom, width - left, std::max(0.0f, height - dividerBottom)});
}

void LibraryView::paint(Canvas &canvas)
{
    const Palette &palette = canvas.palette();
    canvas.fillRect(bounds(), palette.bgB1);
    const Rect &header = m_header->frame();
    canvas.fillRect({header.x, header.bottom() + dividerGap, header.width, 1}, palette.dividerDim);
}

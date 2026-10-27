#pragma once

#include <optional>
#include <vector>

#include "core/Signal.h"
#include "library/LibraryContext.h"
#include "library/LibraryItems.h"
#include "player/BarItems.h"
#include "widgets/View.h"

class HomePage;
class IconButton;
class Label;
class NavigationRail;
class Row;
class ScrollView;
class TextField;
class TitleBar;

// Figma "Player" Home (6261:15078): the library, which the window shows while nothing plays. The
// title bar; the navigation on the left; then the header (back and forward through the places
// visited, where the library is, how many items it shows, sort, grid or list, search and more) over
// the page, Home or one of its sections in full.
class LibraryView : public View
{
public:
    LibraryView(BarContext barContext, LibraryContext context);
    ~LibraryView() override;

    TitleBar &titleBar() const { return *m_titleBar; }
    void focusSearch();

    void layout() override;
    void paint(Canvas &canvas) override;

private:
    // Home, or one of its sections in full.
    using Place = std::optional<HomeSection>;

    void go(Place place);
    void goBack();
    void goForward();
    // Shows the current place as the settings and search say, once what asked for it is handled.
    void scheduleRefresh();
    void refresh();
    void updateBreadcrumb();
    void openSortMenu();
    void openViewMenu();
    void openMoreMenu();

    BarContext m_barContext;
    LibraryContext m_context;
    TitleBar *m_titleBar;
    NavigationRail *m_rail;
    Row *m_header;
    IconButton *m_backButton;
    IconButton *m_forwardButton;
    Row *m_breadcrumb;
    std::vector<View *> m_crumbs;
    Label *m_count;
    IconButton *m_sortButton;
    IconButton *m_viewButton;
    TextField *m_search;
    IconButton *m_moreButton;
    ScrollView *m_scroll;
    HomePage *m_page;
    // The places visited, and which of them is shown.
    std::vector<Place> m_places{Place{}};
    size_t m_placeIndex = 0;
    bool m_refreshPending = false;
    Subscription m_historySubscription;
    Subscription m_thumbnailsSubscription;
    Subscription m_settingsSubscription;
};

#include "library/HomePage.h"

#include "data/History.h"
#include "graphics/Canvas.h"
#include "library/ItemViews.h"
#include "settings/SettingsStore.h"
#include "style/Theme.h"
#include "widgets/Icon.h"
#include "widgets/Label.h"

namespace {

// Figma: the page has 20 at its sides and bottom; the sections start 17 under the header's divider
// and are 17 apart. A section has 20 above its title (24 tall), and its items 20 above and below
// them, then 20 more.
constexpr float pageSide = 20;
constexpr float pageTop = 17;
constexpr float pageBottom = 20;
constexpr float sectionGap = 17;
constexpr float sectionPadding = 20;
constexpr float titleHeight = 24;
constexpr float shelfPadding = 20;
constexpr TextStyle titleStyle{.size = 20};
constexpr TextStyle emptyStyle{.size = 16, .align = TextAlign::Center};

// A section's title, then a chevron 5 after it: a click opens the section in full.
class SectionTitle : public View
{
public:
    SectionTitle(HomeSection section, std::function<void(HomeSection)> open) : m_section(section), m_open(std::move(open)) {}

    bool takesMouse() const override { return true; }
    Cursor cursor() const override { return Cursor::Hand; }

    void paint(Canvas &canvas) override
    {
        const Palette &palette = canvas.palette();
        const Color color = Color::mix(palette.fgA1, palette.fg2, hoverAmount());
        const wchar_t *title = sectionTitle(m_section);
        const float width = canvas.measureText(title, titleStyle).width;
        canvas.drawText(title, {0, 0, width + 1, frame().height}, titleStyle, color);
        drawIcon(canvas, L"chevron-right", {width + 5, (frame().height - iconSize) / 2}, color);
    }

    void mouseReleased(Point, bool inside) override
    {
        if (inside)
            m_open(m_section);
    }

private:
    HomeSection m_section;
    std::function<void(HomeSection)> m_open;
};

} // namespace

HomePage::HomePage(LibraryContext &context, std::function<void(HomeSection)> openSection)
    : m_context(context)
    , m_openSection(std::move(openSection))
{
}

int HomePage::show(std::optional<HomeSection> section, std::wstring_view search)
{
    while (!children().empty())
        remove(*children().back());
    m_shelves.clear();
    m_emptyLabel = nullptr;

    const LibraryLayout itemLayout = m_context.settings.get(AppSettings::libraryLayout);
    const LibrarySort sort = m_context.settings.get(AppSettings::librarySort);
    int count = 0;
    for (HomeSection shown : homeSections) {
        if (section && shown != *section)
            continue;
        const std::vector<LibraryItem> items = sectionItems(shown, m_context.history.entries(), search, sort);
        if (items.empty())
            continue;
        count += static_cast<int>(items.size());
        // Home shows a line of tiles, or two of rows; a section in full, all of them.
        View *title = section ? nullptr : &add<SectionTitle>(shown, m_openSection);
        auto &shelf = add<ItemShelf>(m_context, items, itemLayout, section ? 0 : itemLayout == LibraryLayout::Grid ? 1 : 2);
        m_shelves.push_back({title, &shelf});
    }
    if (m_shelves.empty())
        m_emptyLabel = &add<Label>(search.empty() ? L"What you play shows up here" : L"No results", emptyStyle, &Palette::fgA3);
    layout();
    invalidate();
    return count;
}

Size HomePage::preferredSize() const
{
    const float width = frame().width - 2 * pageSide;
    float height = pageTop + pageBottom;
    for (const Shelf &shelf : m_shelves) {
        if (&shelf != &m_shelves.front())
            height += sectionGap;
        height += shelf.title ? 2 * sectionPadding + titleHeight + 2 * shelfPadding : 0;
        height += shelf.items->heightFor(width);
    }
    if (m_emptyLabel)
        height += titleHeight;
    return {0, height};
}

void HomePage::layout()
{
    const float width = frame().width - 2 * pageSide;
    float top = pageTop;
    for (const Shelf &shelf : m_shelves) {
        if (&shelf != &m_shelves.front())
            top += sectionGap;
        if (shelf.title) {
            top += sectionPadding;
            shelf.title->setFrame({pageSide, top, width, titleHeight});
            top += titleHeight + shelfPadding;
        }
        const float height = shelf.items->heightFor(width);
        shelf.items->setFrame({pageSide, top, width, height});
        top += height;
        if (shelf.title)
            top += shelfPadding + sectionPadding;
    }
    if (m_emptyLabel)
        m_emptyLabel->setFrame({pageSide, top, width, titleHeight});
}

#include "library/NavigationRail.h"

#include <array>
#include <string>

#include "graphics/Canvas.h"
#include "style/Theme.h"
#include "widgets/Icon.h"

namespace {

// Figma: the panel is 8 in from the rail's edges, with corners of 6; its items start 16 in and 12
// down, 32 tall and 14 apart: the icon in a 30×32 wrapper, then the name 8 after it.
constexpr float panelMargin = 8;
constexpr float panelRadius = 6;
constexpr float itemLeft = 16;
constexpr float itemTop = 12;
constexpr float itemHeight = 32;
constexpr float itemGap = 14;
constexpr float iconWrapperWidth = 30;
constexpr float nameGap = 8;
constexpr TextStyle nameStyle{.size = 14, .elide = true};

struct Destination
{
    const wchar_t *icon;
    const wchar_t *name;
    // Built: Home only, so far.
    bool built;
};

constexpr std::array destinations{
    Destination{L"home", L"Home", true},          Destination{L"videos", L"Videos", false},
    Destination{L"songs", L"Music", false},       Destination{L"streaming", L"Network", false},
    Destination{L"playlist", L"Playlists", false}, Destination{L"recent", L"Recent", false},
    Destination{L"add", L"Add item", false},
};

// One destination. Home is where the library is, so it's shown active.
class RailItem : public View
{
public:
    RailItem(const Destination &destination, std::function<void()> onClick)
        : m_destination(destination)
        , m_onClick(std::move(onClick))
    {
    }

    bool takesMouse() const override { return m_destination.built; }
    Cursor cursor() const override { return Cursor::Hand; }

    void paint(Canvas &canvas) override
    {
        const Palette &palette = canvas.palette();
        const Rect wrapper{0, 0, iconWrapperWidth, itemHeight};
        canvas.fillRoundedRect(wrapper, 4, palette.hoverOverlay.faded(hoverAmount()));
        const Point iconOrigin{(iconWrapperWidth - iconSize) / 2, (itemHeight - iconSize) / 2};
        if (m_destination.built)
            drawActiveIcon(canvas, std::wstring(m_destination.icon), iconOrigin);
        else
            drawIcon(canvas, m_destination.icon, iconOrigin, palette.fgA4);
        const float nameLeft = iconWrapperWidth + nameGap;
        canvas.drawText(m_destination.name, {nameLeft, 0, frame().width - nameLeft, itemHeight}, nameStyle,
                        m_destination.built ? palette.fg2 : palette.fgA4);
    }

    void mouseReleased(Point, bool inside) override
    {
        if (inside)
            m_onClick();
    }

private:
    const Destination &m_destination;
    std::function<void()> m_onClick;
};

} // namespace

NavigationRail::NavigationRail(std::function<void()> goHome)
{
    for (const Destination &destination : destinations)
        add<RailItem>(destination, goHome);
}

void NavigationRail::layout()
{
    const float left = panelMargin + itemLeft;
    const float itemWidth = frame().width - 2 * left;
    float top = panelMargin + itemTop;
    for (const auto &item : children()) {
        item->setFrame({left, top, itemWidth, itemHeight});
        top += itemHeight + itemGap;
    }
}

void NavigationRail::paint(Canvas &canvas)
{
    const Palette &palette = canvas.palette();
    canvas.fillRect(bounds(), palette.bgB1);
    canvas.fillRoundedRect({panelMargin, panelMargin, frame().width - 2 * panelMargin, frame().height - 2 * panelMargin}, panelRadius,
                           palette.bgA4);
}

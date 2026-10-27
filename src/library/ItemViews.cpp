#include "library/ItemViews.h"

#include <algorithm>
#include <cmath>

#include "graphics/Canvas.h"
#include "library/Thumbnails.h"
#include "style/Theme.h"
#include "widgets/Icon.h"

namespace {

// Figma Home: a video's picture is 316×177.8, a song's and a folder's 200×200, all with corners of
// 12; the title is 20 below, a song's artist 10 under that. Folders show their icon at 48.
constexpr Size videoPicture{316, 177.8f};
constexpr Size squarePicture{200, 200};
constexpr float tileRadius = 12;
constexpr float titleGap = 20;
constexpr float artistGap = 10;
constexpr float tileTextHeight = 19;
constexpr float tileGlyph = 48;
constexpr float tileStrip = 4;
constexpr TextStyle tileText{.size = 16, .elide = true};
// Figma "media": a 32×32 picture with corners of 2, the title 10 after it.
constexpr float rowPicture = 32;
constexpr float rowRadius = 2;
constexpr float rowTitleLeft = rowPicture + 10;
constexpr float rowStrip = 2;
constexpr TextStyle rowText{.size = 14, .elide = true};
// Between tiles, and between the lines of a grid; between rows, both ways.
constexpr Size tileGaps{20, 40};
constexpr Size rowGaps{10, 10};

Size pictureSize(LibraryItem::Kind kind)
{
    return kind == LibraryItem::Kind::Video ? videoPicture : squarePicture;
}

// An item's picture in `box`: its thumbnail, a folder's icon, or a song's note while it has no
// cover; under a video's, how much was watched.
void drawItemPicture(Canvas &canvas, Thumbnails &thumbnails, const LibraryItem &item, const Rect &box, float radius,
                     float glyphSize, float stripHeight, float hover)
{
    const Palette &palette = canvas.palette();
    const bool folder = item.kind == LibraryItem::Kind::Folder;
    canvas.fillRoundedRect(box, radius, folder ? palette.bgA2 : palette.bgA4);
    const std::shared_ptr<const Picture> picture = folder ? nullptr : thumbnails.get(item.location);
    canvas.pushRoundedClip(box, radius);
    if (picture)
        canvas.drawPicture(box, picture);
    else if (item.kind != LibraryItem::Kind::Video)
        drawIcon(canvas, folder ? L"folder" : L"music",
                 {box.x + (box.width - glyphSize) / 2, box.y + (box.height - glyphSize) / 2}, palette.fgA2, glyphSize);
    if (item.progress > 0)
        canvas.fillRect({box.x, box.bottom() - stripHeight, box.width * static_cast<float>(item.progress), stripHeight},
                        palette.accentMain);
    canvas.fillRect(box, palette.hoverOverlay.faded(hover));
    canvas.popRoundedClip();
}

Color titleColor(const Palette &palette, float hover)
{
    return Color::mix(palette.fgA1, palette.fg2, hover);
}

} // namespace

ItemTile::ItemTile(LibraryContext &context, LibraryItem item)
    : m_context(context)
    , m_item(std::move(item))
{
}

Size ItemTile::sizeFor(LibraryItem::Kind kind)
{
    const Size picture = pictureSize(kind);
    const float text = tileTextHeight + (kind == LibraryItem::Kind::Audio ? artistGap + tileTextHeight : 0);
    return {picture.width, picture.height + titleGap + text};
}

void ItemTile::paint(Canvas &canvas)
{
    const Palette &palette = canvas.palette();
    const Size picture = pictureSize(m_item.kind);
    drawItemPicture(canvas, m_context.thumbnails, m_item, {0, 0, picture.width, picture.height}, tileRadius, tileGlyph, tileStrip,
                    hoverAmount());
    const float titleTop = picture.height + titleGap;
    canvas.drawText(m_item.title, {0, titleTop, frame().width, tileTextHeight}, tileText, titleColor(palette, hoverAmount()));
    if (m_item.kind == LibraryItem::Kind::Audio)
        canvas.drawText(m_item.artist, {0, titleTop + tileTextHeight + artistGap, frame().width, tileTextHeight}, tileText,
                        palette.fgA3);
}

void ItemTile::mouseReleased(Point, bool inside)
{
    if (inside)
        m_context.open(m_item);
}

ItemRow::ItemRow(LibraryContext &context, LibraryItem item)
    : m_context(context)
    , m_item(std::move(item))
{
}

void ItemRow::paint(Canvas &canvas)
{
    drawItemPicture(canvas, m_context.thumbnails, m_item, {0, 0, rowPicture, rowPicture}, rowRadius, iconSize, rowStrip, 0);
    canvas.drawText(m_item.title, {rowTitleLeft, 0, frame().width - rowTitleLeft, frame().height}, rowText,
                    titleColor(canvas.palette(), hoverAmount()));
}

void ItemRow::mouseReleased(Point, bool inside)
{
    if (inside)
        m_context.open(m_item);
}

ItemShelf::ItemShelf(LibraryContext &context, const std::vector<LibraryItem> &items, LibraryLayout layout, int maxLines)
    : m_layout(layout)
    , m_maxLines(maxLines)
{
    if (!items.empty())
        m_kind = items.front().kind;
    for (const LibraryItem &item : items) {
        if (layout == LibraryLayout::Grid)
            add<ItemTile>(context, item);
        else
            add<ItemRow>(context, item);
    }
}

Size ItemShelf::itemSize() const
{
    return m_layout == LibraryLayout::Grid ? ItemTile::sizeFor(m_kind) : ItemRow::size;
}

Size ItemShelf::gaps() const
{
    return m_layout == LibraryLayout::Grid ? tileGaps : rowGaps;
}

int ItemShelf::columnsFor(float width) const
{
    return std::max(1, static_cast<int>((width + gaps().width) / (itemSize().width + gaps().width)));
}

int ItemShelf::linesFor(float width) const
{
    const int count = static_cast<int>(children().size());
    const int lines = (count + columnsFor(width) - 1) / columnsFor(width);
    return m_maxLines > 0 ? std::min(lines, m_maxLines) : lines;
}

float ItemShelf::heightFor(float width) const
{
    const int lines = linesFor(width);
    return lines > 0 ? lines * itemSize().height + (lines - 1) * gaps().height : 0;
}

void ItemShelf::layout()
{
    const int columns = columnsFor(frame().width);
    const int shown = columns * linesFor(frame().width);
    const Size size = itemSize();
    for (int i = 0; i < static_cast<int>(children().size()); ++i) {
        View &item = *children()[i];
        item.setVisible(i < shown);
        item.setFrame({(i % columns) * (size.width + gaps().width), (i / columns) * (size.height + gaps().height), size.width,
                       size.height});
    }
}

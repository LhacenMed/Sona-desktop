#pragma once

#include <vector>

#include "library/LibraryContext.h"
#include "library/LibraryItems.h"
#include "widgets/View.h"

// Figma Home (6261:15078): an item of the grid. Its picture (a video's frame with how much was
// watched, a song's cover, a folder), then its title, and a song's artist under it. A click plays it.
class ItemTile : public View
{
public:
    ItemTile(LibraryContext &context, LibraryItem item);

    // A video's tile is 16:9, a song's and a folder's square.
    static Size sizeFor(LibraryItem::Kind kind);

    bool takesMouse() const override { return true; }
    Cursor cursor() const override { return Cursor::Hand; }
    void paint(Canvas &canvas) override;
    void mouseReleased(Point point, bool inside) override;

private:
    LibraryContext &m_context;
    LibraryItem m_item;
};

// Figma "media" (2404:9167): an item of the list, its picture small before its title. A click plays it.
class ItemRow : public View
{
public:
    static constexpr Size size{406, 32};

    ItemRow(LibraryContext &context, LibraryItem item);

    bool takesMouse() const override { return true; }
    Cursor cursor() const override { return Cursor::Hand; }
    void paint(Canvas &canvas) override;
    void mouseReleased(Point point, bool inside) override;

private:
    LibraryContext &m_context;
    LibraryItem m_item;
};

// Items laid out in lines as wide as the shelf, as tiles or rows. At most `maxLines` lines show (0:
// all of them), as a Home section shows what fits.
class ItemShelf : public View
{
public:
    ItemShelf(LibraryContext &context, const std::vector<LibraryItem> &items, LibraryLayout layout, int maxLines);

    // How tall the shelf is at `width`.
    float heightFor(float width) const;

    void layout() override;

private:
    Size itemSize() const;
    Size gaps() const;
    int columnsFor(float width) const;
    int linesFor(float width) const;

    LibraryLayout m_layout;
    LibraryItem::Kind m_kind = LibraryItem::Kind::Video;
    int m_maxLines;
};

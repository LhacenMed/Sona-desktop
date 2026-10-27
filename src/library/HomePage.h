#pragma once

#include <functional>
#include <optional>
#include <string_view>
#include <vector>

#include "library/LibraryContext.h"
#include "library/LibraryItems.h"
#include "widgets/View.h"

class ItemShelf;
class Label;

// Figma Home's page (6261:15078), under the library's header. Home shows each section with items
// (VLC hides the empty ones) as its title, which opens it in full, over one line of its items; a
// section in full shows all of them. With nothing to show, a line says so.
class HomePage : public View
{
public:
    HomePage(LibraryContext &context, std::function<void(HomeSection)> openSection);

    // Shows Home (no section), or a section in full, with the items matching `search`. Returns how
    // many items that is.
    int show(std::optional<HomeSection> section, std::wstring_view search);

    // As tall as its content at its width.
    Size preferredSize() const override;
    void layout() override;

private:
    struct Shelf
    {
        // None for a section shown in full.
        View *title;
        ItemShelf *items;
    };

    LibraryContext &m_context;
    std::function<void(HomeSection)> m_openSection;
    std::vector<Shelf> m_shelves;
    Label *m_emptyLabel = nullptr;
};

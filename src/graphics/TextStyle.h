#pragma once

#include <dwrite.h>

enum class TextAlign { Leading, Center };

// The app font (Typography), or one of the system's: the one Windows sets window titles in, or the
// icon font it draws its own glyphs with. Window chrome uses the system's to match the native caption.
enum class TextFont { App, System, Icons };

// How a piece of text is set. Text is centred vertically in the box it's drawn in.
struct TextStyle
{
    float size = 14;
    DWRITE_FONT_WEIGHT weight = DWRITE_FONT_WEIGHT_NORMAL;
    TextAlign align = TextAlign::Leading;
    bool wrap = false;
    // Ends text that doesn't fit with "…".
    bool elide = false;
    // 0 keeps the font's own line height.
    float lineHeight = 0;
    TextFont font = TextFont::App;

    bool operator==(const TextStyle &) const = default;
};

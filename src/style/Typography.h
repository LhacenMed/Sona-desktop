#pragma once

#include <array>

// The app font: Inter, bundled under assets/fonts in the weights the design uses.
namespace Typography {

inline constexpr wchar_t fontFamily[] = L"Inter";
inline constexpr std::array fontAssets{L"fonts/Inter-Light.ttf", L"fonts/Inter-Regular.ttf", L"fonts/Inter-SemiBold.ttf"};

// The system fonts, Windows 11's first, then Windows 10's: the one window titles are set in, and
// the icon font (both icon fonts share their code points).
inline constexpr std::array systemFontFamilies{L"Segoe UI Variable Text", L"Segoe UI"};
inline constexpr std::array iconFontFamilies{L"Segoe Fluent Icons", L"Segoe MDL2 Assets"};

// Glyphs of the icon font, as Windows uses them in its own window chrome.
namespace Glyph {
inline constexpr wchar_t minimize = 0xE921;
inline constexpr wchar_t maximize = 0xE922;
inline constexpr wchar_t restore = 0xE923;
inline constexpr wchar_t close = 0xE8BB;
inline constexpr wchar_t fullScreen = 0xE740;
inline constexpr wchar_t exitFullScreen = 0xE73F;
inline constexpr wchar_t more = 0xE712;
inline constexpr wchar_t sidebar = 0xE90C;
inline constexpr wchar_t folder = 0xE8B7;
} // namespace Glyph

} // namespace Typography

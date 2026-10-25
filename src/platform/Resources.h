#pragma once

#include <cstddef>
#include <span>
#include <string_view>

// A file embedded from src/assets, by its path there (e.g. L"icons/play-dark.svg"). The bytes live
// in the executable's image for the app's lifetime. Asset names are fixed at build time, so a
// missing one is a programming error and throws.
std::span<const std::byte> asset(std::wstring_view path);

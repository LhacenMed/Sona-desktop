#pragma once

#include <string>
#include <string_view>

// Conversions between Windows' UTF-16 and the UTF-8 libVLC uses.
std::string toUtf8(std::wstring_view text);
std::wstring fromUtf8(std::string_view text);

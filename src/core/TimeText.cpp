#include "core/TimeText.h"

#include <algorithm>
#include <cmath>
#include <format>

std::wstring clockText(double seconds)
{
    const long long total = std::llround(std::abs(seconds));
    return std::format(L"{}{:02}:{:02}:{:02}", seconds < 0 ? L"-" : L"", total / 3600, total % 3600 / 60, total % 60);
}

std::wstring lengthText(double seconds)
{
    const long long total = std::llround(seconds);
    if (total < 3600)
        return std::format(L"{:02}:{:02}", total / 60, total % 60);
    return std::format(L"{}:{:02}:{:02}", total / 3600, total % 3600 / 60, total % 60);
}

std::wstring totalLengthText(double seconds)
{
    // A queue shorter than a minute still takes one.
    const long long minutes = std::max(1LL, std::llround(seconds / 60));
    if (minutes < 60)
        return std::format(L"{}m", minutes);
    return std::format(L"{}h {}m", minutes / 60, minutes % 60);
}

std::wstring preciseClockText(double seconds)
{
    const long long total = std::llround(seconds * 1000);
    return std::format(L"{:02}:{:02}:{:02}:{:03}", total / 3'600'000, total % 3'600'000 / 60'000, total % 60'000 / 1000, total % 1000);
}

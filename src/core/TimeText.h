#pragma once

#include <string>

// How times are written, from seconds.

// "01:44:10": the player's clock (current time, duration, chapters); "-00:23:30" for time left.
std::wstring clockText(double seconds);
// "23:06" under an hour, "1:44:10" from one: how long a queue item is.
std::wstring lengthText(double seconds);
// "2h 3m" from an hour, "45m" under one: how long the whole queue is.
std::wstring totalLengthText(double seconds);
// "00:00:42:023": a bookmark, to the millisecond.
std::wstring preciseClockText(double seconds);

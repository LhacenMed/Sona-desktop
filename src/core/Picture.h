#pragma once

#include <cstdint>
#include <vector>

// A picture in memory: 32-bit BGR pixels, top row first.
struct Picture
{
    int width = 0;
    int height = 0;
    std::vector<std::uint32_t> pixels;
};

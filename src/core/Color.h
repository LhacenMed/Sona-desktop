#pragma once

#include <cstdint>

// A straight (not premultiplied) RGBA color, channels 0–1. The default is fully transparent.
struct Color
{
    float r = 0;
    float g = 0;
    float b = 0;
    float a = 0;

    // From 0xRRGGBB, the way Figma writes colors.
    static constexpr Color hex(std::uint32_t rgb, float alpha = 1)
    {
        return {((rgb >> 16) & 0xff) / 255.0f, ((rgb >> 8) & 0xff) / 255.0f, (rgb & 0xff) / 255.0f, alpha};
    }

    // The color `amount` of the way from `from` to `to`.
    static constexpr Color mix(Color from, Color to, float amount)
    {
        const auto at = [amount](float start, float end) { return start + (end - start) * amount; };
        return {at(from.r, to.r), at(from.g, to.g), at(from.b, to.b), at(from.a, to.a)};
    }

    // The same color with its opacity scaled by `amount`.
    constexpr Color faded(float amount) const { return {r, g, b, a * amount}; }

    // Back to 0xRRGGBB, dropping alpha.
    constexpr std::uint32_t rgb() const { return (channel(r) << 16) | (channel(g) << 8) | channel(b); }

private:
    static constexpr std::uint32_t channel(float value) { return static_cast<std::uint32_t>(value * 255.0f + 0.5f); }
};

#pragma once

#include <array>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

// Every setting of the preferences panel (Figma "Controls" playback, video settings, audio settings
// and subtitle settings), in the panel's order. Presets save their values; the player applies them.
enum class Preference {
    // Playback.
    SubtitleTimeline,
    Statistics,
    Reverse,
    SleepTimer,
    AudioVisualizer,
    Speed,
    // Video defaults.
    Flip,
    Rotate,
    Zoom,
    Hdr,
    Resolution,
    AspectRatio,
    Crop,
    Render,
    // Video filters: Essential.
    Hue,
    Brightness,
    Contrast,
    Saturation,
    Gamma,
    Sharpen,
    BandingRemoval,
    FilmGrain,
    // Video filters: Crop.
    CropTop,
    CropBottom,
    CropLeft,
    CropRight,
    // Video filters: Colors.
    ColorExtraction,
    ExtractedColor,
    ColorThreshold,
    ThresholdColor,
    ThresholdSaturation,
    ThresholdSimilarity,
    NegateColors,
    Posterize,
    Gradient,
    GradientMode,
    GradientColor,
    Cartoon,
    SepiaIntensity,
    // Audio.
    AudioLanguage,
    StereoMode,
    AudioDelay,
    EqualizerPreset,
    Preamp,
    Band60,
    Band170,
    Band310,
    Band600,
    Band1k,
    Band3k,
    Band6k,
    Band12k,
    Band14k,
    Band16k,
    TwoPass,
    Compressor,
    Spatializer,
    StereoWidener,
    Pitch,
    // Subtitles.
    SubtitleLanguage,
    SubtitleDelay,
    SubtitleSpeed,
    SubtitleDurationFactor,
    EmbeddedSubFormat,
    SubtitleFont,
    TextColor,
    OutlineColor,
    SubtitleOffset,
    SizeValues,
    RelativeFontSize,
    PixelFontSize,
    OutlineThickness,
    BackgroundOpacity,
};

inline constexpr size_t preferenceCount = static_cast<size_t>(Preference::BackgroundOpacity) + 1;

// How a preference is set in the panel.
enum class PreferenceControl {
    Toggle,
    // A range, with its value in a field above it when it has a format, or the name of its step
    // when it has names.
    Slider,
    // A dropdown (Figma "EQ" 3178:16768).
    Dropdown,
    // A field that lists its names to choose from (Figma text_input 4046:12353).
    Choice,
    // Joined options (Figma "Values").
    Options,
    // A number typed in a field.
    Number,
    // A colour typed in a field as #RRGGBB, beside a swatch.
    Color,
};

// When a change takes effect.
enum class PreferenceTiming {
    // At once.
    Live,
    // libVLC 3 reads it only when it starts: the player restarts with it and reopens the media where
    // it was.
    Restart,
    // libVLC 3 can't do it.
    Unavailable,
    // Built in a later phase.
    Later,
};

struct PreferenceInfo
{
    // Its name in presets.
    std::string_view key;
    std::wstring_view label;
    PreferenceControl control;
    PreferenceTiming timing;
    double minimum = 0;
    double maximum = 1;
    double step = 1;
    double fallback = 0;
    // A Dropdown's, Choice's or Options' names, in value order; a Slider's step names.
    std::vector<std::wstring> names;
    // A Slider's labels under its track, from its minimum to its maximum.
    std::vector<std::wstring> ticks;
    // How its value reads in a field, as std::format writes one number: "{:.3f} s".
    std::wstring_view format;
};

const PreferenceInfo &preferenceInfo(Preference preference);

// A value for every preference, numbers all: a toggle is 0 or 1, a choice its index, a colour its
// 0xRRGGBB.
class PreferenceValues
{
public:
    // Every preference at its fallback.
    PreferenceValues();

    double operator[](Preference preference) const { return m_values[static_cast<size_t>(preference)]; }
    double &operator[](Preference preference) { return m_values[static_cast<size_t>(preference)]; }
    bool operator==(const PreferenceValues &) const = default;

private:
    std::array<double, preferenceCount> m_values;
};

// The languages a media's tracks can be chosen by, "Any" first: their names and ISO 639-1 codes.
struct Language
{
    std::wstring_view name;
    std::string_view code;
};
extern const std::vector<Language> trackLanguages;

// The subtitle fonts: Windows' own families, "Default" (libVLC's) first.
extern const std::vector<std::wstring> subtitleFonts;

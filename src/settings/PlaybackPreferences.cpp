#include "settings/PlaybackPreferences.h"

const std::vector<Language> trackLanguages{
    {L"Any", ""},         {L"English", "en"},  {L"Arabic", "ar"},   {L"Chinese", "zh"},  {L"Dutch", "nl"},
    {L"French", "fr"},    {L"German", "de"},   {L"Hindi", "hi"},    {L"Italian", "it"},  {L"Japanese", "ja"},
    {L"Korean", "ko"},    {L"Polish", "pl"},   {L"Portuguese", "pt"}, {L"Russian", "ru"}, {L"Spanish", "es"},
    {L"Swedish", "sv"},   {L"Turkish", "tr"},
};

const std::vector<std::wstring> subtitleFonts{
    L"Default", L"Arial",   L"Calibri",         L"Cambria",      L"Comic Sans MS", L"Consolas",
    L"Georgia", L"Segoe UI", L"Tahoma",         L"Times New Roman", L"Trebuchet MS", L"Verdana",
};

namespace {

using enum PreferenceControl;
using enum PreferenceTiming;

std::vector<std::wstring> languageNames()
{
    std::vector<std::wstring> names;
    for (const Language &language : trackLanguages)
        names.emplace_back(language.name);
    return names;
}

const std::vector<std::wstring> delayTicks{L"-10 s", L"0 s", L"+10 s"};

// A band of the equalizer, in decibels (libVLC's range).
PreferenceInfo band(std::string_view key, std::wstring_view label)
{
    return {key, label, Slider, Live, -20, 20, 0.1, 0, {}, {}, L"{:.1f} dB"};
}

// In Preference's order. The ranges and fallbacks are libVLC's, as VLC's own panels set them.
std::vector<PreferenceInfo> preferenceTable()
{
    return {
        {"subtitleTimeline", L"Show Subtitle Timeline on Seekbar", Toggle, Later},
        {"statistics", L"Show Statistics", Toggle, Later},
        {"reverse", L"Reverse", Toggle, Unavailable},
        {"sleepTimer", L"Sleep Timer", Dropdown, Later, 0, 0, 1, 0, {L"Off"}},
        {"audioVisualizer", L"Audio Visualizer", Dropdown, Later, 0, 4, 1, 0,
         {L"Disabled", L"Spectrometer", L"Scope", L"Spectrum", L"Waveform"}},
        {"speed", L"Speed", Slider, Live, 0.25, 2, 0.05, 1, {},
         {L"0.25", L"0.5", L"0.75", L"1", L"1.25", L"1.5", L"1.75", L"2"}, L"{:g}x"},

        {"flip", L"Flip", Options, Restart, 0, 2, 1, 0, {L"None", L"Horizontal", L"Vertical"}},
        {"rotate", L"Rotate", Number, Restart, 0, 359, 1, 0, {}, {}, L"{:.0f} deg"},
        {"zoom", L"Zoom", Dropdown, Live, 0, 4, 1, 0, {L"Fit", L"1:4 Quarter", L"1:2 Half", L"1:1 Original", L"2:1 Double"}},
        {"hdr", L"HDR", Dropdown, Unavailable, 0, 0, 1, 0, {L"Auto"}},
        {"resolution", L"Resolution", Dropdown, Unavailable, 0, 0, 1, 0, {L"Best Available"}},
        {"aspectRatio", L"Aspect Ratio", Dropdown, Live, 0, 8, 1, 0,
         {L"Default", L"16:9", L"4:3", L"1:1", L"16:10", L"2.21:1", L"2.35:1", L"2.39:1", L"5:4"}},
        {"crop", L"Crop", Dropdown, Live, 0, 10, 1, 0,
         {L"Default", L"16:10", L"16:9", L"4:3", L"1.85:1", L"2.21:1", L"2.35:1", L"2.39:1", L"5:3", L"5:4", L"1:1"}},
        {"render", L"Render", Dropdown, Later, 0, 0, 1, 0, {L"Local"}},

        {"hue", L"Hue", Slider, Live, -180, 180, 1, 0},
        {"brightness", L"Brightness", Slider, Live, 0, 2, 0.01, 1},
        {"contrast", L"Contrast", Slider, Live, 0, 2, 0.01, 1},
        {"saturation", L"Saturation", Slider, Live, 0, 3, 0.01, 1},
        {"gamma", L"Gamma", Slider, Live, 0.01, 10, 0.01, 1},
        // 0 turns the filter off.
        {"sharpen", L"Sharpen Sigma", Slider, Restart, 0, 2, 0.05, 0},
        {"bandingRemoval", L"Banding Removal Radius", Slider, Restart, 0, 32, 1, 0},
        {"filmGrain", L"Film Grain Variance", Slider, Restart, 0, 10, 0.1, 0},

        {"cropTop", L"Top", Number, Live, 0, 4096, 1, 0, {}, {}, L"{:.0f} px"},
        {"cropBottom", L"Bottom", Number, Live, 0, 4096, 1, 0, {}, {}, L"{:.0f} px"},
        {"cropLeft", L"Left", Number, Live, 0, 4096, 1, 0, {}, {}, L"{:.0f} px"},
        {"cropRight", L"Right", Number, Live, 0, 4096, 1, 0, {}, {}, L"{:.0f} px"},

        {"colorExtraction", L"Color Extraction", Toggle, Restart},
        {"extractedColor", L"Extracted Color", Color, Restart, 0, 0xFFFFFF, 1, 0xFF0000},
        {"colorThreshold", L"Color Threshold", Toggle, Restart},
        {"thresholdColor", L"Threshold Color", Color, Restart, 0, 0xFFFFFF, 1, 0xFF0000},
        {"thresholdSaturation", L"Saturation", Slider, Restart, 0, 255, 1, 20},
        {"thresholdSimilarity", L"Similarity", Slider, Restart, 0, 255, 1, 15},
        {"negateColors", L"Negate Colors", Toggle, Restart},
        {"posterize", L"Posterize", Toggle, Restart},
        {"gradient", L"Gradient", Toggle, Restart},
        {"gradientMode", L"Gradient", Dropdown, Restart, 0, 2, 1, 0, {L"Gradient", L"Edge", L"Hough"}},
        {"gradientColor", L"Color", Toggle, Restart},
        {"cartoon", L"Cartoon", Toggle, Restart},
        // 0 turns the filter off.
        {"sepiaIntensity", L"Sepia Intensity", Slider, Restart, 0, 255, 1, 0},

        {"audioLanguage", L"Audio Language", Choice, Restart, 0, static_cast<double>(trackLanguages.size() - 1), 1, 0,
         languageNames()},
        {"stereoMode", L"Stereo Mode", Dropdown, Live, 0, 4, 1, 0,
         {L"Stereo", L"Reverse Stereo", L"Left", L"Right", L"Dolby Surround"}},
        {"audioDelay", L"Audio Track Delay", Slider, Live, -10, 10, 0.05, 0, {}, delayTicks, L"{:.3f} s"},
        // 0: off; then libVLC's presets, in its order.
        {"equalizerPreset", L"Equalizer", Dropdown, Live, 0, 18, 1, 0,
         {L"None", L"Flat", L"Classical", L"Club", L"Dance", L"Full Bass", L"Full Bass and Treble", L"Full Treble",
          L"Headphones", L"Large Hall", L"Live", L"Party", L"Pop", L"Reggae", L"Rock", L"Ska", L"Soft", L"Soft Rock",
          L"Techno"}},
        band("preamp", L"Preamp"),
        band("band60", L"60 Hz"),
        band("band170", L"170 Hz"),
        band("band310", L"310 Hz"),
        band("band600", L"600 Hz"),
        band("band1k", L"1 KHz"),
        band("band3k", L"3 KHz"),
        band("band6k", L"6 KHz"),
        band("band12k", L"12 KHz"),
        band("band14k", L"14 KHz"),
        band("band16k", L"16 KHz"),
        {"twoPass", L"2 Pass", Toggle, Restart},
        {"compressor", L"Compressor", Toggle, Unavailable},
        {"spatializer", L"Spatializer", Toggle, Unavailable},
        {"stereoWidener", L"Stereo Widener", Toggle, Unavailable},
        {"pitch", L"Adjust Pitch", Slider, Unavailable, -12, 12, 0.5, 0, {}, {}, L"{:.1f} semitones"},

        {"subtitleLanguage", L"Subtitle Language", Choice, Restart, 0, static_cast<double>(trackLanguages.size() - 1), 1, 0,
         languageNames()},
        {"subtitleDelay", L"Subtitle Track Delay", Slider, Live, -10, 10, 0.05, 0, {}, delayTicks, L"{:.3f} s"},
        // 0: the subtitles' own.
        {"subtitleSpeed", L"Subtitle Speed", Slider, Restart, 0, 60, 0.5, 0, {}, {}, L"{:.3f} fps"},
        // 0: as the subtitles say.
        {"subtitleDurationFactor", L"Subtitle Duration Factor", Slider, Restart, 0, 20, 0.1, 0, {}, {}, L"{:.3f}"},
        {"embeddedSubFormat", L"Embedded Sub Format", Toggle, Restart, 0, 1, 1, 1},
        {"subtitleFont", L"Subtitle Font", Choice, Restart, 0, static_cast<double>(subtitleFonts.size() - 1), 1, 0,
         subtitleFonts},
        {"textColor", L"Text Color", Color, Restart, 0, 0xFFFFFF, 1, 0xFFFFFF},
        {"outlineColor", L"Outline Color", Color, Restart, 0, 0xFFFFFF, 1, 0x000000},
        {"subtitleOffset", L"Subtitle Offset", Number, Restart, 0, 500, 1, 0, {}, {}, L"{:.0f}px"},
        {"sizeValues", L"Size Values", Options, Restart, 0, 1, 1, 0, {L"Relative", L"Pixels"}},
        {"relativeFontSize", L"Font Size", Slider, Restart, 0, 5, 1, 0,
         {L"Auto", L"Smaller", L"Small", L"Normal", L"Large", L"Larger"}},
        // 0: libVLC's.
        {"pixelFontSize", L"Font Size", Slider, Restart, 0, 72, 1, 0, {}, {}, L"{:.0f} px"},
        {"outlineThickness", L"Outline Thickness", Slider, Restart, 0, 3, 1, 2, {L"None", L"Thin", L"Normal", L"Thick"}},
        {"backgroundOpacity", L"Background Opacity", Slider, Restart, 0, 100, 1, 0, {}, {}, L"{:.0f} %"},
    };
}

} // namespace

const PreferenceInfo &preferenceInfo(Preference preference)
{
    static const std::vector<PreferenceInfo> table = preferenceTable();
    return table[static_cast<size_t>(preference)];
}

PreferenceValues::PreferenceValues()
{
    for (size_t i = 0; i < preferenceCount; ++i)
        m_values[i] = preferenceInfo(static_cast<Preference>(i)).fallback;
}

pragma Singleton

import QtQuick
import Sona

// Design tokens from the Figma "VLC Redesign" file, resolved for the theme and accent in AppSettings.
// Names follow the Figma variables (Dark/… and Light/…).
QtObject {
    readonly property bool dark: AppSettings.theme === "dark"
                                 || (AppSettings.theme === "system" && Qt.styleHints.colorScheme !== Qt.ColorScheme.Light)

    // The onboarding swatches (Figma 1902:3670). `dim` is the elapsed seek fill (BG/Accent Bright).
    // Orange is the Figma token pair; the others use the 24% blend over BG/B2 measured on the red variant,
    // and in Light a 35% blend over Light/BG/B2 (Figma defines no Light values for them).
    readonly property var accents: [
        { dark: "#d37918", darkDim: "#371f06", light: "#e8851a", lightDim: "#d8ba9a" },
        { dark: "#e15032", darkDim: "#3d1a13", light: "#e15032", lightDim: "#dfaca2" },
        { dark: "#c49e23", darkDim: "#362d0f", light: "#c49e23", lightDim: "#d5c89d" },
        { dark: "#19a234", darkDim: "#0d2e13", light: "#19a234", lightDim: "#99c9a2" },
        { dark: "#18aad3", darkDim: "#0d3039", light: "#18aad3", lightDim: "#99ccda" },
        { dark: "#185dd3", darkDim: "#0d1d39", light: "#185dd3", lightDim: "#99b1da" },
        { dark: "#8e42ff", darkDim: "#291744", light: "#8e42ff", lightDim: "#c2a7ea" },
        { dark: "#f052b3", darkDim: "#401b32", light: "#f052b3", lightDim: "#e4adcf" },
        { dark: "#afafaf", darkDim: "#313131", light: "#afafaf", lightDim: "#cecece" }
    ]
    readonly property var accent: accents[AppSettings.accentIndex]

    readonly property color accentMain: dark ? accent.dark : accent.light
    readonly property color bgAccentBright: dark ? accent.darkDim : accent.lightDim
    readonly property color bgA3: dark ? "#1b1b1b" : "#d5d5d5"
    readonly property color bgB1: dark ? "#0e0e0e" : "#e7e7e7"
    readonly property color bgB2: dark ? "#090909" : "#dedede"
    readonly property color fgA2: dark ? "#8f8f8f" : "#2e2e2e"
    readonly property color fgA3: "#6b6b6b"
    readonly property color fgB1: "#161616"
    readonly property color dividerBright: dark ? "#373737" : "#9b9b9b"
    readonly property color dividerDim: dark ? "#222222" : "#acacac"
    readonly property color volumeTrack: dark ? "#090909" : "#888888" // BG/B2 in Dark, BG/A5 in Light
    // Not defined for Light in Figma; chosen to continue the Light foreground scale.
    readonly property color fg1: dark ? "#eaeaea" : "#161616"
    readonly property color fg2: dark ? "#c6c6c6" : "#2e2e2e"
    readonly property color fgA1: dark ? "#969696" : "#3d3d3d"
    readonly property color bgA4: dark ? "#191919" : "#acacac"

    // Button states shared by every button. Dark hover is the Figma "Icon Wrapper" fill;
    // the others are the Windows 11 caption button values.
    readonly property color hoverOverlay: dark ? Qt.rgba(1, 1, 1, 0.06) : Qt.rgba(0, 0, 0, 0.0373)
    readonly property color pressedOverlay: dark ? Qt.rgba(1, 1, 1, 0.0419) : Qt.rgba(0, 0, 0, 0.0241)
    readonly property color closeHover: "#c42b1c"
    readonly property color closePressed: Qt.rgba(196 / 255, 43 / 255, 28 / 255, 0.9)
    readonly property color closeForeground: "#ffffff"

    function hex(color) {
        return color.toString().slice(1)
    }
}

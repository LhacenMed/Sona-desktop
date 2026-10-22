import QtQuick
import Sona

// A 24×24 icon from the Figma "Icon" set, looked up by name in assets/icons and tinted for the theme.
Image {
    required property string name
    // Replaces the icon's Figma foreground (Dark/FG/A2).
    property color color: Theme.fgA2

    sourceSize: Qt.size(24, 24)
    source: "image://themed/assets/icons/" + name + ".svg?8F8F8F=" + Theme.hex(color)
            + "&373737=" + Theme.hex(Theme.dividerBright)
}

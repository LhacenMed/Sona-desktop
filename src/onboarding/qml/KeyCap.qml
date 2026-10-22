import QtQuick
import Sona

// Figma keycap (1902:7075): a key with a hard 6px drop shadow.
Item {
    id: keyCap

    required property string text
    // Arrow and punctuation keys, shown in pairs, use narrower padding.
    property bool compact: false

    implicitWidth: cap.width
    implicitHeight: cap.height

    Rectangle {
        y: 6
        width: cap.width
        height: cap.height
        radius: 12
        color: Qt.rgba(92 / 255, 92 / 255, 92 / 255, 0.25)
    }

    Rectangle {
        id: cap

        width: label.implicitWidth + 2 * (keyCap.compact ? 12 : 20) + 2 * border.width
        height: 42
        radius: 12
        color: Theme.bgA3
        border.width: 3
        border.color: Theme.dividerBright

        Text {
            id: label

            anchors.centerIn: parent
            text: keyCap.text
            color: Theme.fg1
            font.pixelSize: 16
            font.weight: Font.Light
        }
    }
}

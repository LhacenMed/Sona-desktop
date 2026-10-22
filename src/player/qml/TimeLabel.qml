import QtQuick
import Sona

// Figma "Current Time" / "Duration" (1904:7238, 1905:7312): a time in hh:mm:ss.
Rectangle {
    id: timeLabel

    required property real seconds
    property bool hoverHighlight: false

    function format(totalSeconds) {
        const s = Math.max(0, Math.floor(totalSeconds))
        const pad = n => String(n).padStart(2, "0")
        return pad(Math.floor(s / 3600)) + ":" + pad(Math.floor(s % 3600 / 60)) + ":" + pad(s % 60)
    }

    implicitWidth: 76
    implicitHeight: 28
    radius: 6
    color: hoverHighlight && hover.hovered ? Theme.bgB2 : "transparent"

    HoverHandler {
        id: hover
    }

    Text {
        anchors.centerIn: parent
        text: timeLabel.format(timeLabel.seconds)
        color: Theme.fgA2
        font.pixelSize: 15
    }
}

import QtQuick
import Sona

// Figma "volume" (244:184): a wedge whose gradient fill is revealed up to the current level (0–200%).
Item {
    id: volumeBar

    // Each designed volume level and the fill width (px) its Figma variant reveals.
    readonly property var levels: [0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 150, 200]
    readonly property var fillWidths: [1, 7, 14, 20, 26, 33, 41, 48, 53, 58, 63, 75, 88]

    // Piecewise-linear lookup between two parallel, ascending stop lists.
    function interpolate(value, from, to) {
        for (let i = 1; i < from.length; ++i) {
            if (value <= from[i])
                return to[i - 1] + (value - from[i - 1]) / (from[i] - from[i - 1]) * (to[i] - to[i - 1])
        }
        return to[to.length - 1]
    }

    implicitWidth: 88
    implicitHeight: 28

    Image {
        y: 0.32
        width: 89
        height: 27.36
        sourceSize: Qt.size(width, height)
        source: "image://themed/assets/images/volume-track.svg?090909=" + Theme.hex(Theme.volumeTrack)
    }

    Item {
        width: volumeBar.interpolate(Player.volume, volumeBar.levels, volumeBar.fillWidths)
        height: volumeBar.height
        clip: true

        Image {
            x: 1.5
            y: 1.72
            width: 86
            height: 24.56
            sourceSize: Qt.size(width, height)
            source: "qrc:/qt/qml/Sona/assets/images/volume-fill.svg"
        }
    }

    MouseArea {
        function setVolumeAt(mouseX) {
            const fill = Math.min(Math.max(mouseX, volumeBar.fillWidths[0]), volumeBar.width)
            Player.volume = Math.round(volumeBar.interpolate(fill, volumeBar.fillWidths, volumeBar.levels))
        }

        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onPressed: mouse => setVolumeAt(mouse.x)
        onPositionChanged: mouse => setVolumeAt(mouse.x)
    }
}

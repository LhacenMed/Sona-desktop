import QtQuick
import Sona

// Figma seek track (1908:7387): elapsed fill and handle over a dark track. Click or drag to seek.
Rectangle {
    id: seekBar

    readonly property real progress: dragArea.pressed ? dragArea.pressedProgress
                                     : Player.duration > 0 ? Player.position / Player.duration : 0

    implicitHeight: 28
    radius: 6
    color: Theme.bgB2

    Rectangle {
        x: 2
        y: 2
        width: handle.x + handle.width - x
        height: 24
        radius: 4
        color: Theme.bgAccentBright
    }

    Rectangle {
        id: handle

        x: 2 + seekBar.progress * (seekBar.width - 4 - width)
        y: 2
        width: 8
        height: 24
        radius: 4
        color: Theme.accentMain
    }

    MouseArea {
        id: dragArea

        property real pressedProgress: 0

        function seekTo(mouseX) {
            const travel = seekBar.width - 4 - handle.width
            pressedProgress = Math.min(Math.max((mouseX - 2 - handle.width / 2) / travel, 0), 1)
            Player.seek(pressedProgress * Player.duration)
        }

        anchors.fill: parent
        enabled: Player.duration > 0
        cursorShape: Qt.PointingHandCursor
        onPressed: mouse => seekTo(mouse.x)
        onPositionChanged: mouse => seekTo(mouse.x)
    }
}

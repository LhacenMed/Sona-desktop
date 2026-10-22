import QtQuick
import QtQuick.Templates as T
import Sona

// Figma "play" (848:1712): shows "play" while paused and "pause" while playing.
T.AbstractButton {
    implicitWidth: 54
    implicitHeight: 28
    focusPolicy: Qt.NoFocus
    // The play glyph sits 1px right of centre, as in the design.
    leftPadding: Player.playing ? 15 : 16
    rightPadding: Player.playing ? 15 : 14
    verticalPadding: 2

    onClicked: Player.togglePause()

    background: Rectangle {
        radius: 6
        color: Player.playing ? Theme.fgA3 : Theme.accentMain
    }

    contentItem: Icon {
        name: Player.playing ? "pause-dark" : "play-dark"
    }

    HoverHandler {
        cursorShape: Qt.PointingHandCursor
    }
}

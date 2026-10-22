import QtQuick
import Sona

// The video viewport. Its frame reproduces Figma 6586:17371: a 20px centre-aligned #1b1b1b stroke
// with a 19px radius, whose outer half runs under the bars and whose inner half rounds the video.
Item {
    VideoSurface {
        anchors.fill: parent
        player: Player
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: -20
        radius: 29
        color: "transparent"
        border.width: 20
        border.color: Theme.bgA3
    }
}

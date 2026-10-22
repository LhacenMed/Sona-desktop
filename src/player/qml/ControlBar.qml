import QtQuick
import QtQuick.Layouts
import Sona

// Figma "Controls" (491:1779): transport, seek, volume and panel buttons.
Rectangle {
    implicitHeight: 54
    color: Theme.bgA3

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 20
        anchors.rightMargin: 20
        spacing: 20

        RowLayout {
            spacing: 8

            IconButton {
                iconName: "previous"
                horizontalPadding: 3
                verticalPadding: 2
            }

            PlayButton {}

            IconButton {
                iconName: "next"
                horizontalPadding: 3
                verticalPadding: 2
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            TimeLabel {
                seconds: Player.position
                hoverHighlight: true
            }

            SeekBar {
                Layout.fillWidth: true
            }

            TimeLabel {
                seconds: Player.duration
            }
        }

        RowLayout {
            spacing: 8

            IconButton {
                iconName: Player.muted || Player.volume === 0 ? "volume-mute"
                          : Player.volume < 50 ? "volume-medium" : "volume-high"
                onClicked: Player.toggleMute()
            }

            VolumeBar {}
        }

        RowLayout {
            spacing: 12

            IconWrapper {
                iconName: "audio"
            }

            IconWrapper {
                iconName: "subtitles"
            }

            IconWrapper {
                iconName: "playback-settings"
            }
        }

        IconWrapper {
            iconName: "lists"
        }
    }
}

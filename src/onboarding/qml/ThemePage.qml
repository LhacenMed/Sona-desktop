import QtQuick
import QtQuick.Shapes
import Sona

// Onboarding step 2 (Figma 1902:3670): theme and accent, previewed live on the real player bars.
Item {
    component ThemeTile: Item {
        id: tile

        required property string value
        required property string label
        default property alias face: face.data

        width: 124
        height: 166

        Rectangle {
            width: 124
            height: 124
            radius: 19
            color: "transparent"
            border.width: 4
            border.color: AppSettings.theme === tile.value ? Theme.accentMain : "transparent"
        }

        Item {
            id: face

            x: 12
            y: 12
            width: 100
            height: 100
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            y: 142
            text: tile.label
            color: Theme.fgA2
            font.pixelSize: 16
            font.weight: Font.Light
        }

        TapHandler {
            onTapped: AppSettings.theme = tile.value
        }

        HoverHandler {
            cursorShape: Qt.PointingHandCursor
        }
    }

    // The player at 55% (Figma 1902:3671).
    Rectangle {
        x: 17
        y: 19
        width: 1056
        height: 594
        color: Theme.bgB2
        clip: true

        Item {
            width: 1920
            height: 1080
            scale: 0.55
            transformOrigin: Item.TopLeft
            enabled: false

            TitleBar {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                title: ""
            }

            ControlBar {
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
            }
        }
    }

    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        y: 173
        text: qsTr("Select your preferred theme and color.")
        color: Theme.fgA1
        font.pixelSize: 24
        font.weight: Font.Light
    }

    ThemeTile {
        x: 327
        y: 282
        value: "light"
        label: qsTr("Light")

        Rectangle {
            anchors.fill: parent
            radius: 12
            color: "#afafaf"
        }
    }

    ThemeTile {
        x: 483
        y: 282
        value: "dark"
        label: qsTr("Dark")

        Rectangle {
            anchors.fill: parent
            radius: 12
            color: "#222222"
        }
    }

    ThemeTile {
        x: 639
        y: 282
        value: "system"
        label: qsTr("System")

        // A hard diagonal split: the CSS 238.7° gradient with both stops at 52.4%.
        Shape {
            anchors.fill: parent
            preferredRendererType: Shape.CurveRenderer

            ShapePath {
                strokeWidth: -1
                fillGradient: LinearGradient {
                    x1: 108.7
                    y1: 14.3
                    x2: -8.7
                    y2: 85.7

                    GradientStop { position: 0.524; color: "#afafaf" }
                    GradientStop { position: 0.524; color: "#222222" }
                }

                PathRectangle {
                    width: 100
                    height: 100
                    radius: 12
                }
            }
        }
    }

    Repeater {
        model: Theme.accents

        delegate: Item {
            id: swatch

            required property var modelData
            required property int index
            readonly property color swatchColor: Theme.dark ? modelData.dark : modelData.light
            readonly property bool selected: AppSettings.accentIndex === index

            x: 41 + index * 80
            y: 655
            width: 64
            height: 64

            Rectangle {
                anchors.fill: parent
                visible: swatch.selected
                radius: 32
                color: "transparent"
                border.width: 4
                border.color: Qt.rgba(swatch.swatchColor.r, swatch.swatchColor.g, swatch.swatchColor.b, 0.3)
            }

            Rectangle {
                anchors.centerIn: parent
                width: 40
                height: 40
                radius: 20
                color: swatch.swatchColor
                opacity: swatch.selected ? 1 : 0.5
            }

            TapHandler {
                onTapped: AppSettings.accentIndex = swatch.index
            }

            HoverHandler {
                cursorShape: Qt.PointingHandCursor
            }
        }
    }
}

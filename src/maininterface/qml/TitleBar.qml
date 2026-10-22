import QtQuick
import QtQuick.Layouts
import Sona

// Figma "topbar" (247:447). MainWindow hands its controls to the window agent, which gives them
// native behavior (snap layouts, system menu, dragging, double-click to maximize).
Rectangle {
    id: titleBar

    property string title: Window.window ? Window.window.title : ""
    property alias logo: logo
    property alias minimizeButton: minimizeButton
    property alias maximizeButton: maximizeButton
    property alias closeButton: closeButton
    // Controls that keep receiving clicks instead of dragging the window.
    property list<Item> interactiveControls: [moreButton, sidebarButton, fullScreenButton]

    signal fullScreenToggled()

    implicitHeight: 40
    color: Theme.bgA3

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 6
        spacing: 0

        RowLayout {
            Layout.preferredWidth: 300
            spacing: 7

            Item {
                id: logo

                implicitWidth: 38
                implicitHeight: 28

                Image {
                    anchors.centerIn: parent
                    width: 24
                    height: 24
                    sourceSize: Qt.size(24, 24)
                    source: "qrc:/qt/qml/Sona/assets/logo/sona-logo.svg"
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 0

                RowLayout {
                    spacing: 10

                    IconButton {
                        id: moreButton

                        iconName: "more"
                        padding: 2
                    }

                    IconButton {
                        id: sidebarButton

                        iconName: "sidebar-left"
                        padding: 2
                    }
                }

                Icon {
                    Layout.topMargin: 2
                    Layout.bottomMargin: 2
                    name: "divider"
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    Icon {
                        Layout.margins: 2
                        name: "folder"
                    }

                    Text {
                        Layout.fillWidth: true
                        Layout.rightMargin: 4
                        text: Player.folderName
                        color: Theme.fgA2
                        font.pixelSize: 14
                        elide: Text.ElideRight
                    }
                }
            }
        }

        Text {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            text: titleBar.title
            color: Theme.fgA2
            font.pixelSize: 14
            elide: Text.ElideRight
        }

        // Caption buttons fill the bar's height flush with the window corner, like native ones.
        // The group is 6px wider than the left one to make up for its padding, keeping the title centered.
        RowLayout {
            Layout.preferredWidth: 306
            Layout.fillHeight: true
            spacing: 0

            Item {
                Layout.fillWidth: true
            }

            CaptionButton {
                id: minimizeButton

                Layout.fillHeight: true
                iconName: "minimize"
                onClicked: WindowCommands.minimize(titleBar.Window.window)
            }

            CaptionButton {
                id: maximizeButton

                Layout.fillHeight: true
                iconName: titleBar.Window.visibility === Window.Maximized ? "restore" : "maximize"
                onClicked: WindowCommands.toggleMaximized(titleBar.Window.window)
            }

            CaptionButton {
                id: fullScreenButton

                Layout.fillHeight: true
                iconName: titleBar.Window.visibility === Window.FullScreen ? "fullscreen-active" : "fullscreen"
                onClicked: titleBar.fullScreenToggled()
            }

            CaptionButton {
                id: closeButton

                Layout.fillHeight: true
                iconName: "close"
                closesWindow: true
                onClicked: titleBar.Window.window.close()
            }
        }
    }
}

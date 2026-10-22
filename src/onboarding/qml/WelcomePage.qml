import QtCore
import QtQuick
import QtQuick.Dialogs
import QtQuick.Layouts
import QtQuick.Templates as T
import Sona

// Onboarding step 1 (Figma 1899:3055): welcome and the media folder.
Item {
    // Shows a folder URL as a path, abbreviating the home folder to "~" as the design does.
    function displayPath(url) {
        const localPath = folderUrl => decodeURIComponent(folderUrl.toString().replace(/^file:\/\/\//, ""))
        const path = localPath(url)
        const home = localPath(StandardPaths.writableLocation(StandardPaths.HomeLocation))
        return path.startsWith(home) ? "~" + path.slice(home.length) : path
    }

    Image {
        x: (parent.width - width) / 2 + 0.5
        y: 116
        width: 141
        height: 141
        sourceSize: Qt.size(width, height)
        source: "qrc:/qt/qml/Sona/assets/logo/sona-logo.svg"
    }

    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        y: 286
        text: qsTr("Welcome to Sona!")
        color: Theme.fg1
        font.pixelSize: 32
        font.weight: Font.Light
    }

    Text {
        x: 248
        y: 362
        width: 595
        text: qsTr("Sona is a cross-platform media player that plays most video and audio files.")
        color: Theme.fgA1
        font.pixelSize: 16
        font.weight: Font.Light
        lineHeight: 28
        lineHeightMode: Text.FixedHeight
        wrapMode: Text.WordWrap
    }

    ColumnLayout {
        x: 248
        y: 500
        width: 595
        spacing: 16

        Text {
            text: qsTr("Choose a Media Folder to get started with")
            color: Theme.fgA2
            font.pixelSize: 14
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 36
                radius: 6
                color: Theme.bgB2
                border.width: 2
                border.color: Theme.bgA4

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    x: 12
                    width: parent.width - 30
                    text: displayPath(AppSettings.mediaFolder)
                    color: Theme.fg2
                    font.pixelSize: 16
                    elide: Text.ElideMiddle
                }
            }

            T.AbstractButton {
                implicitWidth: 114
                implicitHeight: 35
                focusPolicy: Qt.NoFocus
                onClicked: folderDialog.open()

                background: Rectangle {
                    radius: 6
                    color: Theme.bgA3
                }

                contentItem: Text {
                    text: qsTr("Browse")
                    color: Theme.fgA2
                    font.pixelSize: 16
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                HoverHandler {
                    cursorShape: Qt.PointingHandCursor
                }
            }
        }
    }

    FolderDialog {
        id: folderDialog

        currentFolder: AppSettings.mediaFolder
        onAccepted: AppSettings.mediaFolder = selectedFolder
    }
}

import QtQuick
import Sona

// Onboarding step 3 (Figma 1902:6770): the shortcut guide, two columns of six.
Item {
    id: page

    required property var groups

    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        y: 60
        text: qsTr("Learn the basic keyboard shortcuts.")
        color: Theme.fgA1
        font.pixelSize: 24
    }

    Repeater {
        model: page.groups

        delegate: Item {
            id: row

            required property var modelData
            required property int index

            x: index < 6 ? 62 : 605
            y: 154 + (index % 6) * 82
            width: 393
            height: 42

            Text {
                anchors.verticalCenter: parent.verticalCenter
                width: 220
                text: row.modelData.title
                color: Theme.fgA1
                font.pixelSize: 16
                font.weight: Font.Light
            }

            Row {
                x: 240
                spacing: 10

                Repeater {
                    model: row.modelData.bindings

                    delegate: Row {
                        id: binding

                        required property var modelData
                        required property int index

                        spacing: 10

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            visible: binding.index > 0
                            text: "/"
                            color: Theme.fgA1
                            font.pixelSize: 16
                            font.weight: Font.Light
                        }

                        Row {
                            spacing: 4

                            Repeater {
                                model: binding.modelData.keys

                                delegate: KeyCap {
                                    required property string modelData

                                    text: modelData
                                    compact: row.modelData.bindings.length > 1
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    Text {
        x: 62
        y: 671
        text: qsTr("Find the full list in the menu.")
        color: Theme.fgA3
        font.pixelSize: 16
        lineHeight: 28
        lineHeightMode: Text.FixedHeight
    }
}

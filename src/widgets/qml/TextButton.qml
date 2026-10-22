import QtQuick
import QtQuick.Templates as T
import Sona

// Figma onboarding "Next" / "Back" buttons (1902:3383, 6696:20706): 100×32, semibold label.
T.AbstractButton {
    id: control

    property bool primary: false

    implicitWidth: 100
    implicitHeight: 32
    focusPolicy: Qt.NoFocus

    background: Rectangle {
        radius: 6
        color: control.primary ? Theme.accentMain : Theme.bgA3
    }

    contentItem: Text {
        text: control.text
        color: control.primary ? Theme.fgB1 : Theme.fgA2
        font.pixelSize: 16
        font.weight: Font.DemiBold
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    HoverHandler {
        cursorShape: Qt.PointingHandCursor
    }
}

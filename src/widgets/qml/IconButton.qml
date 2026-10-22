import QtQuick
import QtQuick.Templates as T
import Sona

// A clickable icon; each call site sets the padding its Figma frame uses.
// Every icon button shares the same hover and pressed feedback.
T.AbstractButton {
    id: control

    required property string iconName
    property int cursorShape: Qt.PointingHandCursor

    implicitWidth: implicitContentWidth + leftPadding + rightPadding
    implicitHeight: implicitContentHeight + topPadding + bottomPadding
    focusPolicy: Qt.NoFocus
    hoverEnabled: true

    contentItem: Icon {
        name: control.iconName
    }

    background: Rectangle {
        radius: 4
        color: control.pressed ? Theme.pressedOverlay : control.hovered ? Theme.hoverOverlay : "transparent"
    }

    HoverHandler {
        cursorShape: control.cursorShape
    }
}

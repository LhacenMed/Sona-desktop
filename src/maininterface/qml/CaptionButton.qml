import QtQuick
import Sona

// A window caption button that looks and responds like the native Windows ones: a full-height
// 46px cell with the native hover and pressed fills, a red close button with a white glyph,
// and glyphs that dim while the window is inactive.
IconButton {
    id: control

    property bool closesWindow: false
    readonly property bool closeHighlighted: closesWindow && (hovered || pressed)

    implicitWidth: 46
    cursorShape: Qt.ArrowCursor

    contentItem: Item {
        Icon {
            anchors.centerIn: parent
            name: control.iconName
            color: control.closeHighlighted ? Theme.closeForeground
                                            : control.Window.active ? Theme.fgA2 : Theme.fgA3
        }
    }

    background: Rectangle {
        color: control.closesWindow
               ? (control.pressed ? Theme.closePressed : control.hovered ? Theme.closeHover : "transparent")
               : (control.pressed ? Theme.pressedOverlay : control.hovered ? Theme.hoverOverlay : "transparent")
    }
}

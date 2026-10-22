import QtQuick
import QtQuick.Layouts
import Sona

// First-run onboarding (Figma "Onboarding" 1899:3055, 1902:3670, 1902:6770): welcome, theme, shortcuts.
Window {
    id: onboardingWindow

    required property var shortcutGroups

    signal finished()

    width: 1090
    height: 759
    // Shown once the window agent has taken over the frame.
    visible: false
    modality: Qt.ApplicationModal
    color: Theme.bgB1
    title: qsTr("Welcome to Sona")

    Component.onCompleted: {
        windowAgent.attach(onboardingWindow)
        windowAgent.setTitleBar(dragBand)
        // Size constraints go after setup, as the agent requires.
        minimumWidth = maximumWidth = width
        minimumHeight = maximumHeight = height
        visible = true
    }

    AppWindowAgent {
        id: windowAgent
    }

    // The empty band above every page moves the window, like a title bar.
    Item {
        id: dragBand

        width: parent.width
        height: 48
    }

    StackLayout {
        id: pages

        anchors.fill: parent

        WelcomePage {}

        ThemePage {}

        ShortcutsPage {
            groups: onboardingWindow.shortcutGroups
        }
    }

    TextButton {
        x: 820
        y: 671
        visible: pages.currentIndex > 0
        text: qsTr("Back")
        onClicked: pages.currentIndex--
    }

    TextButton {
        readonly property bool lastPage: pages.currentIndex === pages.count - 1

        x: 928
        y: 671
        primary: true
        text: lastPage ? qsTr("Finish") : qsTr("Next")
        onClicked: lastPage ? onboardingWindow.finished() : pages.currentIndex++
    }
}

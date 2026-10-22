import QtQuick
import QtQuick.Dialogs
import QWindowKit
import Sona

// Figma "Player" (247:445): the title bar, video and controls in a natively behaving window.
Window {
    id: mainWindow

    // Launching with media opens maximized; an empty launch opens at the default size.
    required property bool opensMaximized
    property int visibilityBeforeFullScreen: Window.Windowed

    function toggleFullScreen() {
        if (visibility === Window.FullScreen) {
            visibility = visibilityBeforeFullScreen
        } else {
            visibilityBeforeFullScreen = visibility
            visibility = Window.FullScreen
        }
    }

    width: 1280
    height: 720
    // Shown once the window agent has taken over the frame.
    visible: false
    color: Theme.bgA3
    title: Player.title || Qt.application.name

    Component.onCompleted: {
        windowAgent.attach(mainWindow)
        windowAgent.setTitleBar(titleBar)
        windowAgent.setSystemButton(WindowAgent.WindowIcon, titleBar.logo)
        windowAgent.setSystemButton(WindowAgent.Minimize, titleBar.minimizeButton)
        windowAgent.setSystemButton(WindowAgent.Maximize, titleBar.maximizeButton)
        windowAgent.setSystemButton(WindowAgent.Close, titleBar.closeButton)
        for (const control of titleBar.interactiveControls)
            windowAgent.setHitTestVisible(control)
        // Size constraints go after setup, as the agent requires.
        minimumWidth = 640
        minimumHeight = 360
        opensMaximized ? showMaximized() : show()
        onboarding.active = !AppSettings.onboardingCompleted
    }

    AppWindowAgent {
        id: windowAgent
    }

    // Declared first so the bars paint over the outer half of its rounded frame.
    VideoView {
        anchors.top: titleBar.bottom
        anchors.bottom: controlBar.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 1
        anchors.leftMargin: 10
        anchors.rightMargin: 10
    }

    TitleBar {
        id: titleBar

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        onFullScreenToggled: mainWindow.toggleFullScreen()
    }

    ControlBar {
        id: controlBar

        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
    }

    DropArea {
        anchors.fill: parent
        onDropped: drop => {
            if (drop.hasUrls)
                Player.open(drop.urls[0])
        }
    }

    KeyboardShortcuts {
        id: keyboardShortcuts

        onFullScreenToggled: mainWindow.toggleFullScreen()
    }

    FileDialog {
        id: openFileDialog

        currentFolder: AppSettings.mediaFolder
        onAccepted: Player.open(selectedFile)
    }

    Shortcut {
        sequences: [StandardKey.Open]
        onActivated: openFileDialog.open()
    }

    Shortcut {
        sequence: "Esc"
        enabled: mainWindow.visibility === Window.FullScreen
        onActivated: mainWindow.toggleFullScreen()
    }

    Loader {
        id: onboarding

        active: false
        sourceComponent: OnboardingWindow {
            transientParent: mainWindow
            shortcutGroups: keyboardShortcuts.groups
            onFinished: {
                AppSettings.onboardingCompleted = true
                onboarding.active = false
            }
        }
    }
}

import QWindowKit
import Sona

// Native window behavior for a window with a custom title bar; the native frame follows the app theme.
WindowAgent {
    readonly property bool darkMode: Theme.dark
    // Window attributes need a window, so changes before attach() are skipped.
    property bool attached: false

    function attach(window) {
        setup(window)
        attached = true
        setWindowAttribute("dark-mode", darkMode)
    }

    onDarkModeChanged: if (attached) setWindowAttribute("dark-mode", darkMode)
}

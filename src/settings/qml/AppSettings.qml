pragma Singleton

import QtCore

// User preferences persisted across launches.
Settings {
    property bool onboardingCompleted: false
    property string theme: "system" // "light", "dark" or "system"
    property int accentIndex: 0
    property url mediaFolder: StandardPaths.writableLocation(StandardPaths.MoviesLocation)
}

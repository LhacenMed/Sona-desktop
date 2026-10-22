import QtQuick
import Sona

// Every player shortcut, in the order of the onboarding guide (Figma 1902:6770): one list drives
// both the key bindings and the guide. A binding without `sequence` is listed in the guide but has
// no feature behind it yet.
Item {
    id: shortcuts

    signal fullScreenToggled()

    readonly property var groups: [
        { title: "Play / Pause", bindings: [
            { keys: ["Space"], sequence: "Space", run: () => Player.togglePause() }] },
        { title: "Seek Backward / Forward", bindings: [
            { keys: ["←"], sequence: "Left", run: () => Player.seekBy(-10) },
            { keys: ["→"], sequence: "Right", run: () => Player.seekBy(10) }] },
        { title: "Previous / Next Item", bindings: [
            { keys: ["<"] },
            { keys: [">"] }] },
        { title: "Previous / Next Frame", bindings: [
            { keys: [","], sequence: ",", run: () => Player.stepFrame(false) },
            { keys: ["."], sequence: ".", run: () => Player.stepFrame(true) }] },
        { title: "Cycle Subtitle Track", bindings: [
            { keys: ["V"], sequence: "V", run: () => Player.cycleSubtitleTrack() }] },
        { title: "Cycle Audio Track", bindings: [
            { keys: ["B"], sequence: "B", run: () => Player.cycleAudioTrack() }] },
        { title: "Toggle Fullscreen", bindings: [
            { keys: ["F"], sequence: "F", run: () => shortcuts.fullScreenToggled() }] },
        { title: "Increase / Decrease Volume", bindings: [
            { keys: ["↑"], sequence: "Up", run: () => Player.volume = Math.min(Player.volume + 5, 200) },
            { keys: ["↓"], sequence: "Down", run: () => Player.volume = Math.max(Player.volume - 5, 0) }] },
        { title: "Show / Hide Queue", bindings: [
            { keys: ["Q"] }] },
        { title: "Show / Hide File Browser", bindings: [
            { keys: ["L"] }] },
        { title: "Take a Snapshot", bindings: [
            { keys: ["SHIFT", "S"], sequence: "Shift+S", run: () => Player.takeSnapshot() }] },
        { title: "Record", bindings: [
            { keys: ["SHIFT", "R"] }] }
    ]

    Repeater {
        model: shortcuts.groups.reduce((all, group) => all.concat(group.bindings), [])
                                .filter(binding => binding.sequence)

        delegate: Item {
            required property var modelData

            Shortcut {
                sequence: modelData.sequence
                onActivated: modelData.run()
            }
        }
    }
}

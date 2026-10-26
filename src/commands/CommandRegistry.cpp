#include "commands/CommandRegistry.h"

#include <algorithm>

bool CommandRegistry::runShortcut(KeyChord chord) const
{
    for (const auto &[id, command] : m_commands) {
        if (!command.run || std::ranges::find(command.shortcuts, chord) == command.shortcuts.end())
            continue;
        if (command.enabled && !command.enabled())
            continue;
        command.run();
        return true;
    }
    return false;
}

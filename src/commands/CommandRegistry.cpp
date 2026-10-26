#include "commands/CommandRegistry.h"

#include <algorithm>

std::wstring CommandRegistry::label(CommandId id) const
{
    const Command &entry = command(id);
    return entry.active && entry.active() ? entry.activeLabel : entry.label;
}

bool CommandRegistry::canRun(CommandId id) const
{
    const Command &entry = command(id);
    return entry.run && (!entry.enabled || entry.enabled());
}

void CommandRegistry::run(CommandId id) const
{
    if (canRun(id))
        command(id).run();
}

std::optional<CommandId> CommandRegistry::commandFor(KeyChord chord) const
{
    for (const auto &[id, entry] : m_commands) {
        if (std::ranges::find(entry.shortcuts, chord) != entry.shortcuts.end() && canRun(id))
            return id;
    }
    return {};
}

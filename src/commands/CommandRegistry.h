#pragma once

#include <map>

#include "commands/Command.h"

// The one list of commands: every shortcut runs through it, and everything that shows a command
// (the shortcut guide, and later menus, the shortcuts dialog and customizable bars) reads it here.
class CommandRegistry
{
public:
    void define(CommandId id, Command command) { m_commands[id] = std::move(command); }
    const Command &command(CommandId id) const { return m_commands.at(id); }

    // Runs the command bound to `chord` if it can run now; returns whether one ran.
    bool runShortcut(KeyChord chord) const;

private:
    std::map<CommandId, Command> m_commands;
};

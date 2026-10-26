#pragma once

#include <map>
#include <optional>
#include <string>

#include "commands/Command.h"

// The one list of commands: every shortcut runs through it, and everything that shows a command
// (menus, the bars' buttons, the shortcut guide, on-screen messages, and later the shortcuts dialog
// and Customize Player) reads it here.
class CommandRegistry
{
public:
    void define(CommandId id, Command command) { m_commands[id] = std::move(command); }
    const Command &command(CommandId id) const { return m_commands.at(id); }

    // Its name for the current state.
    std::wstring label(CommandId id) const;
    // Whether it has a feature behind it and can run now.
    bool canRun(CommandId id) const;
    void run(CommandId id) const;
    // The command `chord` runs now, if any.
    std::optional<CommandId> commandFor(KeyChord chord) const;

private:
    std::map<CommandId, Command> m_commands;
};

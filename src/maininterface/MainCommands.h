#pragma once

class Bookmarks;
class CommandRegistry;
class MainWindow;
class Player;
class Queue;

// Defines the player's commands and their default shortcuts. The keys follow the menus in Figma
// (6062:13354, 6065:13616), so the guide, the menus and the keyboard agree.
void defineMainCommands(CommandRegistry &commands, MainWindow &window, Player &player, Queue &queue, Bookmarks &bookmarks);

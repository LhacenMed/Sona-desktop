#pragma once

#include <functional>

class CommandRegistry;
class History;
class SettingsStore;
class Thumbnails;
struct LibraryItem;

// What the library's views work with.
struct LibraryContext
{
    History &history;
    Thumbnails &thumbnails;
    SettingsStore &settings;
    const CommandRegistry &commands;
    // Plays an item in place of the queue.
    std::function<void(const LibraryItem &item)> open;
};

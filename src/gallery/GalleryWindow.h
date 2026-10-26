#pragma once

#include "platform/Window.h"

class SettingsStore;

// A test page with every toolkit widget, for checking them against Figma in both themes. Debug
// builds only: sona.exe --gallery.
class GalleryWindow : public Window
{
public:
    GalleryWindow(GraphicsDevice &graphics, Theme &theme, SettingsStore &settings);

private:
    void destroyed() override;
};

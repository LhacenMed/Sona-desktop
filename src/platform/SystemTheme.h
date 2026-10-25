#pragma once

// Whether Windows is set to light mode for apps (Settings > Personalization > Colors). Windows
// announces changes with WM_SETTINGCHANGE "ImmersiveColorSet".
bool systemPrefersLightTheme();

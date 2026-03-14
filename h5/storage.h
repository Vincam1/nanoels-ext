#pragma once

#include "globals.h"
#include <Preferences.h>

// Loads all hardware config from Preferences, falling back to config.h defaults.
// Must be called once in setup() before initAxis().
void loadHardwareSettings();

// Saves hardware config to Preferences. Called by settings page on user confirmation.
void saveHardwareSettings();

// Saves runtime state (positions, mode, etc.) if anything has changed since last save.
// Returns true if a write was performed.
bool saveIfChanged();

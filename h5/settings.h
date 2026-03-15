#pragma once

#include "globals.h"

// ============================================================
// HMI SETTINGS PAGE
//
// Firmware side of the Nextion settings page (page 2).
//
// When the user opens the settings page, openSettingsPage()
// pushes all current hardware config values to the display
// text fields so the user can see and edit them.
//
// When the user taps "Save", the Nextion sends back a
// B_SETTINGS_SAVE event. saveSettingsFromDisplay() then reads
// the values back from the display components, writes them to
// Preferences, and reboots so the new config takes effect.
//
// HMI COMPONENT NAMES (to be created in Nextion Editor):
//   tEncPPR    — Encoder PPR          (number, t0)
//   tEncBL     — Encoder backlash     (number, t1)
//   tScrewZ    — Z screw pitch (du)   (number, t2)
//   tMotorZ    — Z motor steps        (number, t3)
//   tBLZ       — Z backlash (du)      (number, t4)
//   tScrewX    — X screw pitch (du)   (number, t5)
//   tMotorX    — X motor steps        (number, t6)
//   tBLX       — X backlash (du)      (number, t7)
//   tMotorY    — Y motor steps        (number, t8)
//   tScrewY    — Y screw pitch (du)   (number, t9)
//   bSave      — Save button          (id=10 → B_SETTINGS_SAVE)
//   bCancel    — Cancel button        (id=11 → B_SETTINGS_CANCEL)
//   bMeasureBLZ — Measure Z backlash  (id=12 → B_MEASURE_BL_Z)
//   bMeasureBLX — Measure X backlash  (id=13 → B_MEASURE_BL_X)
//   tSettingsStatus — Status line (read-only, shows results/errors)
//
// Bool toggles (invertZ, invertX, etc.) can be added as
// checkbox or toggle button components using additional ids.
// ============================================================

// Open the settings page: navigate Nextion to page 2 and push
// all current hardware config values to the display fields.
void openSettingsPage();

// Called when the user confirms changes on the HMI settings page.
// Reads values from Nextion display components, saves to Preferences,
// then triggers a reboot so the new hardware config takes effect.
void saveSettingsFromDisplay();

// Read a number back from a Nextion text component.
// Returns defaultValue if the component text is empty or invalid.
long readNumberFromDisplay(const String& componentId, long defaultValue);

// Backlash calibration via DRO scale.
// Pulses the axis in the negative direction (reversal from positive jog)
// until the scale detects movement, then saves the result to Preferences.
// Displays status on tSettingsStatus. Machine must be OFF before calling.
void measureAndSaveBacklashZ();
void measureAndSaveBacklashX();

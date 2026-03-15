#pragma once

#include "globals.h"

// Set the pitch (dupr) — deferred until motionMutex is available
void setDupr(long value);
void applyDupr();

// Set the number of thread starts — deferred
void setStarts(int value);
void applyStarts();

// Set the mode — deferred (setModeFromTask) or immediate (setModeFromLoop)
void setModeFromTask(int value);
void setModeFromLoop(int value);

// Set turn/face pass count
void setTurnPasses(int value);

// Set cone ratio — deferred
void setConeRatio(float value);
void applyConeRatio();

// Taper preset lookup (16 entries: MT0-MT7, JT0-JT6, JT33)
extern const int TAPER_PRESET_COUNT;
const char* taperPresetName(int idx);
float       taperPresetRatio(int idx);

// Normalize a pitch value to remove sub-precision noise
long normalizePitch(long pitch);

// Reset all state to defaults (held off button for 3+ seconds)
void reset();

// Set the measurement unit
void setMeasure(int value);

// Soft stop management
void setLeftStop(Axis* a, long value);
void setRightStop(Axis* a, long value);
void applyLeftStop(Axis* a);
void applyRightStop(Axis* a);
void leaveStop(Axis* a, long oldStop);

// Apply all pending deferred settings (call while holding motionMutex in loop())
void applySettings();

// Operating mode implementations (called from loop())
void modeGearbox();
void modeTurn(Axis* main, Axis* aux);
void modeFace();
void modeCone();
void modeTaper();
void modeCut();
void modeGroove();
void modeGrooveStraight();
void modeEllipse(Axis* main, Axis* aux);

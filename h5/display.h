#pragma once

#include "globals.h"
#include <Arduino.h>

// Send a raw command string to the Nextion display
void toScreen(const String& command);

// Set the text of a named Nextion component
void setText(const String& id, const String& text);

// Formatting helpers
String printDeciMicrons(long deciMicrons, int precisionPointsMax);
String printDegrees(long degrees10000);
String printDupr(long value);
String printAxisPos(Axis* a);
String printDistanceToLeftStop(Axis* a);
String printDistanceToRightStop(Axis* a);
String printAxisStopDiff(Axis* a, bool addTrailingSpace);
String printNoTrailing0(float value);
String printMode();

// Mode / state queries used by display
bool needZStops();
bool isPassMode();
bool manualMovesAllowedWhenOn();
bool manualMovesIgnoredWhenOn();
int  getLastSetupIndex();
Axis* getPitchAxis();
long getPassModeZStart();
long getPassModeXStart();
long getNumpadResult();
float numpadToConeRatio();
long numpadToDeciMicrons();
long spindleModulo(long value);

// Spindle / axis utilities used across modules
long stepsToDu(Axis* a, long steps);
long getAxisPosDu(Axis* a);
long getAxisStopDiffDu(Axis* a);
long getAxisLeftStopDistanceDu(Axis* a);
long getAxisRightStopDistanceDu(Axis* a);
int  getApproxRpm();
bool stepperIsRunning(Axis* a);

// Main display refresh (call from taskDisplay loop)
void updateDisplay();

// Buzzer
void beep();

// FreeRTOS task
void taskDisplay(void* param);

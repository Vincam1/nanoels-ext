#pragma once

#include "globals.h"
#include <LittleFS.h>

// File management
int    getGcodeProgramCount();
bool   saveGcode();
bool   removeGcodeByName(const String& name);
bool   removeAllGcode();
String readGcodeProgram(const String& name);
String getCurrentGcodeProgramName();

// Command parsing
String getValueString(const String& command, char letter);
float  getFloat(const String& command, char letter);
int    getInt(const String& command, char letter);
void   setFeedRate(const String& command);
long   mmOrInchToAbsolutePos(Axis* a, float mmOrInch);
void   updateAxisSpeeds(long diffX, long diffZ, long diffY);

// Motion synchronisation
void gcodeWaitEpsilon(int epsilon);
void gcodeWaitNear();
void gcodeWaitStop();

// G/M code handlers
void G00_01(const String& command);
bool handleGcode(const String& command);
bool handleMcode(const String& command);
bool handleGcodeCommand(String command);

// FreeRTOS task
void taskGcode(void* param);

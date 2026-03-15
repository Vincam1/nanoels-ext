#pragma once

#include "config.h"
#include "types.h"
#include <Arduino.h>
#include <PS2KeyAdvanced.h>

// ============================================================
// RUNTIME HARDWARE CONFIGURATION
// Loaded from Preferences on boot; defaults come from config.h.
// Can be updated via the HMI settings page.
// ============================================================

// Encoder
extern int   encoderPPR;
extern int   encoderBacklash;

// Derived encoder constants (computed once after loading encoderPPR)
extern int   encoderStepsInt;   // encoderPPR * 2
extern float encoderStepsFloat; // float copy of encoderStepsInt
extern long  rpmBulk;           // Averaging window for RPM calculation

// Z axis hardware settings
extern long  screwZDu;
extern long  motorStepsZ;
extern bool  invertZ;
extern bool  invertZEnable;
extern bool  needsRestZ;
extern long  maxTravelMmZ;
extern long  backlashDuZ;

// X axis hardware settings
extern long  screwXDu;
extern long  motorStepsX;
extern bool  invertX;
extern bool  invertXEnable;
extern bool  needsRestX;
extern long  maxTravelMmX;
extern long  backlashDuX;

// Y axis hardware settings
extern bool  activeY;
extern bool  rotaryY;
extern long  motorStepsY;
extern long  screwYDu;
extern long  speedStartY;
extern long  accelerationY;
extern long  speedManualMoveY;
extern bool  invertY;
extern bool  invertYEnable;
extern bool  needsRestY;
extern long  maxTravelMmY;
extern long  backlashDuY;

// ============================================================
// AXIS OBJECTS
// ============================================================
extern Axis z;
extern Axis x;
extern Axis y;

// ============================================================
// RUNTIME STATE
// ============================================================

extern SemaphoreHandle_t motionMutex;

// Spindle / encoder state
extern unsigned long spindleEncTime;
extern unsigned long spindleEncTimeDiffBulk;
extern unsigned long spindleEncTimeAtIndex0;
extern int           spindleEncTimeIndex;
extern long          spindlePos;
extern long          spindlePosAvg;
extern long          savedSpindlePosAvg;
extern long          savedSpindlePos;
extern int           spindleCount;
extern int           spindlePosSync;
extern int           savedSpindlePosSync;
extern long          spindlePosGlobal;
extern long          savedSpindlePosGlobal;

// Display / RPM
extern bool          showAngle;
extern bool          showTacho;
extern bool          savedShowAngle;
extern bool          savedShowTacho;
extern int           shownRpm;
extern unsigned long shownRpmTime;
extern long          lcdHashLine0;
extern long          lcdHashLine1;
extern long          lcdHashLine2;
extern long          lcdHashLine3;
extern bool          splashScreen;

// On/off state
extern bool          isOn;
extern bool          nextIsOn;
extern bool          nextIsOnFlag;
extern unsigned long resetMillis;
extern int           emergencyStop;
extern bool          beepFlag;

// Pitch / threading
extern long  dupr;
extern long  savedDupr;
extern long  nextDupr;
extern bool  nextDuprFlag;
extern int   starts;
extern int   savedStarts;
extern int   nextStarts;
extern bool  nextStartsFlag;

// Mode
extern volatile int mode;
extern int          nextMode;
extern bool         nextModeFlag;
extern int          savedMode;

// Measurement units
extern int measure;
extern int savedMeasure;

// Cone
extern float coneRatio;
extern float savedConeRatio;
extern float nextConeRatio;
extern bool  nextConeRatioFlag;

// Pass / automation
extern int   turnPasses;
extern int   savedTurnPasses;
extern long  setupIndex;
extern bool  auxForward;
extern bool  savedAuxForward;
extern long  opIndex;
extern bool  opIndexAdvanceFlag;
extern long  opSubIndex;
extern int   opDuprSign;
extern long  opDupr;

// Manual step size
extern long moveStep;
extern long savedMoveStep;

// Manual button state
extern bool buttonLeftPressed;
extern bool buttonRightPressed;
extern bool buttonUpPressed;
extern bool buttonDownPressed;
extern bool buttonOffPressed;
extern bool buttonBackPressed;
extern bool buttonForwardPressed;

// Numpad
extern bool inNumpad;
extern int  numpadDigits[20];
extern int  numpadIndex;

// GCode
extern String gcodeCommand;
extern long   gcodeFeedDuPerSec;
extern bool   gcodeInitialized;
extern bool   gcodeAbsolutePositioning;
extern bool   gcodeInBrace;
extern bool   gcodeInSemicolon;
extern bool   wsInKeycode;
extern int    wsKeycode;
extern String keycodeCommand;
extern bool   gcodeInSave;
extern bool   gcodeInSaveFirstLine;
extern String gcodeSaveName;
extern String gcodeSaveValue;
extern int    gcodeProgramIndex;
extern int    gcodeProgramCount;
extern String gcodeProgram;
extern int    gcodeProgramCharIndex;

// WiFi / WebSocket buffers
extern CircleBuffer inBuffer;
extern CircleBuffer outBuffer;
extern String       wifiStatus;
extern unsigned long wifiStatusMillis;

// Timing
extern unsigned long saveTime;
extern unsigned long lastDisplayUpdateTime;
extern unsigned long keypadTimeUs;

// Async timer
extern hw_timer_t* async_timer;
extern bool        timerAttached;

// Keyboard
extern PS2KeyAdvanced keyboard;

// Nextion protocol buffer
extern const int NEXTION_BUFFER_LENGTH;
extern byte      nextionBuffer[];
extern int       nextionBufferIndex;
extern byte      lastNextionPageId;

// Multi-start button timing
extern unsigned long multistartPressMillis;

// ============================================================
// DRO (LINEAR SCALE) STATE
// ============================================================
extern bool zDroActive; // Scale is physically connected and should be read
extern bool xDroActive;
extern bool zDroMode;   // true = display scale position; false = display stepper position
extern bool xDroMode;

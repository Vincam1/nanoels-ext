#pragma once

#include "globals.h"

// Initialise an Axis struct with all hardware parameters.
// Call once in setup() after loading hardware settings from Preferences.
void initAxis(Axis* a, char name, bool active, bool rotational,
              float motorSteps, float screwPitch,
              long speedStart, long speedManualMove, long acceleration,
              bool invertStepper, bool invertEnable, bool needsRest,
              long maxTravelMm, long backlashDu,
              int ena, int dir, int step,
              int pulseA, int pulseB, pcnt_unit_t pulseUnit);

// Enable/disable a stepper driver (reference counted for needsRest axes)
void stepperEnable(Axis* a, bool value);
void updateEnable(Axis* a);

// Emergency stop
void setEmergencyStop(int kind);

// Async timer
void setAsyncTimerEnable(bool value);
void updateAsyncTimerSettings();

// On/off control (call from task context or from main loop respectively)
void setIsOnFromTask(bool on);
void setIsOnFromLoop(bool on);

// Origin management
void markAxisOrigin(Axis* a);
void zeroSpindlePos();
void markOrigin();
void markAxis0(Axis* a);

// Motion helpers
bool stepTo(Axis* a, long newPos, bool continuous);
bool stepToContinuous(Axis* a, long newPos);
bool stepToFinal(Axis* a, long newPos);
void setDir(Axis* a, bool dir);
void moveAxis(Axis* a);

// Spindle <-> stepper position conversion
long posFromSpindle(Axis* a, long s, bool respectStops);
long spindleFromPos(Axis* a, long p);

// Manual move helpers
void waitForPendingPosNear0(Axis* a);
void waitForPendingPos0(Axis* a);
bool isContinuousStep();
long getMoveStepForAxis(Axis* a);
long getStepMaxSpeed(Axis* a);
void waitForStep(Axis* a);
int  getAndResetPulses(Axis* a);

// Async axis selection
Axis* getAsyncAxis();
unsigned int getTimerLimit();

// Async timer ISR (must be in IRAM)
void IRAM_ATTR onAsyncTimer();

// Pulse counter setup
void startPulseCounter(pcnt_unit_t unit, int gpioA, int gpioB);

// FreeRTOS tasks
void taskMoveZ(void* param);
void taskMoveX(void* param);
void taskMoveY(void* param);
void taskAttachInterrupts(void* param);

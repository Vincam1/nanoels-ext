#pragma once

#include "globals.h"

// Button press handlers
void buttonPlusMinusPress(bool plus);
void buttonOnOffPress(bool on);
void buttonOffRelease();
void buttonLeftStopPress(Axis* a);
void buttonRightStopPress(Axis* a);
void buttonDisplayPress();
void buttonMoveStepPress();
void buttonMeasurePress();
void buttonReversePress();
void buttonMultistartPress();

// Numpad
void numpadPress(int digit);
void numpadBackspace();
void resetNumpad();
void numpadPlusMinus(bool plus);
bool processNumpadResult(int keyCode);
bool processNumpad(int keyCode);

// Nextion protocol
bool checkForTerminator();
int  processNextionMessage();
void setModeFromUi(int modeToSet, bool eventFromNextion);

// Main keypad event dispatcher (called from taskKeypad)
void processKeypadEvent();

// FreeRTOS task
void taskKeypad(void* param);

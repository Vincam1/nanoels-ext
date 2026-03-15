#include "input.h"
#include "display.h"
#include "stepper.h"
#include "modes.h"
#include "settings.h"
#include "gcode.h"
#include "dro.h"
#include "storage.h"

// ============================================================
// BUTTON HANDLERS
// ============================================================

void buttonPlusMinusPress(bool plus) {
  bool minus = !plus;
  if (mode == MODE_THREAD && setupIndex == 2) {
    if (minus && starts > 2)          setStarts(starts - 1);
    else if (plus && starts < STARTS_MAX) setStarts(starts + 1);
  } else if (mode == MODE_TAPER && setupIndex == 1) {
    if (plus  && taperPreset < TAPER_PRESET_COUNT - 1) taperPreset++;
    else if (minus && taperPreset > 0)                  taperPreset--;
    else beepFlag = true;
  } else if (isPassMode() && setupIndex == 1 && getNumpadResult() == 0) {
    if (minus && turnPasses > 1)          setTurnPasses(turnPasses - 1);
    else if (plus && turnPasses < PASSES_MAX) setTurnPasses(turnPasses + 1);
  } else if (measure != MEASURE_TPI) {
    int delta             = measure == MEASURE_METRIC ? MOVE_STEP_3 : MOVE_STEP_IMP_3;
    long normalizedDupr   = normalizePitch(dupr);
    if (minus && dupr > -DUPR_MAX)  setDupr(max(-DUPR_MAX, normalizedDupr - delta));
    else if (plus && dupr < DUPR_MAX)   setDupr(min(DUPR_MAX, normalizedDupr + delta));
  } else { // TPI
    if (dupr == 0) {
      setDupr(plus ? 1 : -1);
    } else {
      long currentTpi = round(254000.0 / dupr);
      long tpi        = currentTpi + (plus ? 1 : -1);
      long newDupr    = tpi == 0 ? (plus ? DUPR_MAX : -DUPR_MAX) : round(254000.0 / tpi);
      if (newDupr == dupr) newDupr += plus ? -1 : 1;
      if (newDupr != dupr && newDupr < DUPR_MAX && newDupr > -DUPR_MAX) setDupr(newDupr);
    }
  }
}

void buttonOnOffPress(bool on) {
  resetMillis = millis();
  bool missingZStops = needZStops() && (z.leftStop == LONG_MAX || z.rightStop == LONG_MIN);
  if (on && isPassMode() && (missingZStops || x.leftStop == LONG_MAX || x.rightStop == LONG_MIN)) {
    beepFlag = true;
  } else if (!isOn && on && mode == MODE_GCODE && gcodeProgramIndex >= gcodeProgramCount && setupIndex == 1) {
    beepFlag = true;
  } else if (!isOn && on && setupIndex < getLastSetupIndex()) {
    if (mode == MODE_THREAD && setupIndex == 3) setConeRatio(0);
    if (mode == MODE_TAPER  && setupIndex == 1) setConeRatio(taperPresetRatio(taperPreset));
    setupIndex++;
  } else if (isOn && on && (mode == MODE_TURN || mode == MODE_FACE || mode == MODE_THREAD)) {
    opIndexAdvanceFlag = true;
  } else if (!on && (z.movingManually || x.movingManually || y.movingManually)) {
    setEmergencyStop(ESTOP_OFF_MANUAL_MOVE);
  } else if (!isOn && on && mode == MODE_GCODE && gcodeProgramIndex >= gcodeProgramCount) {
    beepFlag = true;
  } else if (!isOn && on && mode == MODE_GCODE) {
    String name = getCurrentGcodeProgramName();
    if (name.length() == 0) {
      beepFlag = true;
    } else {
      gcodeProgramCharIndex = 0;
      gcodeProgram          = readGcodeProgram(name);
      if (gcodeProgram.length() == 0) {
        beepFlag = true;
      } else {
        gcodeProgram += '\n';
        setIsOnFromTask(true);
      }
    }
  } else {
    setIsOnFromTask(on);
  }
}

void buttonOffRelease() {
  if (millis() - resetMillis > 3000) {
    reset();
    splashScreen = true;
  }
}

void buttonLeftStopPress(Axis* a) {
  setLeftStop(a, a->leftStop == LONG_MAX ? a->pos : LONG_MAX);
}

void buttonRightStopPress(Axis* a) {
  setRightStop(a, a->rightStop == LONG_MIN ? a->pos : LONG_MIN);
}

void buttonDisplayPress() {
  if (!showAngle && !showTacho)     showAngle = true;
  else if (showAngle) { showAngle = false; showTacho = true; }
  else                               showTacho = false;
}

void buttonMoveStepPress() {
  if (measure == MEASURE_METRIC) {
    if (moveStep == MOVE_STEP_1)      moveStep = MOVE_STEP_2;
    else if (moveStep == MOVE_STEP_2) moveStep = MOVE_STEP_3;
    else                              moveStep = MOVE_STEP_1;
  } else {
    if (moveStep == MOVE_STEP_IMP_1)      moveStep = MOVE_STEP_IMP_2;
    else if (moveStep == MOVE_STEP_IMP_2) moveStep = MOVE_STEP_IMP_3;
    else                                  moveStep = MOVE_STEP_IMP_1;
  }
}

void buttonMeasurePress() {
  if (measure == MEASURE_METRIC)     setMeasure(MEASURE_INCH);
  else if (measure == MEASURE_INCH)  setMeasure(MEASURE_TPI);
  else                               setMeasure(MEASURE_METRIC);
}

void buttonReversePress() {
  setDupr(-dupr);
}

void buttonMultistartPress() {
  if (millis() - multistartPressMillis > 3000 && starts > 1) setStarts(1);
  else                                                         setStarts(starts + 1);
  multistartPressMillis = millis();
}

// ============================================================
// NUMPAD
// ============================================================

void numpadPress(int digit) {
  if (!inNumpad) numpadIndex = 0;
  numpadDigits[numpadIndex] = digit;
  if (numpadIndex < 7) numpadIndex++;
  else                 numpadIndex = 0;
}

void numpadBackspace() {
  if (inNumpad && numpadIndex > 0) numpadIndex--;
}

void resetNumpad() {
  numpadIndex = 0;
}

void numpadPlusMinus(bool plus) {
  if (numpadDigits[numpadIndex - 1] < 9 && plus)  numpadDigits[numpadIndex - 1]++;
  else if (numpadDigits[numpadIndex - 1] > 1 && !plus) numpadDigits[numpadIndex - 1]--;
}

bool processNumpadResult(int keyCode) {
  long  newDu          = numpadToDeciMicrons();
  float newConeRatio   = numpadToConeRatio();
  long  numpadResult   = getNumpadResult();
  resetNumpad();

  if (keyCode == B_ON) {
    if (isPassMode() && setupIndex == 1) {
      setTurnPasses(int(min(PASSES_MAX, numpadResult)));
      setupIndex++;
    } else if (mode == MODE_THREAD && setupIndex == 3) {
      setConeRatio(newConeRatio);
      setupIndex++;
    } else if (mode == MODE_CONE && setupIndex == 1) {
      setConeRatio(newConeRatio);
      setupIndex++;
    } else {
      if (abs(newDu) <= DUPR_MAX) setDupr(newDu);
    }
    return true;
  }

  Axis* a   = (keyCode == B_STOPL || keyCode == B_STOPR || keyCode == B_LEFT || keyCode == B_RIGHT || keyCode == B_Z) ? &z : &x;
  int   sign = ((keyCode == B_STOPL || keyCode == B_STOPU || keyCode == B_LEFT || keyCode == B_UP || keyCode == B_Z || keyCode == B_X || keyCode == B_X_ENA) ? 1 : -1);
  if (keyCode == B_STOPF || keyCode == B_STOPB || keyCode == B_FORWARD || keyCode == B_BACK || keyCode == B_Y) {
    a    = &y;
    sign = (keyCode == B_BACK || keyCode == B_STOPB) ? -1 : 1;
  }
  long posDiffAbs = (a->rotational ? numpadResult * 10 : newDu) / a->screwPitch * a->motorSteps;
  long pos        = a->pos + posDiffAbs * sign;

  if (keyCode == B_STOPL) { setLeftStop(&z, pos);  return true; }
  if (keyCode == B_STOPR) { setRightStop(&z, pos); return true; }
  if (keyCode == B_STOPU) { setLeftStop(&x, pos);  return true; }
  if (keyCode == B_STOPD) { setRightStop(&x, pos); return true; }
  if (keyCode == B_STOPF && activeY) { setLeftStop(&y, pos);  return true; }
  if (keyCode == B_STOPB && activeY) { setRightStop(&y, pos); return true; }

  if (!isOn && (keyCode == B_LEFT || keyCode == B_RIGHT || keyCode == B_UP || keyCode == B_DOWN || keyCode == B_FORWARD || keyCode == B_BACK)) {
    if (pos < a->rightStop)         { pos = a->rightStop; beepFlag = true; }
    else if (pos > a->leftStop)     { pos = a->leftStop;  beepFlag = true; }
    else if (abs(pos - a->pos) > a->estopSteps) { beepFlag = true; return true; }
    a->speedMax = a->speedManualMove;
    stepToFinal(a, pos);
    return true;
  }

  if (keyCode == B_Z) {
    if (zDroActive && zDroMode) zScale.setPosition(numpadToDeciMicrons() / 10000.0f);
    else                        a->originPos = -pos;
    return true;
  }
  if (keyCode == B_X) {
    if (xDroActive && xDroMode) xScale.setPosition(numpadToDeciMicrons() / 10000.0f);
    else                        a->originPos = -pos;
    return true;
  }
  if (keyCode == B_Y) { a->originPos = -pos; return true; }
  if (keyCode == B_DIAMETER || keyCode == B_X_ENA) {
    if (xDroActive && xDroMode) xScale.setPosition(numpadToDeciMicrons() / 20000.0f); // diameter → radius
    else                        a->originPos = -a->pos - posDiffAbs / 2;
    return true;
  }
  if (keyCode == B_STEP) {
    if (newDu > 0) moveStep = newDu;
    else           beepFlag = true;
    return true;
  }
  return false;
}

bool processNumpad(int keyCode) {
  if      (keyCode == B_0) { numpadPress(0); inNumpad = true; }
  else if (keyCode == B_1) { numpadPress(1); inNumpad = true; }
  else if (keyCode == B_2) { numpadPress(2); inNumpad = true; }
  else if (keyCode == B_3) { numpadPress(3); inNumpad = true; }
  else if (keyCode == B_4) { numpadPress(4); inNumpad = true; }
  else if (keyCode == B_5) { numpadPress(5); inNumpad = true; }
  else if (keyCode == B_6) { numpadPress(6); inNumpad = true; }
  else if (keyCode == B_7) { numpadPress(7); inNumpad = true; }
  else if (keyCode == B_8) { numpadPress(8); inNumpad = true; }
  else if (keyCode == B_9) { numpadPress(9); inNumpad = true; }
  else if (keyCode == B_BACKSPACE) { numpadBackspace(); inNumpad = true; }
  else if (inNumpad && (keyCode == B_PLUS || keyCode == B_MINUS)) {
    numpadPlusMinus(keyCode == B_PLUS);
    return true;
  } else if (inNumpad) {
    inNumpad = false;
    return processNumpadResult(keyCode);
  }
  return inNumpad;
}

// ============================================================
// NEXTION PROTOCOL
// ============================================================

bool checkForTerminator() {
  if (nextionBufferIndex < 3) return false;
  return nextionBuffer[nextionBufferIndex - 3] == 0xFF &&
         nextionBuffer[nextionBufferIndex - 2] == 0xFF &&
         nextionBuffer[nextionBufferIndex - 1] == 0xFF;
}

// Page 0 button ID → keycode lookup table
static const byte HEX_TO_KEYCODE[256] = {
  [0]  = 0,
  [1]  = 0,
  [2]  = 0,
  [3]  = B_OFF,
  [4]  = B_MODE,
  [5]  = B_REVERSE,
  [6]  = B_MEASURE,
  [7]  = B_STEP,
  [8]  = 0,
  [9]  = B_OFF,        // tTurns
  [10] = B_OFF,        // tAngle
  [11] = B_X_ENA,
  [12] = B_X,
  [13] = 0,
  [14] = 0,
  [15] = B_Y_ENA,
  [16] = 0,
  [17] = B_Y,
  [18] = 0,
  [19] = B_Z_ENA,
  [20] = 0,
  [21] = B_Z,
  [22] = 0,
  [23] = B_OFF,
  [24] = B_BACKSPACE,
  [25] = B_ON,
  [26] = B_0,
  [27] = B_1,
  [28] = B_2,
  [29] = B_3,
  [30] = B_4,
  [31] = B_5,
  [32] = B_6,
  [33] = B_7,
  [34] = B_8,
  [35] = B_9,
  [36] = B_STOPU,
  [37] = B_STOPD,
  [38] = B_STOPF,
  [39] = B_STOPB,
  [40] = B_STOPL,
  [41] = B_STOPR,
  [42] = B_PLUS,
  [43] = B_MINUS,
  [44] = B_UP,
  [45] = B_DOWN,
  [46] = B_FORWARD,
  [47] = B_BACK,
  [48] = B_LEFT,
  [49] = B_RIGHT,
  [50] = B_MULTISTART,
  // Settings button on page 0 (add to Nextion HMI at id=51)
  [51] = B_SETTINGS_OPEN,
  // DRO toggle — tap tZ (id=52) or tX (id=53) display area on Nextion
  [52] = B_Z_DRO,
  [53] = B_X_DRO,
};

int processNextionMessage() {
  lastNextionPageId = 255;
  if (nextionBufferIndex < 6) return 0;
  if (nextionBuffer[0] != 0x65) return 0;

  byte pageId = nextionBuffer[1];
  int  code   = 0;

  if (pageId == 0x00) {
    code = HEX_TO_KEYCODE[nextionBuffer[2]];
  } else if (pageId == 0x01) {
    // Mode selection page
    switch (nextionBuffer[2]) {
      case 12: code = B_MODE_GEARS;   break;
      case 13: code = B_MODE_TURN;    break;
      case 14: code = B_MODE_FACE;    break;
      case 15: code = B_MODE_CONE;    break;
      case 16: code = B_MODE_CUT;     break;
      case 17: code = B_MODE_THREAD;  break;
      case 18: code = B_MODE_ELLIPSE; break;
      case 19: code = B_MODE_GCODE;   break;
      case 20: code = B_MODE_ASYNC;   break;
      case 21: code = B_MODE_Y;       break;
      case 22: code = B_MODE_TAPER;   break;
    }
  } else if (pageId == 0x02) {
    // Settings page
    switch (nextionBuffer[2]) {
      case 10: code = B_SETTINGS_SAVE;   break;
      case 11: code = B_SETTINGS_CANCEL; break;
      case 12: code = B_MEASURE_BL_Z;    break;
      case 13: code = B_MEASURE_BL_X;    break;
    }
  }

  if (code != 0) {
    lastNextionPageId = pageId;
    if (nextionBuffer[3] == 0) code |= PS2_BREAK;
  }
  return code;
}

void setModeFromUi(int modeToSet, bool eventFromNextion) {
  setModeFromTask(modeToSet);
  if (eventFromNextion && lastNextionPageId == 1) toScreen("page 0");
}

// ============================================================
// MAIN KEYPAD EVENT DISPATCHER
// ============================================================

void processKeypadEvent() {
  int  event           = 0;
  bool eventFromNextion = false;
  lastNextionPageId    = 255;

  if (wsKeycode != 0) {
    event     = wsKeycode;
    wsKeycode = 0;
  } else if (keyboard.available()) {
    event = keyboard.read();
  } else if (Serial1.available() > 0) {
    byte incomingByte = Serial1.read();
    if (nextionBufferIndex < NEXTION_BUFFER_LENGTH) {
      nextionBuffer[nextionBufferIndex++] = incomingByte;
    } else {
      nextionBufferIndex = 0;
    }
    if (checkForTerminator()) {
      event             = processNextionMessage();
      eventFromNextion  = event != 0 && lastNextionPageId != 255;
      nextionBufferIndex = 0;
    }
  }
  if (event == 0) return;

  int  keyCode = event & 0xFF;
  bool isPress = !(event & PS2_BREAK);
  keypadTimeUs = micros();

  // Uncomment to debug key codes on screen:
  // setText("t3", (isPress ? "Press " : "Release ") + String(keyCode));

  if (keyCode == 170) { keyboard.echo(); return; } // PS2 init handshake

  // Off is always handled
  if (keyCode == B_OFF) {
    buttonOffPressed = isPress;
    isPress ? buttonOnOffPress(false) : buttonOffRelease();
  }

  // Settings page open/save/cancel
  if (isPress && keyCode == B_SETTINGS_OPEN) {
    openSettingsPage();
    return;
  }
  if (isPress && keyCode == B_SETTINGS_SAVE) {
    saveSettingsFromDisplay();
    return;
  }
  if (isPress && keyCode == B_SETTINGS_CANCEL) {
    toScreen("page 0");
    return;
  }
  if (isPress && keyCode == B_MEASURE_BL_Z) {
    measureAndSaveBacklashZ();
    return;
  }
  if (isPress && keyCode == B_MEASURE_BL_X) {
    measureAndSaveBacklashX();
    return;
  }

  if (mode == MODE_GCODE && isOn) {
    if (isPress && keyCode != B_OFF) beepFlag = true;
    return;
  }

  if (isPress && processNumpad(keyCode)) return;

  buttonLeftPressed    = false;
  buttonRightPressed   = false;
  buttonUpPressed      = false;
  buttonDownPressed    = false;
  buttonBackPressed    = false;
  buttonForwardPressed = false;

  if (isPress && setupIndex == 2 && (keyCode == B_LEFT || keyCode == B_RIGHT)) {
    auxForward = !auxForward;
  } else if (isPress && mode == MODE_GCODE && setupIndex == 1 && (keyCode == B_UP || keyCode == B_DOWN)) {
    if (gcodeProgramIndex > 0 && keyCode == B_UP)                               gcodeProgramIndex--;
    else if (gcodeProgramIndex == 0 && gcodeProgramCount > 0 && keyCode == B_UP) gcodeProgramIndex = gcodeProgramCount - 1;
    else if ((gcodeProgramIndex < gcodeProgramCount - 1) && keyCode == B_DOWN)  gcodeProgramIndex++;
    else if (keyCode == B_DOWN)                                                   gcodeProgramIndex = 0;
  } else if (isPress && mode == MODE_GCODE && setupIndex == 1 && keyCode == B_MINUS) {
    removeGcodeByName(getCurrentGcodeProgramName());
    return;
  } else if (keyCode == B_LEFT)    { buttonLeftPressed    = isPress; }
  else if (keyCode == B_RIGHT)     { buttonRightPressed   = isPress; }
  else if (keyCode == B_UP)        { buttonUpPressed      = isPress; }
  else if (keyCode == B_DOWN)      { buttonDownPressed    = isPress; }
  else if (keyCode == B_FORWARD)   { buttonForwardPressed = isPress; }
  else if (keyCode == B_BACK)      { buttonBackPressed    = isPress; }

  if (!isPress) return;

  if      (keyCode == B_PLUS)          buttonPlusMinusPress(true);
  else if (keyCode == B_MINUS)         buttonPlusMinusPress(false);
  else if (keyCode == B_ON)            buttonOnOffPress(true);
  else if (keyCode == B_STOPL)         buttonLeftStopPress(&z);
  else if (keyCode == B_STOPR)         buttonRightStopPress(&z);
  else if (keyCode == B_STOPU)         buttonLeftStopPress(&x);
  else if (keyCode == B_STOPD)         buttonRightStopPress(&x);
  else if (keyCode == B_STOPF && activeY) buttonLeftStopPress(&y);
  else if (keyCode == B_STOPB && activeY) buttonRightStopPress(&y);
  else if (keyCode == B_MODE_TAPER)        setModeFromUi(MODE_TAPER, eventFromNextion);
  else if (keyCode == B_MODE_Y && activeY) setModeFromUi(MODE_Y, eventFromNextion);
  else if (keyCode == B_MODE_ELLIPSE)  setModeFromUi(MODE_ELLIPSE, eventFromNextion);
  else if (keyCode == B_MODE_GCODE)    setModeFromUi(MODE_GCODE,   eventFromNextion);
  else if (keyCode == B_MODE_ASYNC)    setModeFromUi(MODE_ASYNC,   eventFromNextion);
  else if (keyCode == B_MULTISTART)    buttonMultistartPress();
  else if (keyCode == B_DISPL)         buttonDisplayPress();
  else if (keyCode == B_X) {
    if (xDroActive && xDroMode) xScale.clearCount();
    else                        markAxis0(&x);
  }
  else if (keyCode == B_Z) {
    if (zDroActive && zDroMode) zScale.clearCount();
    else                        markAxis0(&z);
  }
  else if (keyCode == B_Y && activeY)  markAxis0(&y);
  else if (keyCode == B_Z_DRO && zDroActive) zDroMode = !zDroMode;
  else if (keyCode == B_X_DRO && xDroActive) xDroMode = !xDroMode;
  else if (keyCode == B_X_ENA)         { x.disabled = !x.disabled; updateEnable(&x); }
  else if (keyCode == B_Z_ENA)         { z.disabled = !z.disabled; updateEnable(&z); }
  else if (keyCode == B_Y_ENA && activeY) { y.disabled = !y.disabled; updateEnable(&y); }
  else if (keyCode == B_STEP)          buttonMoveStepPress();
  else if (keyCode == B_REVERSE)       buttonReversePress();
  else if (keyCode == B_MEASURE)       buttonMeasurePress();
  else if (keyCode == B_MODE_GEARS)    setModeFromUi(MODE_NORMAL, eventFromNextion);
  else if (keyCode == B_MODE_TURN)     setModeFromUi(MODE_TURN,   eventFromNextion);
  else if (keyCode == B_MODE_FACE)     setModeFromUi(MODE_FACE,   eventFromNextion);
  else if (keyCode == B_MODE_CONE)     setModeFromUi(MODE_CONE,   eventFromNextion);
  else if (keyCode == B_MODE_CUT)      setModeFromUi(MODE_CUT,    eventFromNextion);
  else if (keyCode == B_MODE_THREAD)   setModeFromUi(MODE_THREAD, eventFromNextion);
  else if (keyCode == B_MODE) {
    if (eventFromNextion) toScreen("page 1");
    else if (mode == MODE_NORMAL)  setModeFromTask(MODE_TURN);
    else if (mode == MODE_TURN)    setModeFromTask(MODE_FACE);
    else if (mode == MODE_FACE)    setModeFromTask(MODE_CONE);
    else if (mode == MODE_CONE)    setModeFromTask(MODE_TAPER);
    else if (mode == MODE_TAPER)   setModeFromTask(MODE_CUT);
    else if (mode == MODE_CUT)     setModeFromTask(MODE_THREAD);
    else if (mode == MODE_THREAD)  setModeFromTask(MODE_ELLIPSE);
    else if (mode == MODE_ELLIPSE) setModeFromTask(MODE_GCODE);
    else if (mode == MODE_GCODE)   setModeFromTask(MODE_ASYNC);
    else if (mode == MODE_ASYNC)   setModeFromTask(activeY ? MODE_Y : MODE_NORMAL);
    else if (mode == MODE_Y)       setModeFromTask(MODE_NORMAL);
  }
}

// ============================================================
// KEYPAD TASK
// ============================================================

void taskKeypad(void* param) {
  while (emergencyStop == ESTOP_NONE) {
    processKeypadEvent();
    taskYIELD();
  }
  vTaskDelete(NULL);
}

#include "display.h"
#include "storage.h"
#include "stepper.h"
#include "dro.h"

// ============================================================
// NEXTION COMMUNICATION
// ============================================================

void toScreen(const String& command) {
  Serial1.print(command);
  Serial1.write(0xFF);
  Serial1.write(0xFF);
  Serial1.write(0xFF);
}

void setText(const String& id, const String& text) {
  toScreen(id + ".txt=\"" + text + "\"");
}

// ============================================================
// NUMBER FORMATTERS
// ============================================================

String printDeciMicrons(long deciMicrons, int precisionPointsMax) {
  if (deciMicrons == 0) return "0";
  bool imperial = measure != MEASURE_METRIC;
  long v = imperial ? round(deciMicrons / 25.4) : deciMicrons;
  int points = 0;
  if (v == 0 && precisionPointsMax >= 5)              points = 5;
  else if ((v % 10) != 0 && precisionPointsMax >= 4)  points = 4;
  else if ((v % 100) != 0 && precisionPointsMax >= 3) points = 3;
  else if ((v % 1000) != 0 && precisionPointsMax >= 2) points = 2;
  else if ((v % 10000) != 0 && precisionPointsMax >= 1) points = 1;
  return String(deciMicrons / (imperial ? 254000.0 : 10000.0), points);
}

String printDegrees(long degrees10000) {
  int points = 0;
  if ((degrees10000 % 100) != 0)   points = 3;
  else if ((degrees10000 % 1000) != 0)  points = 2;
  else if ((degrees10000 % 10000) != 0) points = 1;
  return String(degrees10000 / 10000.0, points) + char(223); // degree symbol
}

String printDupr(long value) {
  if (measure != MEASURE_TPI) return printDeciMicrons(value, 5);
  float tpi = 254000.0 / value;
  String result = "";
  if (abs(tpi - round(tpi)) < TPI_ROUND_EPSILON) {
    result = String(int(round(tpi)));
  } else {
    int tpi100 = round(tpi * 100);
    int points = ((tpi100 % 10) != 0) ? 2 : ((tpi100 % 100) != 0) ? 1 : 0;
    result = String(tpi, points);
  }
  return result;
}

String printNoTrailing0(float value) {
  long v = round(value * 100000);
  int points = 0;
  if ((v % 10) != 0)      points = 5;
  else if ((v % 100) != 0)     points = 4;
  else if ((v % 1000) != 0)    points = 3;
  else if ((v % 10000) != 0)   points = 2;
  else if ((v % 100000) != 0)  points = 1;
  return String(value, points);
}

// ============================================================
// AXIS FORMATTERS
// ============================================================

long stepsToDu(Axis* a, long steps) {
  return round(steps * a->screwPitch / a->motorSteps);
}

long getAxisPosDu(Axis* a) {
  return stepsToDu(a, a->pos + a->originPos);
}

long getAxisStopDiffDu(Axis* a) {
  if (a->leftStop == LONG_MAX || a->rightStop == LONG_MIN) return 0;
  return stepsToDu(a, a->leftStop - a->rightStop);
}

long getAxisLeftStopDistanceDu(Axis* a) {
  return stepsToDu(a, a->leftStop - a->pos);
}

long getAxisRightStopDistanceDu(Axis* a) {
  return stepsToDu(a, a->pos - a->rightStop);
}

String printAxisPos(Axis* a) {
  if (a->rotational) return printDegrees(getAxisPosDu(a));
  return printDeciMicrons(getAxisPosDu(a), 3);
}

String printDistanceToLeftStop(Axis* a) {
  if (a->leftStop == LONG_MAX) return "";
  if (a->rotational) return printDegrees(getAxisLeftStopDistanceDu(a));
  return printDeciMicrons(getAxisLeftStopDistanceDu(a), 3);
}

String printDistanceToRightStop(Axis* a) {
  if (a->rightStop == LONG_MIN) return "";
  if (a->rotational) return printDegrees(getAxisRightStopDistanceDu(a));
  return printDeciMicrons(getAxisRightStopDistanceDu(a), 3);
}

String printAxisStopDiff(Axis* a, bool addTrailingSpace) {
  String result = a->rotational
    ? printDegrees(getAxisStopDiffDu(a))
    : printDeciMicrons(getAxisStopDiffDu(a), 3);
  return addTrailingSpace ? result + ' ' : result;
}

// ============================================================
// MODE / STATE QUERIES
// ============================================================

bool needZStops() {
  return mode == MODE_TURN || mode == MODE_FACE || mode == MODE_THREAD || mode == MODE_ELLIPSE;
}

bool isPassMode() {
  return mode == MODE_TURN || mode == MODE_FACE || mode == MODE_CUT ||
         mode == MODE_THREAD || mode == MODE_ELLIPSE;
}

bool manualMovesAllowedWhenOn() {
  return mode == MODE_NORMAL || mode == MODE_ASYNC || mode == MODE_CONE || mode == MODE_Y;
}

bool manualMovesIgnoredWhenOn() {
  return mode == MODE_GCODE || isPassMode();
}

int getLastSetupIndex() {
  if (mode == MODE_CONE || mode == MODE_GCODE) return 2;
  if (mode == MODE_THREAD) return 4;
  if (mode == MODE_TURN || mode == MODE_FACE || mode == MODE_CUT || mode == MODE_ELLIPSE) return 3;
  return 0;
}

Axis* getPitchAxis() {
  return mode == MODE_FACE ? &x : &z;
}

long getPassModeZStart() {
  if (mode == MODE_TURN || mode == MODE_THREAD) return dupr > 0 ? z.rightStop : z.leftStop;
  if (mode == MODE_FACE) return auxForward ? z.rightStop : z.leftStop;
  if (mode == MODE_ELLIPSE) return dupr > 0 ? z.leftStop : z.rightStop;
  return z.pos;
}

long getPassModeXStart() {
  if (mode == MODE_TURN || mode == MODE_THREAD) return auxForward ? x.rightStop : x.leftStop;
  if (mode == MODE_FACE || mode == MODE_CUT) return dupr > 0 ? x.rightStop : x.leftStop;
  if (mode == MODE_ELLIPSE) return x.rightStop;
  return x.pos;
}

long getNumpadResult() {
  long result = 0;
  for (int i = 0; i < numpadIndex; i++) {
    result += numpadDigits[i] * (long)pow(10, numpadIndex - 1 - i);
  }
  return result;
}

float numpadToConeRatio() {
  return getNumpadResult() / 100000.0;
}

long numpadToDeciMicrons() {
  long result = getNumpadResult();
  if (result == 0) return 0;
  if (measure == MEASURE_INCH)     result = result * 254;
  else if (measure == MEASURE_TPI) result = round(254000.0 / result);
  else                             result = result * 10; // metric
  return result;
}

long spindleModulo(long value) {
  value = value % encoderStepsInt;
  if (value < 0) value += encoderStepsInt;
  return value;
}

String printMode() {
  if (mode == MODE_NORMAL)  return "GEAR";
  if (mode == MODE_ASYNC)   return "ASYNC";
  if (mode == MODE_CONE)    return "CONE";
  if (mode == MODE_TURN)    return "TURN";
  if (mode == MODE_FACE)    return "FACE";
  if (mode == MODE_CUT)     return "CUT";
  if (mode == MODE_THREAD)  return "THREAD";
  if (mode == MODE_ELLIPSE) return "ELLIP";
  if (mode == MODE_GCODE)   return "GCODE";
  if (mode == MODE_Y)       return "Y";
  return "";
}

// ============================================================
// RPM / STEPPER STATUS
// ============================================================

int getApproxRpm() {
  unsigned long t = micros();
  unsigned long elapsedTime = t - spindleEncTime;
  if (elapsedTime > 50000) {
    spindleEncTimeDiffBulk = 0;
    return 0;
  }
  int rpm = 0;
  if (spindleEncTimeDiffBulk > 0) {
    rpm = 60000000 / spindleEncTimeDiffBulk;
    if (abs(rpm - shownRpm) > (rpm < 1000 ? 3 : 5)) {
      shownRpm = rpm;
      shownRpmTime = t;
    }
  }
  return rpm;
}

bool stepperIsRunning(Axis* a) {
  return micros() - a->stepStartUs < 50000;
}

// ============================================================
// DISPLAY UPDATE
// ============================================================

void updateDisplay() {
  if (millis() - lastDisplayUpdateTime < 100) return;
  lastDisplayUpdateTime = millis();

  long newHashLine0 = isOn + spindlePosSync + mode + measure + dupr + starts;
  if (lcdHashLine0 != newHashLine0) {
    lcdHashLine0 = newHashLine0;
    if (spindlePosSync) setText("bStatus", "SYN");
    else setText("bStatus", isOn ? "ON" : "OFF");
    setText("bMode", printMode());
    String bPitchText = printDupr(dupr);
    if (starts != 1) bPitchText += " x" + String(starts);
    setText("tPitch", bPitchText);
    setText("bMeasure", measure == MEASURE_INCH ? "IN" : measure == MEASURE_METRIC ? "MM" : "TPI");
  }

  int rpm = getApproxRpm();
  long newHashLine1 = moveStep + rpm + spindlePos + measure;
  if (lcdHashLine1 != newHashLine1) {
    lcdHashLine1 = newHashLine1;
    setText("tStepVal", printDeciMicrons(moveStep, 5));
    setText("tRPMVal", String(rpm));
    float turns = (float)abs(spindlePos) / encoderStepsInt;
    setText("tTurnsVal", String(turns, turns < 100 ? 2 : (turns < 1000 ? 1 : 0)));
    setText("tAngleVal", String(spindleModulo(spindlePos) * 360 / encoderStepsFloat, 2) + String(char(223)));
  }

  long newHashLine2 =
    x.pos + x.originPos + x.disabled + x.leftStop - x.rightStop +
    z.pos + z.originPos + z.disabled + z.leftStop - z.rightStop +
    y.pos + y.originPos + y.disabled + y.leftStop - y.rightStop +
    measure + x.pos % 100 +
    (zDroActive && zDroMode ? (long)(zScale.getPosition() * 100) : 0) +
    (xDroActive && xDroMode ? (long)(xScale.getPosition() * 100) : 0) +
    zDroMode * 10000L + xDroMode * 20000L;
  if (lcdHashLine2 != newHashLine2) {
    lcdHashLine2 = newHashLine2;

    // X axis — green text when showing scale position, white for stepper
    if (!x.active || x.disabled) {
      setText("tX", "");
      toScreen("tX.pco=65535");
    } else if (xDroActive && xDroMode) {
      setText("tX", String(xScale.getPosition(), 3));
      toScreen("tX.pco=2016");   // green
    } else {
      setText("tX", printAxisPos(&x));
      toScreen("tX.pco=65535");  // white
    }
    setText("tXUp",   !x.active || x.disabled ? "" : printDistanceToLeftStop(&x));
    setText("tXDown", !x.active || x.disabled ? "" : printDistanceToRightStop(&x));

    setText("tY",     !y.active || y.disabled ? "" : printAxisPos(&y));
    setText("tYUp",   !y.active || y.disabled ? "" : printDistanceToLeftStop(&y));
    setText("tYDown", !y.active || y.disabled ? "" : printDistanceToRightStop(&y));

    // Z axis — green text when showing scale position, white for stepper
    if (!z.active || z.disabled) {
      setText("tZ", "");
      toScreen("tZ.pco=65535");
    } else if (zDroActive && zDroMode) {
      setText("tZ", String(zScale.getPosition(), 3));
      toScreen("tZ.pco=2016");   // green
    } else {
      setText("tZ", printAxisPos(&z));
      toScreen("tZ.pco=65535");  // white
    }
    setText("tZLeft",  !z.active || z.disabled ? "" : printDistanceToLeftStop(&z));
    setText("tZRight", !z.active || z.disabled ? "" : printDistanceToRightStop(&z));
  }

  long numpadResult = getNumpadResult();
  long gcodeCommandHash = 0;
  for (int i = 0; i < (int)gcodeCommand.length(); i++) gcodeCommandHash += gcodeCommand.charAt(i);
  for (int i = 0; i < (int)wifiStatus.length(); i++)   gcodeCommandHash += wifiStatus.charAt(i);
  bool spindleStopped = micros() > spindleEncTime + 100000;
  long newHashLine3 = z.pos + (showAngle ? spindlePos : -1) + (showTacho ? rpm : -2) +
    measure + (numpadResult > 0 ? numpadResult : -1) + mode * 5 + dupr +
    (mode == MODE_CONE ? round(coneRatio * 10000) : 0) + turnPasses +
    opIndex + setupIndex + gcodeProgramIndex + gcodeProgramCount +
    spindleStopped * 3 + (isOn ? 139 : -117) + (inNumpad ? 10 : 0) +
    (auxForward ? 17 : -31) +
    (z.leftStop == LONG_MAX ? 123 : z.leftStop) + (z.rightStop == LONG_MIN ? 1234 : z.rightStop) +
    (x.leftStop == LONG_MAX ? 1235 : x.leftStop) + (x.rightStop == LONG_MIN ? 123456 : x.rightStop) +
    gcodeCommandHash +
    (mode == MODE_Y ? y.pos + y.originPos +
      (y.leftStop == LONG_MAX ? 123 : y.leftStop) +
      (y.rightStop == LONG_MIN ? 1234 : y.rightStop) + y.disabled : 0) +
    x.pos + x.originPos + z.pos;

  if (lcdHashLine3 != newHashLine3) {
    lcdHashLine3 = newHashLine3;
    String result = "";

    if (mode == MODE_GCODE) {
      if (setupIndex == 1 && gcodeProgramCount == 0) {
        result = "No stored programs";
      } else if (setupIndex == 1) {
        if (gcodeProgramIndex >= gcodeProgramCount) result = "Program deleted";
        else result = getCurrentGcodeProgramName();
      } else if (setupIndex == 2) {
        result = spindleStopped ? "Turn on the spindle!" : "Spindle on. Go?";
      } else if (isOn) {
        result = gcodeCommand.substring(0, 20);
      }
    } else if (isPassMode()) {
      bool missingZStops = needZStops() && (z.leftStop == LONG_MAX || z.rightStop == LONG_MIN);
      bool missingStops  = missingZStops || x.leftStop == LONG_MAX || x.rightStop == LONG_MIN;
      if (!inNumpad && missingStops) {
        result = needZStops() ? "Set all stops" : "Set X stops";
      } else if (numpadResult != 0 && setupIndex == 1) {
        long passes = min(PASSES_MAX, numpadResult);
        result = String(passes) + (passes == 1 ? " pass?" : " passes?");
      } else if (!isOn && setupIndex == 1) {
        result = String(turnPasses) + (turnPasses == 1 ? " pass?" : " passes?");
      } else if (!isOn && setupIndex == 2) {
        if (mode == MODE_FACE) result = auxForward ? "Right to left?" : "Left to right?";
        else if (mode == MODE_CUT) result = dupr >= 0 ? "Pitch > 0, external" : "Pitch < 0, internal";
        else result = auxForward ? "External?" : "Internal?";
      } else if (mode == MODE_THREAD && !isOn && setupIndex == 3) {
        result = "Cone ratio " + String(numpadToConeRatio(), 5) + "?";
      } else if (!isOn && setupIndex == getLastSetupIndex()) {
        long zOffset = getPassModeZStart() - z.pos;
        long xOffset = getPassModeXStart() - x.pos;
        result = "Go";
        if (zOffset != 0) { result += " "; result += z.name; result += printDeciMicrons(stepsToDu(&z, zOffset), 2); }
        if (xOffset != 0) { result += " "; result += x.name; result += printDeciMicrons(stepsToDu(&x, xOffset), 2); }
        result += "?";
      } else if (isOn && numpadResult == 0) {
        result = "Pass " + String(opIndex) + " of " + String(max(opIndex, long(turnPasses * starts)));
      }
    } else if (mode == MODE_CONE) {
      if (numpadResult != 0 && setupIndex == 1) result = "Use ratio " + String(numpadToConeRatio(), 5) + "?";
      else if (!isOn && setupIndex == 1) result = "Use ratio " + printNoTrailing0(coneRatio) + "?";
      else if (!isOn && setupIndex == 2) result = auxForward ? "External?" : "Internal?";
      else if (!isOn && setupIndex == 3) result = "Go?";
      else if (isOn && numpadResult == 0) result = "Cone ratio " + printNoTrailing0(coneRatio);
    }

    if (inNumpad && result == "") result = "Use " + printDupr(numpadToDeciMicrons()) + "?";
    if (result == "" && (millis() - wifiStatusMillis < 7000 || !x.active || x.disabled)) result = wifiStatus;
    if (result == "" && x.active && !x.disabled) result = "Diameter " + printDeciMicrons(abs(2 * getAxisPosDu(&x)), 2);

    setText("t3", result);
  }
}

// ============================================================
// BUZZER
// ============================================================

void beep() {
  // TODO: implement buzzer output
}

// ============================================================
// DISPLAY TASK
// ============================================================

void taskDisplay(void* param) {
  while (emergencyStop == ESTOP_NONE) {
    updateDisplay();
    // Calling Preferences.commit() blocks all interrupts for ~30ms.
    // Only save when the encoder/steppers are idle.
    unsigned long now = micros();
    if (!stepperIsRunning(&z) && !stepperIsRunning(&x) &&
        (now > spindleEncTime + SAVE_DELAY_US) &&
        (now < saveTime || now > saveTime + SAVE_DELAY_US) &&
        (now < keypadTimeUs || now > keypadTimeUs + SAVE_DELAY_US)) {
      if (saveIfChanged()) saveTime = now;
    }
    if (beepFlag) {
      beepFlag = false;
      beep();
    }
    if (abs(z.pendingPos) > z.estopSteps || abs(x.pendingPos) > x.estopSteps) {
      setEmergencyStop(ESTOP_POS);
    }
    taskYIELD();
  }
  setText("bMode", "ESTOP");
  if (emergencyStop == ESTOP_POS)             setText("t3", "Requested position outside machine");
  else if (emergencyStop == ESTOP_MARK_ORIGIN) setText("t3", "Unable to mark origin");
  else if (emergencyStop == ESTOP_ON_OFF)      setText("t3", "Unable to turn on/off");
  else if (emergencyStop == ESTOP_OFF_MANUAL_MOVE) setText("t3", "Off during manual move");
  vTaskDelete(NULL);
}

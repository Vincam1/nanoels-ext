#include "gcode.h"
#include "display.h"
#include "stepper.h"
#include "modes.h"

// ============================================================
// GCODE FILE MANAGEMENT (LittleFS)
// ============================================================

int getGcodeProgramCount() {
  File root = LittleFS.open("/");
  if (!root || !root.isDirectory()) {
    writeBuffer(&outBuffer, "error: failed to open directory\n");
    return 0;
  }
  int count = 0;
  File file = root.openNextFile();
  while (file) {
    if (String(file.name()).endsWith(".gcode")) count++;
    file.close();
    file = root.openNextFile();
  }
  return count;
}

bool saveGcode() {
  if (gcodeSaveName.length() < 2) { writeBuffer(&outBuffer, "error: name must be at least 2 chars\n"); return false; }
  if (gcodeSaveValue.length() < 2) { writeBuffer(&outBuffer, "error: program too short\n"); return false; }

  String filename = "/" + gcodeSaveName + ".gcode";
  File file = LittleFS.open(filename, "w");
  if (!file) { writeBuffer(&outBuffer, "error: failed to open file\n"); return false; }

  file.print(gcodeSaveValue);
  file.close();
  writeBuffer(&outBuffer, "success: G-code saved\n");
  gcodeProgramCount = getGcodeProgramCount();
  return true;
}

bool removeGcodeByName(const String& name) {
  if (name.length() == 0) return false;
  String filename = "/" + name + ".gcode";
  if (!LittleFS.exists(filename)) { writeBuffer(&outBuffer, "error: file not found\n"); return false; }
  if (!LittleFS.remove(filename)) { writeBuffer(&outBuffer, "error: failed to delete " + filename + "\n"); return false; }
  writeBuffer(&outBuffer, "success: " + name + " deleted\n");
  gcodeProgramCount = getGcodeProgramCount();
  return true;
}

bool removeAllGcode() {
  File root = LittleFS.open("/");
  if (!root || !root.isDirectory()) { writeBuffer(&outBuffer, "error: failed to open directory\n"); return false; }
  File file = root.openNextFile();
  while (file) {
    String path = file.path();
    file.close();
    if (path.endsWith(".gcode")) {
      if (!LittleFS.remove(path)) writeBuffer(&outBuffer, "error: failed to delete " + path + "\n");
    }
    file = root.openNextFile();
  }
  gcodeProgramCount = getGcodeProgramCount();
  return true;
}

String readGcodeProgram(const String& name) {
  String filename = "/" + name + ".gcode";
  File file = LittleFS.open(filename, "r");
  if (!file) return "";
  String result;
  result.reserve(file.size());
  char buf[64];
  while (file.available()) {
    size_t bytesRead = file.readBytes(buf, sizeof(buf));
    result += String(buf).substring(0, bytesRead);
  }
  file.close();
  return result;
}

String getCurrentGcodeProgramName() {
  File root = LittleFS.open("/");
  if (!root || !root.isDirectory()) { writeBuffer(&outBuffer, "error: failed to open directory\n"); return ""; }
  int count = 0;
  File file = root.openNextFile();
  while (file) {
    String filename = file.name();
    file.close();
    if (filename.endsWith(".gcode")) {
      if (count == gcodeProgramIndex) return filename.substring(0, filename.length() - 6);
      count++;
    }
    file = root.openNextFile();
  }
  return "";
}

// ============================================================
// GCODE COMMAND PARSING
// ============================================================

String getValueString(const String& command, char letter) {
  int index = command.indexOf(letter);
  if (index == -1) return "";
  String valueString;
  for (int i = index + 1; i < (int)command.length(); i++) {
    char c = command.charAt(i);
    if (isDigit(c) || c == '.' || c == '-') valueString += c;
    else break;
  }
  return valueString;
}

float getFloat(const String& command, char letter) { return getValueString(command, letter).toFloat(); }
int   getInt(const String& command, char letter)   { return getValueString(command, letter).toInt(); }

void setFeedRate(const String& command) {
  float feed = getFloat(command, 'F');
  if (feed <= 0) return;
  gcodeFeedDuPerSec = round(feed * (measure == MEASURE_METRIC ? 10000 : 254000) / 60.0);
}

long mmOrInchToAbsolutePos(Axis* a, float mmOrInch) {
  long scaleToDu = measure == MEASURE_METRIC ? 10000 : 254000;
  return a->gcodeRelativePos + round(mmOrInch * scaleToDu / a->screwPitch * a->motorSteps);
}

void updateAxisSpeeds(long diffX, long diffZ, long diffY) {
  if (diffX == 0 && diffZ == 0 && diffY == 0) return;
  long  absX = abs(diffX), absZ = abs(diffZ), absC = abs(diffY);
  float stepsPerSecX = gcodeFeedDuPerSec * x.motorSteps / x.screwPitch;
  float minStepsPerSecX = GCODE_FEED_MIN_DU_SEC * x.motorSteps / x.screwPitch;
  if (stepsPerSecX > x.speedManualMove)  stepsPerSecX = x.speedManualMove;
  else if (stepsPerSecX < minStepsPerSecX) stepsPerSecX = minStepsPerSecX;
  float stepsPerSecZ = gcodeFeedDuPerSec * z.motorSteps / z.screwPitch;
  float minStepsPerSecZ = GCODE_FEED_MIN_DU_SEC * z.motorSteps / z.screwPitch;
  if (stepsPerSecZ > z.speedManualMove)  stepsPerSecZ = z.speedManualMove;
  else if (stepsPerSecZ < minStepsPerSecZ) stepsPerSecZ = minStepsPerSecZ;
  float stepsPerSecY = gcodeFeedDuPerSec * y.motorSteps / y.screwPitch;
  float minStepsPerSecY = GCODE_FEED_MIN_DU_SEC * y.motorSteps / y.screwPitch;
  if (stepsPerSecY > y.speedManualMove)  stepsPerSecY = y.speedManualMove;
  else if (stepsPerSecY < minStepsPerSecY) stepsPerSecY = minStepsPerSecY;

  float secX = absX / stepsPerSecX;
  float secZ = absZ / stepsPerSecZ;
  float secY = absC / stepsPerSecY;
  float sec  = activeY ? max(max(secX, secZ), secY) : max(secX, secZ);

  x.speedMax = sec > 0 ? absX / sec : x.speedManualMove;
  z.speedMax = sec > 0 ? absZ / sec : z.speedManualMove;
  y.speedMax = sec > 0 ? absC / sec : y.speedManualMove;
  if (x.speedMax < minStepsPerSecX) x.speedMax = minStepsPerSecX;
  if (z.speedMax < minStepsPerSecZ) z.speedMax = minStepsPerSecZ;
  if (y.speedMax < minStepsPerSecY) y.speedMax = minStepsPerSecY;
}

// ============================================================
// MOTION WAIT HELPERS
// ============================================================

void gcodeWaitEpsilon(int epsilon) {
  while (abs(x.pendingPos) > epsilon || abs(z.pendingPos) > epsilon || abs(y.pendingPos) > epsilon ||
         (SPINDLE_PAUSES_GCODE && getApproxRpm() < GCODE_MIN_RPM)) {
    taskYIELD();
  }
}

void gcodeWaitNear()  { gcodeWaitEpsilon(GCODE_WAIT_EPSILON_STEPS); }
void gcodeWaitStop()  { gcodeWaitEpsilon(0); }

// ============================================================
// G/M CODE HANDLERS
// ============================================================

void G00_01(const String& command) {
  long xStart = x.pos, zStart = z.pos, yStart = y.pos;
  long xEnd = command.indexOf(x.name) >= 0 ? mmOrInchToAbsolutePos(&x, getFloat(command, x.name)) : xStart;
  long zEnd = command.indexOf(z.name) >= 0 ? mmOrInchToAbsolutePos(&z, getFloat(command, z.name)) : zStart;
  long yEnd = command.indexOf(y.name) >= 0 ? mmOrInchToAbsolutePos(&y, getFloat(command, y.name)) : yStart;
  long xDiff = xEnd - xStart, zDiff = zEnd - zStart, yDiff = yEnd - yStart;
  updateAxisSpeeds(xDiff, zDiff, yDiff);
  long chunks = round(max(max(abs(xDiff), abs(zDiff)), abs(yDiff)) * LINEAR_INTERPOLATION_PRECISION);
  for (long i = 0; i < chunks; i++) {
    if (!isOn) return;
    float scale = i / float(chunks);
    stepToContinuous(&x, xStart + xDiff * scale);
    stepToContinuous(&z, zStart + zDiff * scale);
    if (activeY) stepToContinuous(&y, yStart + yDiff * scale);
    gcodeWaitNear();
  }
  stepToFinal(&x, xEnd);
  stepToFinal(&z, zEnd);
  if (activeY) stepToFinal(&y, yEnd);
  gcodeWaitStop();
}

bool handleGcode(const String& command) {
  int op = getInt(command, 'G');
  if (op == 0 || op == 1) {
    G00_01(command);
  } else if (op == 20 || op == 21) {
    setMeasure(op == 20 ? MEASURE_INCH : MEASURE_METRIC);
  } else if (op == 90 || op == 91) {
    gcodeAbsolutePositioning = op == 90;
  } else if (op == 94 || op == 18) {
    /* no-op */
  } else {
    writeBuffer(&outBuffer, "error: unsupported command ");
    writeBuffer(&outBuffer, command);
    writeBuffer(&outBuffer, "\n");
    return false;
  }
  return true;
}

bool handleMcode(const String& command) {
  int op = getInt(command, 'M');
  if (op == 0 || op == 1 || op == 2 || op == 30) {
    setIsOnFromTask(false);
  } else {
    setIsOnFromTask(false);
    writeBuffer(&outBuffer, "error: unsupported command ");
    writeBuffer(&outBuffer, command);
    writeBuffer(&outBuffer, "\n");
    return false;
  }
  return true;
}

bool handleGcodeCommand(String command) {
  command.trim();
  if (command.length() == 0) return false;

  char code = command.charAt(0);
  int  spaceIndex = command.indexOf(' ');
  if (code == 'N' && spaceIndex > 0) {
    command = command.substring(spaceIndex + 1);
    code    = command.charAt(0);
  }

  z.gcodeRelativePos = gcodeAbsolutePositioning ? -z.originPos : z.pos;
  x.gcodeRelativePos = gcodeAbsolutePositioning ? -x.originPos : x.pos;
  y.gcodeRelativePos = gcodeAbsolutePositioning ? -y.originPos : y.pos;

  setFeedRate(command);
  switch (code) {
    case 'G':
    case NAME_Z:
    case NAME_X:
    case NAME_Y:  return handleGcode(command);
    case 'F':     return true;
    case 'M':     return handleMcode(command);
    case 'T':     return true;
    default:
      writeBuffer(&outBuffer, "error: unsupported command ");
      writeBuffer(&outBuffer, code);
      writeBuffer(&outBuffer, "\n");
      return false;
  }
  return false;
}

// ============================================================
// GCODE TASK
// ============================================================

void taskGcode(void* param) {
  while (emergencyStop == ESTOP_NONE) {
    if (mode != MODE_GCODE) {
      gcodeInitialized = false;
    } else if (!gcodeInitialized) {
      gcodeInitialized         = true;
      gcodeCommand             = "";
      gcodeAbsolutePositioning = true;
      gcodeFeedDuPerSec        = GCODE_FEED_DEFAULT_DU_SEC;
      gcodeInBrace             = false;
      gcodeInSemicolon         = false;
    }

    char receivedChar = '\0';
    bool isWebSocket  = false;
    if (mode == MODE_GCODE && isOn && gcodeProgramCharIndex < (int)gcodeProgram.length()) {
      receivedChar = gcodeProgram.charAt(gcodeProgramCharIndex++);
    } else if (bufferAvailable(&inBuffer)) {
      isWebSocket  = true;
      receivedChar = shiftBuffer(&inBuffer);
    }

    int charCode = int(receivedChar);
    if (charCode > 0) {
      if (gcodeInBrace) {
        if (receivedChar == ')') gcodeInBrace = false;
      } else if (wsInKeycode) {
        if (charCode < 32) {
          if (wsKeycode == 0) {
            wsKeycode = keycodeCommand.toInt();
            writeBuffer(&outBuffer, String(wsKeycode));
            writeBuffer(&outBuffer, "\n");
          } else {
            writeBuffer(&outBuffer, "slower\n");
          }
          wsInKeycode   = false;
          keycodeCommand = "";
        } else {
          keycodeCommand += receivedChar;
        }
      } else if (receivedChar == '(')  { gcodeInBrace = true; }
      else if (receivedChar == ';')    { gcodeInSemicolon = true; }
      else if (gcodeInSemicolon && charCode >= 32) { /* ignoring comment */ }
      else if (receivedChar == '!')    { setIsOnFromTask(false); }
      else if (receivedChar == '~')    { setIsOnFromTask(true); }
      else if (receivedChar == '%')    { /* no-op */ }
      else if (receivedChar == '?') {
        writeBuffer(&outBuffer, "<");
        writeBuffer(&outBuffer, isOn ? "Run" : "Idle");
        writeBuffer(&outBuffer, "|WPos:");
        float divisor = measure == MEASURE_METRIC ? 10000.0 : 254000.0;
        writeBuffer(&outBuffer, getAxisPosDu(&x) / divisor, 3);
        writeBuffer(&outBuffer, ",0.000,");
        writeBuffer(&outBuffer, getAxisPosDu(&z) / divisor, 3);
        writeBuffer(&outBuffer, "|FS:");
        writeBuffer(&outBuffer, round(gcodeFeedDuPerSec * 60 / 10000.0));
        writeBuffer(&outBuffer, ",");
        writeBuffer(&outBuffer, String(getApproxRpm()));
        writeBuffer(&outBuffer, "|Id:");
        writeBuffer(&outBuffer, "H" + String(HARDWARE_VERSION) + "V" + String(SOFTWARE_VERSION));
        writeBuffer(&outBuffer, ">");
      } else if (gcodeInSave && receivedChar == '"') {
        gcodeInSave = false;
        if (gcodeSaveName.length() == 0) {
          if (removeAllGcode()) writeBuffer(&outBuffer, "ok\n");
        } else if (gcodeSaveValue.length() > 1) {
          if (saveGcode()) writeBuffer(&outBuffer, "ok\n");
        } else if (gcodeSaveName.length() == 1) {
          writeBuffer(&outBuffer, "error: name must be at least 2 chars\n");
        } else {
          removeGcodeByName(gcodeSaveName);
        }
        gcodeSaveName  = "";
        gcodeSaveValue = "";
      } else if (!gcodeInSave && receivedChar == '"') {
        gcodeInSave          = true;
        gcodeInSaveFirstLine = true;
      } else if (gcodeInSaveFirstLine && receivedChar >= 32) {
        gcodeSaveName += receivedChar;
      } else if (gcodeInSaveFirstLine && receivedChar < 32) {
        gcodeInSaveFirstLine = false;
        writeBuffer(&outBuffer, "ok\n");
      } else if (gcodeInSave) {
        gcodeSaveValue += receivedChar;
        if (receivedChar < 32) { gcodeInBrace = false; gcodeInSemicolon = false; writeBuffer(&outBuffer, "ok\n"); }
      } else if (isOn) {
        if (gcodeInBrace && charCode < 32) {
          writeBuffer(&outBuffer, "error: comment not closed\n");
          setIsOnFromTask(false);
        } else if (charCode < 32 && gcodeCommand.length() > 1) {
          if (handleGcodeCommand(gcodeCommand) && isWebSocket) writeBuffer(&outBuffer, "ok\n");
          gcodeCommand    = "";
          gcodeInSemicolon = false;
        } else if (charCode < 32) {
          if (isWebSocket) writeBuffer(&outBuffer, "ok\n");
          gcodeCommand    = "";
          gcodeInSemicolon = false;
        } else if (charCode >= 32 && (charCode == 'G' || charCode == 'M')) {
          handleGcodeCommand(gcodeCommand);
          gcodeCommand = receivedChar;
        } else if (charCode >= 32) {
          gcodeCommand += receivedChar;
        }
      } else if (receivedChar == '=') {
        wsInKeycode   = true;
        keycodeCommand = "";
      }
    }

    if (mode == MODE_GCODE && isOn && gcodeProgramCharIndex > 0 &&
        gcodeProgramCharIndex == (int)gcodeProgram.length()) {
      setIsOnFromTask(false);
    }
    taskYIELD();
  }
  vTaskDelete(NULL);
}

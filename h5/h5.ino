// NanoEls H5 — main sketch
// All implementation is in the companion .h/.cpp modules.
// This file contains only setup() and loop().

#include "config.h"
#include "types.h"
#include "globals.h"
#include "storage.h"
#include "display.h"
#include "stepper.h"
#include "spindle.h"
#include "modes.h"
#include "input.h"
#include "settings.h"
#include "gcode.h"
#include "wifi_server.h"
#include "dro.h"

void setup() {
  // Load hardware configuration from Preferences (falls back to config.h defaults)
  loadHardwareSettings();

  // Configure GPIO pins
  pinMode(ENC_A, INPUT_PULLUP);
  pinMode(ENC_B, INPUT_PULLUP);

  pinMode(Z_DIR, OUTPUT);
  pinMode(Z_STEP, OUTPUT);
  pinMode(Z_ENA, OUTPUT);
  digitalWrite(Z_STEP, HIGH);

  pinMode(X_DIR, OUTPUT);
  pinMode(X_STEP, OUTPUT);
  pinMode(X_ENA, OUTPUT);
  digitalWrite(X_STEP, HIGH);

  if (activeY) {
    pinMode(Y_DIR, OUTPUT);
    pinMode(Y_STEP, OUTPUT);
    pinMode(Y_ENA, OUTPUT);
    digitalWrite(Y_STEP, HIGH);
  }

  // Check and initialise Preferences namespace
  Preferences pref;
  pref.begin(PREF_NAMESPACE);
  if (pref.getInt(PREF_VERSION) != PREFERENCES_VERSION) {
    pref.clear();
    pref.putInt(PREF_VERSION, PREFERENCES_VERSION);
  }

  // Initialise axes with runtime hardware configuration
  initAxis(&z, NAME_Z, true, false,
           motorStepsZ, screwZDu,
           motorStepsZ, 8 * motorStepsZ, 25 * motorStepsZ,
           invertZ, invertZEnable, needsRestZ, maxTravelMmZ, backlashDuZ,
           Z_ENA, Z_DIR, Z_STEP, Z_PULSE_A, Z_PULSE_B, PCNT_UNIT_1);

  initAxis(&x, NAME_X, true, false,
           motorStepsX, screwXDu,
           motorStepsX, 8 * motorStepsX, 25 * motorStepsX,
           invertX, invertXEnable, needsRestX, maxTravelMmX, backlashDuX,
           X_ENA, X_DIR, X_STEP, X_PULSE_A, X_PULSE_B, PCNT_UNIT_2);

  initAxis(&y, NAME_Y, activeY, rotaryY,
           motorStepsY, screwYDu,
           speedStartY, speedManualMoveY, accelerationY,
           invertY, invertYEnable, needsRestY, maxTravelMmY, backlashDuY,
           Y_ENA, Y_DIR, Y_STEP, Y_PULSE_A, Y_PULSE_B, PCNT_UNIT_3);

  // Load runtime state (positions, mode, spindle, display prefs)
  isOn = false;
  savedDupr = dupr = pref.getLong(PREF_DUPR);
  motionMutex = xSemaphoreCreateMutex();
  savedStarts = starts = min(STARTS_MAX, max(static_cast<int32_t>(1), pref.getInt(PREF_STARTS)));

  z.savedPos = z.pos = pref.getLong(PREF_POS_Z);
  z.savedPosGlobal = z.posGlobal = pref.getLong(PREF_POS_GLOBAL_Z);
  z.savedOriginPos = z.originPos = pref.getLong(PREF_ORIGIN_POS_Z);
  z.savedMotorPos = z.motorPos = pref.getLong(PREF_MOTOR_POS_Z);
  z.savedLeftStop = z.leftStop = pref.getLong(PREF_LEFT_STOP_Z, LONG_MAX);
  z.savedRightStop = z.rightStop = pref.getLong(PREF_RIGHT_STOP_Z, LONG_MIN);
  z.savedDisabled = z.disabled = pref.getBool(PREF_DISABLED_Z, false);

  x.savedPos = x.pos = pref.getLong(PREF_POS_X);
  x.savedPosGlobal = x.posGlobal = pref.getLong(PREF_POS_GLOBAL_X);
  x.savedOriginPos = x.originPos = pref.getLong(PREF_ORIGIN_POS_X);
  x.savedMotorPos = x.motorPos = pref.getLong(PREF_MOTOR_POS_X);
  x.savedLeftStop = x.leftStop = pref.getLong(PREF_LEFT_STOP_X, LONG_MAX);
  x.savedRightStop = x.rightStop = pref.getLong(PREF_RIGHT_STOP_X, LONG_MIN);
  x.savedDisabled = x.disabled = pref.getBool(PREF_DISABLED_X, false);

  y.savedPos = y.pos = pref.getLong(PREF_POS_Y);
  y.savedPosGlobal = y.posGlobal = pref.getLong(PREF_POS_GLOBAL_Y);
  y.savedOriginPos = y.originPos = pref.getLong(PREF_ORIGIN_POS_Y);
  y.savedMotorPos = y.motorPos = pref.getLong(PREF_MOTOR_POS_Y);
  y.savedLeftStop = y.leftStop = pref.getLong(PREF_LEFT_STOP_Y, LONG_MAX);
  y.savedRightStop = y.rightStop = pref.getLong(PREF_RIGHT_STOP_Y, LONG_MIN);
  y.savedDisabled = y.disabled = pref.getBool(PREF_DISABLED_Y, false);

  savedSpindlePos = spindlePos = pref.getLong(PREF_SPINDLE_POS);
  savedSpindlePosAvg = spindlePosAvg = pref.getLong(PREF_SPINDLE_POS_AVG);
  savedSpindlePosSync = spindlePosSync = pref.getInt(PREF_OUT_OF_SYNC);
  savedSpindlePosGlobal = spindlePosGlobal = pref.getLong(PREF_SPINDLE_POS_GLOBAL);
  savedShowAngle = showAngle = pref.getBool(PREF_SHOW_ANGLE);
  savedShowTacho = showTacho = pref.getBool(PREF_SHOW_TACHO);
  savedMoveStep = moveStep = pref.getLong(PREF_MOVE_STEP, MOVE_STEP_1);
  setModeFromLoop(savedMode = pref.getInt(PREF_MODE));
  savedMeasure = measure = pref.getInt(PREF_MEASURE);
  savedConeRatio = coneRatio = pref.getFloat(PREF_CONE_RATIO, coneRatio);
  savedTurnPasses = turnPasses = pref.getInt(PREF_TURN_PASSES, turnPasses);
  savedAuxForward = auxForward = pref.getBool(PREF_AUX_FORWARD, true);
  savedTaperPreset = taperPreset = pref.getInt(PREF_TAPER_PRESET, taperPreset);
  savedThreadCutMode = threadCutMode = pref.getInt(PREF_THREAD_CUT_MODE, DEFAULT_THREAD_CUT_MODE);
  savedThreadAngleTenths = threadAngleTenths = pref.getInt(PREF_THREAD_ANGLE, DEFAULT_THREAD_ANGLE_TENTHS);
  savedGrooveToolRadiusDu = grooveToolRadiusDu = pref.getFloat(PREF_GROOVE_TOOL_R, DEFAULT_GROOVE_TOOL_RADIUS_DU);
  savedGrooveStraightAngleTenths = grooveStraightAngleTenths = pref.getInt(PREF_GROOVE_ANGLE, DEFAULT_GROOVE_STRAIGHT_ANGLE_TENTHS);
  pref.end();

  // Enable motor drivers for axes that don't require rest between moves
  if (!z.needsRest && !z.disabled) digitalWrite(z.ena, z.invertEnable ? LOW : HIGH);
  if (!x.needsRest && !x.disabled) digitalWrite(x.ena, x.invertEnable ? LOW : HIGH);
  if (y.active && !y.needsRest && !y.disabled) digitalWrite(y.ena, y.invertEnable ? LOW : HIGH);

  // Filesystem for GCode storage
  if (LittleFS.begin(true)) {
    gcodeProgramCount = getGcodeProgramCount();
  }

  // Debug serial
  Serial.begin(115200);

  // Nextion HMI
  Serial1.begin(115200, SERIAL_8N1, 44, 43);

  // PS/2 keyboard
  keyboard.begin(KEY_DATA, KEY_CLOCK);
  xTaskCreatePinnedToCore(taskKeypad, "taskKeypad", 10000, NULL, 0, NULL, 0);

  // Non-time-sensitive tasks on core 0
  delay(1300); // Nextion needs time to boot or first display update will be ignored.
  xTaskCreatePinnedToCore(taskAttachInterrupts, "taskAttachInterrupts", 10000, NULL, 0, NULL, 0);
  xTaskCreatePinnedToCore(taskDisplay,          "taskDisplay",          10000, NULL, 0, NULL, 0);
  xTaskCreatePinnedToCore(taskMoveZ,            "taskMoveZ",            10000, NULL, 0, NULL, 0);
  xTaskCreatePinnedToCore(taskMoveX,            "taskMoveX",            10000, NULL, 0, NULL, 0);
  if (activeY) xTaskCreatePinnedToCore(taskMoveY, "taskMoveY",          10000, NULL, 0, NULL, 0);
  xTaskCreatePinnedToCore(taskGcode,            "taskGcode",            10000, NULL, 0, NULL, 0);
  if (WIFI_ENABLED) xTaskCreatePinnedToCore(taskWiFi, "taskWiFi",       10000, NULL, 0, NULL, 0);
}

void loop() {
  if (emergencyStop != ESTOP_NONE) {
    return;
  }
  if (xSemaphoreTake(motionMutex, 1) != pdTRUE) {
    return;
  }
  applySettings();
  processSpindleCounter();
  discountFullSpindleTurns();
  // Groove modes do not require spindle synchronisation (dupr is unused),
  // so they are checked before the dupr == 0 gate.
  if (!isOn || spindlePosSync != 0) {
    // None of the modes work.
  } else if (mode == MODE_GROOVE) {
    modeGroove();
  } else if (mode == MODE_GROOVE_STRAIGHT) {
    modeGrooveStraight();
  } else if (dupr == 0) {
    // Pitch-dependent modes require a non-zero pitch.
  } else if (mode == MODE_NORMAL) {
    modeGearbox();
  } else if (mode == MODE_TURN) {
    modeTurn(&z, &x);
  } else if (mode == MODE_FACE) {
    modeTurn(&x, &z);
  } else if (mode == MODE_CUT) {
    modeCut();
  } else if (mode == MODE_CONE) {
    modeCone();
  } else if (mode == MODE_TAPER) {
    modeTaper();
  } else if (mode == MODE_THREAD) {
    modeTurn(&z, &x);
  } else if (mode == MODE_ELLIPSE) {
    modeEllipse(&z, &x);
  }
  moveAxis(&z);
  moveAxis(&x);
  if (activeY) moveAxis(&y);
  xSemaphoreGive(motionMutex);
}

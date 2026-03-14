#include "storage.h"

void loadHardwareSettings() {
  Preferences pref;
  pref.begin(PREF_NAMESPACE);

  // Encoder
  encoderPPR      = pref.getInt(PREF_HW_ENC_PPR,  DEFAULT_ENCODER_PPR);
  encoderBacklash = pref.getInt(PREF_HW_ENC_BL,   DEFAULT_ENCODER_BACKLASH);

  // Recompute derived encoder constants
  encoderStepsInt   = encoderPPR * 2;
  encoderStepsFloat = (float)encoderStepsInt;
  rpmBulk           = encoderStepsInt;

  // Z axis
  screwZDu      = pref.getLong(PREF_HW_SCREW_Z,  DEFAULT_SCREW_Z_DU);
  motorStepsZ   = pref.getLong(PREF_HW_MOTOR_Z,  DEFAULT_MOTOR_STEPS_Z);
  invertZ       = pref.getBool(PREF_HW_INV_Z,    DEFAULT_INVERT_Z);
  invertZEnable = pref.getBool(PREF_HW_INV_ZE,   DEFAULT_INVERT_Z_ENABLE);
  needsRestZ    = pref.getBool(PREF_HW_REST_Z,   DEFAULT_NEEDS_REST_Z);
  maxTravelMmZ  = pref.getLong(PREF_HW_TRAVEL_Z, DEFAULT_MAX_TRAVEL_MM_Z);
  backlashDuZ   = pref.getLong(PREF_HW_BL_Z,     DEFAULT_BACKLASH_DU_Z);

  // X axis
  screwXDu      = pref.getLong(PREF_HW_SCREW_X,  DEFAULT_SCREW_X_DU);
  motorStepsX   = pref.getLong(PREF_HW_MOTOR_X,  DEFAULT_MOTOR_STEPS_X);
  invertX       = pref.getBool(PREF_HW_INV_X,    DEFAULT_INVERT_X);
  invertXEnable = pref.getBool(PREF_HW_INV_XE,   DEFAULT_INVERT_X_ENABLE);
  needsRestX    = pref.getBool(PREF_HW_REST_X,   DEFAULT_NEEDS_REST_X);
  maxTravelMmX  = pref.getLong(PREF_HW_TRAVEL_X, DEFAULT_MAX_TRAVEL_MM_X);
  backlashDuX   = pref.getLong(PREF_HW_BL_X,     DEFAULT_BACKLASH_DU_X);

  // Y axis
  activeY          = pref.getBool(PREF_HW_ACTIVE_Y,  DEFAULT_ACTIVE_Y);
  rotaryY          = pref.getBool(PREF_HW_ROTARY_Y,  DEFAULT_ROTARY_Y);
  motorStepsY      = pref.getLong(PREF_HW_MOTOR_Y,   DEFAULT_MOTOR_STEPS_Y);
  screwYDu         = pref.getLong(PREF_HW_SCREW_Y,   DEFAULT_SCREW_Y_DU);
  speedStartY      = pref.getLong(PREF_HW_SPD_START_Y, DEFAULT_SPEED_START_Y);
  accelerationY    = pref.getLong(PREF_HW_ACCEL_Y,   DEFAULT_ACCELERATION_Y);
  speedManualMoveY = pref.getLong(PREF_HW_SPD_MAN_Y, DEFAULT_SPEED_MANUAL_MOVE_Y);
  invertY          = pref.getBool(PREF_HW_INV_Y,     DEFAULT_INVERT_Y);
  invertYEnable    = pref.getBool(PREF_HW_INV_YE,    DEFAULT_INVERT_Y_ENABLE);
  needsRestY       = pref.getBool(PREF_HW_REST_Y,    DEFAULT_NEEDS_REST_Y);
  maxTravelMmY     = pref.getLong(PREF_HW_TRAVEL_Y,  DEFAULT_MAX_TRAVEL_MM_Y);
  backlashDuY      = pref.getLong(PREF_HW_BL_Y,      DEFAULT_BACKLASH_DU_Y);

  pref.end();
}

void saveHardwareSettings() {
  Preferences pref;
  pref.begin(PREF_NAMESPACE);

  pref.putInt(PREF_HW_ENC_PPR,      encoderPPR);
  pref.putInt(PREF_HW_ENC_BL,       encoderBacklash);

  pref.putLong(PREF_HW_SCREW_Z,     screwZDu);
  pref.putLong(PREF_HW_MOTOR_Z,     motorStepsZ);
  pref.putBool(PREF_HW_INV_Z,       invertZ);
  pref.putBool(PREF_HW_INV_ZE,      invertZEnable);
  pref.putBool(PREF_HW_REST_Z,      needsRestZ);
  pref.putLong(PREF_HW_TRAVEL_Z,    maxTravelMmZ);
  pref.putLong(PREF_HW_BL_Z,        backlashDuZ);

  pref.putLong(PREF_HW_SCREW_X,     screwXDu);
  pref.putLong(PREF_HW_MOTOR_X,     motorStepsX);
  pref.putBool(PREF_HW_INV_X,       invertX);
  pref.putBool(PREF_HW_INV_XE,      invertXEnable);
  pref.putBool(PREF_HW_REST_X,      needsRestX);
  pref.putLong(PREF_HW_TRAVEL_X,    maxTravelMmX);
  pref.putLong(PREF_HW_BL_X,        backlashDuX);

  pref.putBool(PREF_HW_ACTIVE_Y,    activeY);
  pref.putBool(PREF_HW_ROTARY_Y,    rotaryY);
  pref.putLong(PREF_HW_MOTOR_Y,     motorStepsY);
  pref.putLong(PREF_HW_SCREW_Y,     screwYDu);
  pref.putLong(PREF_HW_SPD_START_Y, speedStartY);
  pref.putLong(PREF_HW_ACCEL_Y,     accelerationY);
  pref.putLong(PREF_HW_SPD_MAN_Y,   speedManualMoveY);
  pref.putBool(PREF_HW_INV_Y,       invertY);
  pref.putBool(PREF_HW_INV_YE,      invertYEnable);
  pref.putBool(PREF_HW_REST_Y,      needsRestY);
  pref.putLong(PREF_HW_TRAVEL_Y,    maxTravelMmY);
  pref.putLong(PREF_HW_BL_Y,        backlashDuY);

  pref.end();
}

bool saveIfChanged() {
  // Avoid writing Preferences unless something actually changed.
  // Each write takes ~20ms and blocks all interrupts.
  if (dupr == savedDupr && starts == savedStarts &&
      z.pos == z.savedPos && z.originPos == z.savedOriginPos &&
      z.posGlobal == z.savedPosGlobal && z.motorPos == z.savedMotorPos &&
      z.leftStop == z.savedLeftStop && z.rightStop == z.savedRightStop &&
      z.disabled == z.savedDisabled &&
      spindlePos == savedSpindlePos && spindlePosAvg == savedSpindlePosAvg &&
      spindlePosSync == savedSpindlePosSync &&
      savedSpindlePosGlobal == spindlePosGlobal &&
      showAngle == savedShowAngle && showTacho == savedShowTacho &&
      moveStep == savedMoveStep && mode == savedMode &&
      measure == savedMeasure &&
      x.pos == x.savedPos && x.originPos == x.savedOriginPos &&
      x.posGlobal == x.savedPosGlobal && x.motorPos == x.savedMotorPos &&
      x.leftStop == x.savedLeftStop && x.rightStop == x.savedRightStop &&
      x.disabled == x.savedDisabled &&
      y.pos == y.savedPos && y.originPos == y.savedOriginPos &&
      y.posGlobal == y.savedPosGlobal && y.motorPos == y.savedMotorPos &&
      y.leftStop == y.savedLeftStop && y.rightStop == y.savedRightStop &&
      y.disabled == y.savedDisabled &&
      coneRatio == savedConeRatio && turnPasses == savedTurnPasses &&
      savedAuxForward == auxForward) {
    return false;
  }

  Preferences pref;
  pref.begin(PREF_NAMESPACE);
  if (dupr != savedDupr)                     pref.putLong(PREF_DUPR,             savedDupr = dupr);
  if (starts != savedStarts)                 pref.putInt(PREF_STARTS,            savedStarts = starts);
  if (z.pos != z.savedPos)                   pref.putLong(PREF_POS_Z,            z.savedPos = z.pos);
  if (z.posGlobal != z.savedPosGlobal)       pref.putLong(PREF_POS_GLOBAL_Z,     z.savedPosGlobal = z.posGlobal);
  if (z.originPos != z.savedOriginPos)       pref.putLong(PREF_ORIGIN_POS_Z,     z.savedOriginPos = z.originPos);
  if (z.motorPos != z.savedMotorPos)         pref.putLong(PREF_MOTOR_POS_Z,      z.savedMotorPos = z.motorPos);
  if (z.leftStop != z.savedLeftStop)         pref.putLong(PREF_LEFT_STOP_Z,      z.savedLeftStop = z.leftStop);
  if (z.rightStop != z.savedRightStop)       pref.putLong(PREF_RIGHT_STOP_Z,     z.savedRightStop = z.rightStop);
  if (z.disabled != z.savedDisabled)         pref.putBool(PREF_DISABLED_Z,       z.savedDisabled = z.disabled);
  if (spindlePos != savedSpindlePos)         pref.putLong(PREF_SPINDLE_POS,      savedSpindlePos = spindlePos);
  if (spindlePosAvg != savedSpindlePosAvg)   pref.putLong(PREF_SPINDLE_POS_AVG,  savedSpindlePosAvg = spindlePosAvg);
  if (spindlePosSync != savedSpindlePosSync) pref.putInt(PREF_OUT_OF_SYNC,       savedSpindlePosSync = spindlePosSync);
  if (spindlePosGlobal != savedSpindlePosGlobal) pref.putLong(PREF_SPINDLE_POS_GLOBAL, savedSpindlePosGlobal = spindlePosGlobal);
  if (showAngle != savedShowAngle)           pref.putBool(PREF_SHOW_ANGLE,       savedShowAngle = showAngle);
  if (showTacho != savedShowTacho)           pref.putBool(PREF_SHOW_TACHO,       savedShowTacho = showTacho);
  if (moveStep != savedMoveStep)             pref.putLong(PREF_MOVE_STEP,        savedMoveStep = moveStep);
  if (mode != savedMode)                     pref.putInt(PREF_MODE,              savedMode = mode);
  if (measure != savedMeasure)               pref.putInt(PREF_MEASURE,           savedMeasure = measure);
  if (x.pos != x.savedPos)                   pref.putLong(PREF_POS_X,            x.savedPos = x.pos);
  if (x.posGlobal != x.savedPosGlobal)       pref.putLong(PREF_POS_GLOBAL_X,     x.savedPosGlobal = x.posGlobal);
  if (x.originPos != x.savedOriginPos)       pref.putLong(PREF_ORIGIN_POS_X,     x.savedOriginPos = x.originPos);
  if (x.motorPos != x.savedMotorPos)         pref.putLong(PREF_MOTOR_POS_X,      x.savedMotorPos = x.motorPos);
  if (x.leftStop != x.savedLeftStop)         pref.putLong(PREF_LEFT_STOP_X,      x.savedLeftStop = x.leftStop);
  if (x.rightStop != x.savedRightStop)       pref.putLong(PREF_RIGHT_STOP_X,     x.savedRightStop = x.rightStop);
  if (x.disabled != x.savedDisabled)         pref.putBool(PREF_DISABLED_X,       x.savedDisabled = x.disabled);
  if (y.pos != y.savedPos)                   pref.putLong(PREF_POS_Y,            y.savedPos = y.pos);
  if (y.posGlobal != y.savedPosGlobal)       pref.putLong(PREF_POS_GLOBAL_Y,     y.savedPosGlobal = y.posGlobal);
  if (y.originPos != y.savedOriginPos)       pref.putLong(PREF_ORIGIN_POS_Y,     y.savedOriginPos = y.originPos);
  if (y.motorPos != y.savedMotorPos)         pref.putLong(PREF_MOTOR_POS_Y,      y.savedMotorPos = y.motorPos);
  if (y.leftStop != y.savedLeftStop)         pref.putLong(PREF_LEFT_STOP_Y,      y.savedLeftStop = y.leftStop);
  if (y.rightStop != y.savedRightStop)       pref.putLong(PREF_RIGHT_STOP_Y,     y.savedRightStop = y.rightStop);
  if (y.disabled != y.savedDisabled)         pref.putBool(PREF_DISABLED_Y,       y.savedDisabled = y.disabled);
  if (coneRatio != savedConeRatio)           pref.putFloat(PREF_CONE_RATIO,      savedConeRatio = coneRatio);
  if (turnPasses != savedTurnPasses)         pref.putInt(PREF_TURN_PASSES,       savedTurnPasses = turnPasses);
  if (auxForward != savedAuxForward)         pref.putBool(PREF_AUX_FORWARD,      savedAuxForward = auxForward);
  pref.end();
  return true;
}

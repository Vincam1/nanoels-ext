#include "stepper.h"
#include "display.h"
#include "dro.h"

#define DHIGH(x) digitalWrite(x, HIGH)
#define DLOW(x)  digitalWrite(x, LOW)
#define DELAY(x) vTaskDelay(x / portTICK_PERIOD_MS)

// ============================================================
// AXIS INITIALISATION
// ============================================================

void initAxis(Axis* a, char name, bool active, bool rotational,
              float motorSteps, float screwPitch,
              long speedStart, long speedManualMove, long acceleration,
              bool invertStepper, bool invertEnable, bool needsRest,
              long maxTravelMm, long backlashDu,
              int ena, int dir, int step,
              int pulseA, int pulseB, pcnt_unit_t pulseUnit) {
  a->mutex = xSemaphoreCreateMutex();

  a->name       = name;
  a->active     = active;
  a->rotational = rotational;
  a->motorSteps = motorSteps;
  a->screwPitch = screwPitch;

  a->pos             = 0;
  a->savedPos        = 0;
  a->fractionalPos   = 0.0f;
  a->originPos       = 0;
  a->savedOriginPos  = 0;
  a->posGlobal       = 0;
  a->savedPosGlobal  = 0;
  a->pendingPos      = 0;
  a->motorPos        = 0;
  a->savedMotorPos   = 0;
  a->continuous      = false;

  a->leftStop          = 0;
  a->savedLeftStop     = 0;
  a->nextLeftStopFlag  = false;
  a->rightStop         = 0;
  a->savedRightStop    = 0;
  a->nextRightStopFlag = false;

  a->speed          = speedStart;
  a->speedStart     = speedStart;
  a->speedMax       = LONG_MAX;
  a->speedManualMove = speedManualMove;
  a->acceleration   = acceleration;
  a->decelerateSteps = 0;
  long s = speedManualMove;
  while (s > speedStart) {
    a->decelerateSteps++;
    s -= a->acceleration / float(s);
  }

  a->direction           = true;
  a->directionInitialized = false;
  a->stepStartUs         = 0;
  a->stepperEnableCounter = 0;
  a->disabled            = false;
  a->savedDisabled       = false;

  a->invertStepper   = invertStepper;
  a->invertEnable    = invertEnable;
  a->needsRest       = needsRest;
  a->movingManually  = false;
  a->estopSteps      = maxTravelMm * 10000 / a->screwPitch * a->motorSteps;
  a->backlashSteps   = backlashDu * a->motorSteps / a->screwPitch;
  a->gcodeRelativePos = 0;

  a->ena       = ena;
  a->dir       = dir;
  a->step      = step;
  a->pulseA    = pulseA;
  a->pulseB    = pulseB;
  a->pulseUnit = pulseUnit;
  a->pulseCount = 0;
}

// ============================================================
// STEPPER ENABLE
// ============================================================

void updateEnable(Axis* a) {
  if (!a->disabled && (!a->needsRest || a->stepperEnableCounter > 0)) {
    digitalWrite(a->ena, a->invertEnable ? LOW : HIGH);
    DELAY(STEPPED_ENABLE_DELAY_MS);
  } else {
    digitalWrite(a->ena, a->invertEnable ? HIGH : LOW);
  }
}

void stepperEnable(Axis* a, bool value) {
  if (!a->needsRest || !a->active) return;
  if (value) {
    a->stepperEnableCounter++;
    if (value == 1) updateEnable(a);
  } else if (a->stepperEnableCounter > 0) {
    a->stepperEnableCounter--;
    if (a->stepperEnableCounter == 0) updateEnable(a);
  }
}

// ============================================================
// EMERGENCY STOP
// ============================================================

void setEmergencyStop(int kind) {
  emergencyStop = kind;
  setAsyncTimerEnable(false);
  xSemaphoreTake(z.mutex, 10);
  xSemaphoreTake(x.mutex, 10);
  xSemaphoreTake(y.mutex, 10);
}

// ============================================================
// ASYNC TIMER
// ============================================================

void setAsyncTimerEnable(bool value) {
  if (value) timerStart(async_timer);
  else       timerStop(async_timer);
}

Axis* getAsyncAxis() {
  return mode == MODE_Y ? &y : &z;
}

unsigned int getTimerLimit() {
  if (dupr == 0) return 65535;
  return min(long(65535), long(TIMER_FREQ / (z.motorSteps * abs(dupr) / z.screwPitch)) - 1);
}

void updateAsyncTimerSettings() {
  setDir(getAsyncAxis(), dupr > 0);
  timerAlarm(async_timer, getTimerLimit(), true, 0);
  timerWrite(async_timer, 0);
}

void IRAM_ATTR onAsyncTimer() {
  Axis* a = getAsyncAxis();
  if (!isOn || a->movingManually || (mode != MODE_ASYNC && mode != MODE_Y)) {
    return;
  } else if (dupr > 0 && a->pos < a->leftStop) {
    if (a->pos <= a->motorPos) a->pos++;
    a->motorPos++;
    a->posGlobal++;
  } else if (dupr < 0 && a->pos > a->rightStop) {
    if (a->pos >= a->motorPos + a->backlashSteps) a->pos--;
    a->motorPos--;
    a->posGlobal--;
  } else {
    return;
  }
  DLOW(a->step);
  a->stepStartUs = micros();
  delayMicroseconds(10);
  DHIGH(a->step);
}

// ============================================================
// ON / OFF
// ============================================================

void setIsOnFromTask(bool on) {
  nextIsOn     = on;
  nextIsOnFlag = true;
}

void setIsOnFromLoop(bool on) {
  if (isOn && on) return;
  if (!on) {
    isOn       = false;
    setupIndex = 0;
  }
  stepperEnable(&z, on);
  stepperEnable(&x, on);
  stepperEnable(&y, on);
  markOrigin();
  if (on) {
    isOn               = true;
    opDuprSign         = dupr >= 0 ? 1 : -1;
    opDupr             = dupr;
    opIndex            = 0;
    opIndexAdvanceFlag = false;
    opSubIndex         = 0;
    setupIndex         = 0;
  }
}

// ============================================================
// ORIGIN MANAGEMENT
// ============================================================

void markAxisOrigin(Axis* a) {
  bool hasSemaphore = xSemaphoreTake(a->mutex, 10) == pdTRUE;
  if (!hasSemaphore) beepFlag = true;
  if (a->leftStop  != LONG_MAX) a->leftStop  -= a->pos;
  if (a->rightStop != LONG_MIN) a->rightStop -= a->pos;
  a->motorPos  -= a->pos;
  a->originPos += a->pos;
  a->pos        = 0;
  a->fractionalPos = 0;
  a->pendingPos = 0;
  if (hasSemaphore) xSemaphoreGive(a->mutex);
}

void zeroSpindlePos() {
  spindlePos    = 0;
  spindlePosAvg = 0;
  spindlePosSync = 0;
}

void markOrigin() {
  markAxisOrigin(&z);
  markAxisOrigin(&x);
  markAxisOrigin(&y);
  zeroSpindlePos();
}

void markAxis0(Axis* a) {
  a->originPos = -a->pos;
}

// ============================================================
// SPINDLE <-> STEPPER POSITION
// ============================================================

long posFromSpindle(Axis* a, long s, bool respectStops) {
  long newPos = s * a->motorSteps / a->screwPitch / encoderStepsFloat * dupr * starts;
  if (respectStops) {
    if (newPos < a->rightStop) newPos = a->rightStop;
    else if (newPos > a->leftStop) newPos = a->leftStop;
  }
  return newPos;
}

long spindleFromPos(Axis* a, long p) {
  return p * a->screwPitch * encoderStepsFloat / a->motorSteps / (dupr * starts);
}

// ============================================================
// STEP COMMANDS
// ============================================================

bool stepTo(Axis* a, long newPos, bool continuous) {
  if (xSemaphoreTake(a->mutex, 10) == pdTRUE) {
    a->continuous = continuous;
    if (newPos == a->pos) {
      a->pendingPos = 0;
    } else {
      a->pendingPos = newPos - a->motorPos - (newPos > a->pos ? 0 : a->backlashSteps);
    }
    xSemaphoreGive(a->mutex);
    return true;
  }
  return false;
}

bool stepToContinuous(Axis* a, long newPos) { return stepTo(a, newPos, true); }
bool stepToFinal(Axis* a, long newPos)      { return stepTo(a, newPos, false); }

void setDir(Axis* a, bool dir) {
  if (a->direction != dir || !a->directionInitialized) {
    a->speed             = a->speedStart;
    a->direction         = dir;
    a->directionInitialized = true;
    digitalWrite(a->dir, dir ^ a->invertStepper);
    delayMicroseconds(DIRECTION_SETUP_DELAY_US);
  }
}

// ============================================================
// MANUAL MOVE HELPERS
// ============================================================

void waitForPendingPosNear0(Axis* a) {
  while (abs(a->pendingPos) > a->motorSteps / 3) taskYIELD();
}

void waitForPendingPos0(Axis* a) {
  while (a->pendingPos != 0) taskYIELD();
}

bool isContinuousStep() {
  return moveStep == (measure == MEASURE_METRIC ? MOVE_STEP_1 : MOVE_STEP_IMP_1);
}

long getMoveStepForAxis(Axis* a) {
  return (a->rotational && measure != MEASURE_METRIC) ? (moveStep / 25.4) : moveStep;
}

long getStepMaxSpeed(Axis* a) {
  return isContinuousStep()
    ? a->speedManualMove
    : min(long(a->speedManualMove), abs(getMoveStepForAxis(a)) * 1000 / STEP_TIME_MS);
}

void waitForStep(Axis* a) {
  if (isContinuousStep()) {
    waitForPendingPosNear0(a);
  } else {
    a->continuous = false;
    waitForPendingPos0(a);
    DELAY(DELAY_BETWEEN_STEPS_MS);
  }
}

int getAndResetPulses(Axis* a) {
  int16_t count;
  pcnt_get_counter_value(a->pulseUnit, &count);
  int delta = count - a->pulseCount;
  if (delta == 0) return 0;
  if (isOn && manualMovesIgnoredWhenOn()) {
    pcnt_counter_clear(a->pulseUnit);
    a->pulseCount = 0;
    return 0;
  }
  if (count >= PCNT_CLEAR || count <= -PCNT_CLEAR) {
    pcnt_counter_clear(a->pulseUnit);
    a->pulseCount = 0;
  } else {
    a->pulseCount = count;
  }
  return delta;
}

// ============================================================
// MOVE AXIS (called from loop() every iteration)
// ============================================================

void moveAxis(Axis* a) {
  if (a->pendingPos == 0) {
    if (a->speed > a->speedStart) a->speed--;
    return;
  }

  unsigned long nowUs  = micros();
  float delayUs        = 1000000.0 / a->speed;
  if (nowUs - a->stepStartUs < delayUs - 5) return;

  if (xSemaphoreTake(a->mutex, 1) == pdTRUE) {
    if (a->pendingPos != 0) {
      bool dir  = a->pendingPos > 0;
      setDir(a, dir);
      DLOW(a->step);
      int delta     = dir ? 1 : -1;
      a->pendingPos -= delta;
      if (dir  && a->motorPos >= a->pos)                   a->pos++;
      else if (!dir && a->motorPos <= (a->pos - a->backlashSteps)) a->pos--;
      a->motorPos  += delta;
      a->posGlobal += delta;

      bool accelerate = a->continuous ||
                        a->pendingPos >= a->decelerateSteps ||
                        a->pendingPos <= -a->decelerateSteps;
      a->speed += (accelerate ? 1 : -1) * a->acceleration * delayUs / 1000000.0;
      if (a->speed > a->speedMax)   a->speed = a->speedMax;
      else if (a->speed < a->speedStart) a->speed = a->speedStart;
      a->stepStartUs = nowUs;
      DHIGH(a->step);
    }
    xSemaphoreGive(a->mutex);
  }
}

// ============================================================
// PULSE COUNTER SETUP
// ============================================================

void startPulseCounter(pcnt_unit_t unit, int gpioA, int gpioB) {
  pcnt_config_t pcntConfig;
  pcntConfig.pulse_gpio_num = gpioA;
  pcntConfig.ctrl_gpio_num  = gpioB;
  pcntConfig.channel        = PCNT_CHANNEL_0;
  pcntConfig.unit           = unit;
  pcntConfig.pos_mode       = PCNT_COUNT_INC;
  pcntConfig.neg_mode       = PCNT_COUNT_DEC;
  pcntConfig.lctrl_mode     = PCNT_MODE_REVERSE;
  pcntConfig.hctrl_mode     = PCNT_MODE_KEEP;
  pcntConfig.counter_h_lim  = PCNT_LIM;
  pcntConfig.counter_l_lim  = -PCNT_LIM;
  pcnt_unit_config(&pcntConfig);
  pcnt_set_filter_value(unit, ENCODER_FILTER);
  pcnt_filter_enable(unit);
  pcnt_counter_pause(unit);
  pcnt_counter_clear(unit);
  pcnt_counter_resume(unit);
}

void taskAttachInterrupts(void* param) {
  startPulseCounter(PCNT_UNIT_0, ENC_A,    ENC_B);
  startPulseCounter(PCNT_UNIT_1, Z_PULSE_A, Z_PULSE_B);
  startPulseCounter(PCNT_UNIT_2, X_PULSE_A, X_PULSE_B);
  startPulseCounter(PCNT_UNIT_3, Y_PULSE_A, Y_PULSE_B);
  attachScales();  // PCNT_UNIT_4 (Z scale) and PCNT_UNIT_5 (X scale) — no-op when inactive
  vTaskDelete(NULL);
}

// ============================================================
// AXIS MOVEMENT TASKS
// ============================================================

void taskMoveZ(void* param) {
  while (emergencyStop == ESTOP_NONE) {
    int pulseDelta = getAndResetPulses(&z);
    bool left  = buttonLeftPressed;
    bool right = buttonRightPressed;
    if (!left && !right && pulseDelta == 0) { taskYIELD(); continue; }
    if (spindlePosSync != 0)               { taskYIELD(); continue; }
    if (isOn && !manualMovesAllowedWhenOn()) {
      setIsOnFromTask(false);
      taskYIELD();
      continue;
    }
    int sign    = pulseDelta == 0 ? (left ? 1 : -1) : (pulseDelta > 0 ? 1 : -1);
    bool stepperOn = true;
    stepperEnable(&z, true);
    z.movingManually = true;

    if (isOn && dupr != 0 && mode == MODE_NORMAL) {
      float fraction  = pulseDelta == 0 ? 1.0f : abs(pulseDelta) / PULSE_PER_REVOLUTION;
      float turns     = moveStep * fraction / abs(dupr * starts);
      int fullTurns   = ceil(turns);
      int diff        = fullTurns * encoderStepsFloat * sign * (dupr > 0 ? 1 : -1);
      long prevSpindlePos = spindlePos;
      bool resting    = false;
      do {
        z.speedMax = z.speedManualMove;
        if (xSemaphoreTake(motionMutex, 100) == pdTRUE) {
          if (!resting) { spindlePos += diff; spindlePosAvg += diff; }
          while (diff > 0 ? (spindlePos < prevSpindlePos) : (spindlePos > prevSpindlePos)) {
            spindlePos += diff; spindlePosAvg += diff;
          }
          prevSpindlePos = spindlePos;
          xSemaphoreGive(motionMutex);
        }
        long newPos = posFromSpindle(&z, prevSpindlePos, true);
        if (newPos != z.pos) {
          stepToContinuous(&z, newPos);
          waitForPendingPosNear0(&z);
          getAndResetPulses(&z);
        } else if (z.pos == (left ? z.leftStop : z.rightStop)) {
          resting = true;
          if (stepperOn) { stepperEnable(&z, false); stepperOn = false; }
          DELAY(200);
        }
      } while (left ? buttonLeftPressed : buttonRightPressed);
    } else {
      z.speedMax = getStepMaxSpeed(&z);
      int delta  = 0;
      do {
        float fractionalDelta = (pulseDelta == 0
          ? moveStep * sign / z.screwPitch
          : pulseDelta / PULSE_PER_REVOLUTION) * z.motorSteps + z.fractionalPos;
        delta = round(fractionalDelta);
        z.fractionalPos = fractionalDelta - delta;
        if (delta == 0) delta = sign;
        long posCopy = z.pos + z.pendingPos;
        if (posCopy + delta > z.leftStop)  delta = z.leftStop  - posCopy;
        else if (posCopy + delta < z.rightStop) delta = z.rightStop - posCopy;
        z.speedMax = getStepMaxSpeed(&z);
        // Feature 4: sample scale before step for motion validation
        float zScaleBefore = (zDroActive && zDroMode) ? zScale.getPosition() : 0.0f;
        stepToContinuous(&z, posCopy + delta);
        waitForStep(&z);
        // Feature 4: if scale moved opposite to commanded direction by >5 µm, abort
        if (zDroActive && zDroMode &&
            (zScale.getPosition() - zScaleBefore) * sign < -0.005f) break;
      } while (delta != 0 && (left ? buttonLeftPressed : buttonRightPressed));
      z.continuous = false;
      waitForPendingPos0(&z);
      // Feature 2: reconcile stepper position to scale after move settles
      if (zDroActive && zDroMode) syncAxisToScale(&z, zScale);
      if (isOn && mode == MODE_CONE) {
        if (xSemaphoreTake(motionMutex, 100) != pdTRUE) setEmergencyStop(ESTOP_MARK_ORIGIN);
        else { markOrigin(); xSemaphoreGive(motionMutex); }
      } else if (isOn && mode == MODE_ASYNC) {
        updateAsyncTimerSettings();
      }
    }
    z.movingManually = false;
    if (stepperOn) stepperEnable(&z, false);
    z.speedMax = LONG_MAX;
    taskYIELD();
  }
  vTaskDelete(NULL);
}

void taskMoveX(void* param) {
  while (emergencyStop == ESTOP_NONE) {
    int pulseDelta = getAndResetPulses(&x);
    bool up   = buttonUpPressed   || pulseDelta > 0;
    bool down = buttonDownPressed || pulseDelta < 0;
    if (!up && !down) { taskYIELD(); continue; }
    if (isOn && !manualMovesAllowedWhenOn()) {
      setIsOnFromTask(false);
      taskYIELD();
      continue;
    }
    x.movingManually = true;
    x.speedMax       = getStepMaxSpeed(&x);
    stepperEnable(&x, true);
    int delta = 0;
    int sign  = up ? 1 : -1;
    do {
      float fractionalDelta = (pulseDelta == 0
        ? moveStep * sign / x.screwPitch
        : pulseDelta / PULSE_PER_REVOLUTION) * x.motorSteps + x.fractionalPos;
      delta = round(fractionalDelta);
      x.fractionalPos = fractionalDelta - delta;
      if (delta == 0) delta = sign;
      long posCopy = x.pos + x.pendingPos;
      if (posCopy + delta > x.leftStop)  delta = x.leftStop  - posCopy;
      else if (posCopy + delta < x.rightStop) delta = x.rightStop - posCopy;
      // Feature 4: sample scale before step for motion validation
      float xScaleBefore = (xDroActive && xDroMode) ? xScale.getPosition() : 0.0f;
      stepToContinuous(&x, posCopy + delta);
      waitForStep(&x);
      // Feature 4: if scale moved opposite to commanded direction by >5 µm, abort
      if (xDroActive && xDroMode &&
          (xScale.getPosition() - xScaleBefore) * sign < -0.005f) break;
      pulseDelta = getAndResetPulses(&x);
    } while (delta != 0 && (pulseDelta != 0 || (up ? buttonUpPressed : buttonDownPressed)));
    x.continuous = false;
    waitForPendingPos0(&x);
    // Feature 2: reconcile stepper position to scale after move settles
    if (xDroActive && xDroMode) syncAxisToScale(&x, xScale);
    if (isOn && mode == MODE_CONE) {
      if (xSemaphoreTake(motionMutex, 100) != pdTRUE) setEmergencyStop(ESTOP_MARK_ORIGIN);
      else { markOrigin(); xSemaphoreGive(motionMutex); }
    }
    x.movingManually = false;
    x.speedMax       = LONG_MAX;
    stepperEnable(&x, false);
    taskYIELD();
  }
  vTaskDelete(NULL);
}

void taskMoveY(void* param) {
  while (emergencyStop == ESTOP_NONE) {
    bool plus  = buttonForwardPressed;
    bool minus = buttonBackPressed;
    if (!plus && !minus) { taskYIELD(); continue; }
    y.movingManually = true;
    y.speedMax       = getStepMaxSpeed(&y);
    stepperEnable(&y, true);
    int delta = 0;
    int sign  = plus ? 1 : -1;
    do {
      float fractionalDelta = getMoveStepForAxis(&y) * sign / y.screwPitch * y.motorSteps + y.fractionalPos;
      delta = round(fractionalDelta);
      y.fractionalPos = fractionalDelta - delta;
      if (delta == 0) delta = sign;
      long posCopy = y.pos + y.pendingPos;
      if (posCopy + delta > y.leftStop)  delta = y.leftStop  - posCopy;
      else if (posCopy + delta < y.rightStop) delta = y.rightStop - posCopy;
      stepToContinuous(&y, posCopy + delta);
      waitForStep(&y);
    } while (plus ? buttonForwardPressed : buttonBackPressed);
    y.continuous = false;
    waitForPendingPos0(&y);
    if (isOn && mode == MODE_Y) updateAsyncTimerSettings();
    y.movingManually = false;
    y.speedMax       = LONG_MAX;
    stepperEnable(&y, false);
    taskYIELD();
  }
  vTaskDelete(NULL);
}

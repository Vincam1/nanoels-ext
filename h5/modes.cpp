#include "modes.h"
#include "stepper.h"
#include "display.h"

// ============================================================
// PITCH / STARTS / MODE MANAGEMENT
// ============================================================

void setDupr(long value) {
  nextDupr     = value;
  nextDuprFlag = true;
}

void applyDupr() {
  if (nextDupr == dupr) return;
  dupr = nextDupr;
  markOrigin();
  if (mode == MODE_ASYNC || mode == MODE_Y) updateAsyncTimerSettings();
}

void setStarts(int value) {
  nextStarts     = value;
  nextStartsFlag = true;
}

void applyStarts() {
  if (starts == nextStarts) return;
  starts = nextStarts;
  markOrigin();
}

void setModeFromTask(int value) {
  nextMode     = value;
  nextModeFlag = true;
}

void setModeFromLoop(int value) {
  if (mode == value) return;
  if (isOn) setIsOnFromLoop(false);
  if (mode == MODE_THREAD)               setStarts(1);
  else if (mode == MODE_ASYNC || mode == MODE_Y) setAsyncTimerEnable(false);
  mode       = value;
  setupIndex = 0;
  if (mode == MODE_ASYNC || mode == MODE_Y) {
    if (!timerAttached) {
      timerAttached = true;
      timerAttachInterrupt(async_timer, &onAsyncTimer);
    }
    updateAsyncTimerSettings();
    setAsyncTimerEnable(true);
  }
}

void setTurnPasses(int value) {
  if (isOn) beepFlag = true;
  else      turnPasses = value;
}

void setConeRatio(float value) {
  nextConeRatio     = value;
  nextConeRatioFlag = true;
}

void applyConeRatio() {
  if (nextConeRatio == coneRatio) return;
  coneRatio = nextConeRatio;
  markOrigin();
}

long normalizePitch(long pitch) {
  int scale = 1;
  if (measure == MEASURE_METRIC)     scale = 100;
  else if (measure == MEASURE_INCH)  scale = 254;
  return round(pitch / scale) * scale;
}

void reset() {
  z.leftStop = LONG_MAX; z.nextLeftStopFlag  = false;
  z.rightStop = LONG_MIN; z.nextRightStopFlag = false;
  z.originPos = 0; z.posGlobal = 0; z.motorPos = 0; z.pendingPos = 0; z.disabled = false;
  x.leftStop = LONG_MAX; x.nextLeftStopFlag  = false;
  x.rightStop = LONG_MIN; x.nextRightStopFlag = false;
  x.originPos = 0; x.posGlobal = 0; x.motorPos = 0; x.pendingPos = 0; x.disabled = false;
  y.leftStop = LONG_MAX; y.nextLeftStopFlag  = false;
  y.rightStop = LONG_MIN; y.nextRightStopFlag = false;
  y.originPos = 0; y.posGlobal = 0; y.motorPos = 0; y.pendingPos = 0; y.disabled = false;
  setDupr(0);
  setStarts(1);
  moveStep = MOVE_STEP_1;
  setModeFromTask(MODE_NORMAL);
  measure   = MEASURE_METRIC;
  showTacho = false;
  showAngle = false;
  setConeRatio(1);
  auxForward = true;
}

// ============================================================
// SOFT STOP MANAGEMENT
// ============================================================

void setLeftStop(Axis* a, long value) {
  a->nextLeftStop    = value;
  a->nextLeftStopFlag = true;
}

void setRightStop(Axis* a, long value) {
  a->nextRightStop    = value;
  a->nextRightStopFlag = true;
}

void leaveStop(Axis* a, long oldStop) {
  if (mode == MODE_CONE) {
    markOrigin();
  } else if (mode == MODE_NORMAL && a == getPitchAxis() && a->pos == oldStop) {
    spindlePosSync = spindleModulo(spindlePos - spindleFromPos(a, a->pos));
  }
}

void applyLeftStop(Axis* a) {
  long oldStop = a->leftStop;
  a->leftStop  = a->nextLeftStop;
  leaveStop(a, oldStop);
}

void applyRightStop(Axis* a) {
  long oldStop  = a->rightStop;
  a->rightStop  = a->nextRightStop;
  leaveStop(a, oldStop);
}

void setMeasure(int value) {
  if (measure == value) return;
  measure  = value;
  moveStep = (measure == MEASURE_METRIC) ? MOVE_STEP_1 : MOVE_STEP_IMP_1;
}

void applySettings() {
  if (nextDuprFlag)        { applyDupr();           nextDuprFlag       = false; }
  if (nextStartsFlag)      { applyStarts();          nextStartsFlag     = false; }
  if (z.nextLeftStopFlag)  { applyLeftStop(&z);  z.nextLeftStopFlag  = false; }
  if (z.nextRightStopFlag) { applyRightStop(&z); z.nextRightStopFlag = false; }
  if (x.nextLeftStopFlag)  { applyLeftStop(&x);  x.nextLeftStopFlag  = false; }
  if (x.nextRightStopFlag) { applyRightStop(&x); x.nextRightStopFlag = false; }
  if (y.nextLeftStopFlag)  { applyLeftStop(&y);  y.nextLeftStopFlag  = false; }
  if (y.nextRightStopFlag) { applyRightStop(&y); y.nextRightStopFlag = false; }
  if (nextConeRatioFlag)   { applyConeRatio();        nextConeRatioFlag  = false; }
  if (nextIsOnFlag)        { setIsOnFromLoop(nextIsOn); nextIsOnFlag     = false; }
  if (nextModeFlag)        { setModeFromLoop(nextMode); nextModeFlag     = false; }
}

// ============================================================
// OPERATING MODES
// ============================================================

void modeGearbox() {
  if (z.movingManually) return;
  z.speedMax = LONG_MAX;
  stepToContinuous(&z, posFromSpindle(&z, spindlePosAvg, true));
}

long auxSafeDistance, startOffset;

void modeTurn(Axis* main, Axis* aux) {
  if (main->movingManually || aux->movingManually || turnPasses <= 0 ||
      main->leftStop == LONG_MAX || main->rightStop == LONG_MIN ||
      aux->leftStop == LONG_MAX  || aux->rightStop == LONG_MIN  ||
      dupr == 0 || (dupr * opDuprSign < 0) || starts < 1) {
    setIsOnFromLoop(false);
    return;
  }

  long mainStartStop = opDuprSign > 0 ? main->rightStop : main->leftStop;
  long mainEndStop   = opDuprSign > 0 ? main->leftStop  : main->rightStop;
  long auxStartStop  = auxForward ? aux->rightStop : aux->leftStop;
  long auxEndStop    = auxForward ? aux->leftStop  : aux->rightStop;

  auxSafeDistance = (auxForward ? -1 : 1) * SAFE_DISTANCE_DU * aux->motorSteps / aux->screwPitch;

  if (opIndex == 0) {
    startOffset     = starts == 1 ? 0 : round(encoderStepsFloat / starts);
    main->speedMax  = main->speedManualMove;
    aux->speedMax   = aux->speedManualMove;
    long auxPos     = auxStartStop;
    long mainPos    = mainStartStop + (opDuprSign > 0 ? -1 : 1);
    stepToFinal(main, mainPos);
    stepToFinal(aux, auxPos);
    if (main->pos == mainPos && aux->pos == auxPos) {
      stepToFinal(main, mainStartStop);
      opIndex    = 1;
      opSubIndex = 0;
    }
  } else if (opIndex <= turnPasses * starts) {
    if (opIndexAdvanceFlag && (opIndex + starts) < turnPasses * starts) {
      opIndexAdvanceFlag = false;
      opIndex += starts;
    }
    float fraction = (turnPasses - ceil(opIndex / float(starts))) / turnPasses;
    if (mode == MODE_THREAD) fraction = fraction * fraction;
    long auxPos = auxEndStop - (auxEndStop - auxStartStop) * fraction;

    if (opSubIndex == 0) {
      stepToFinal(aux, auxPos);
      if (aux->pos == auxPos) {
        opSubIndex    = 1;
        spindlePosSync = spindleModulo(spindlePosGlobal - spindleFromPos(main, main->posGlobal) + startOffset * (opIndex - 1));
        return;
      }
    }
    if (opSubIndex == 1) {
      markOrigin();
      main->speedMax = LONG_MAX;
      opSubIndex     = 2;
      return;
    }
    if (opSubIndex == 2) {
      long mainTargetPos = posFromSpindle(main, spindlePosAvg, true);
      long auxTargetPos  = auxPos;
      if (mode == MODE_THREAD && coneRatio != 0) {
        float coneEffectRatio = -coneRatio / 2 / main->motorSteps * aux->motorSteps / aux->screwPitch * main->screwPitch * (auxForward ? 1 : -1);
        auxTargetPos = auxPos + round(mainTargetPos * coneEffectRatio);
      }
      if (auxTargetPos > aux->leftStop)  auxTargetPos = aux->leftStop;
      if (auxTargetPos < aux->rightStop) auxTargetPos = aux->rightStop;
      stepToContinuous(main, mainTargetPos);
      stepToContinuous(aux, auxTargetPos);
      if (main->pos == mainEndStop || (coneRatio != 0 && aux->pos == (opDuprSign > 0 ? auxStartStop : auxEndStop))) {
        opSubIndex = 3;
      }
    }
    if (opSubIndex == 3) {
      long auxTargetPos = (mode == MODE_THREAD ? auxStartStop : auxPos) + auxSafeDistance;
      stepToFinal(aux, auxTargetPos);
      if (aux->pos == auxTargetPos) opSubIndex = 4;
    }
    if (opSubIndex == 4) {
      main->speedMax  = main->speedManualMove;
      long mainPos    = mainStartStop + (opDuprSign > 0 ? -1 : 1);
      stepToFinal(main, mainPos);
      if (main->pos == mainPos) {
        stepToFinal(main, mainStartStop);
        opSubIndex = 0;
        opIndex++;
      }
    }
  } else {
    main->speedMax  = main->speedManualMove;
    long auxPos     = auxStartStop;
    long mainPos    = mainStartStop;
    stepToFinal(main, mainPos);
    stepToFinal(aux, auxPos);
    if (main->pos == mainPos && aux->pos == auxPos) {
      setIsOnFromLoop(false);
      beepFlag = true;
    }
  }
}

void modeCone() {
  if (z.movingManually || x.movingManually || coneRatio == 0) return;

  float zToXRatio = -coneRatio / 2 / z.motorSteps * x.motorSteps / x.screwPitch * z.screwPitch * (auxForward ? 1 : -1);
  if (zToXRatio == 0) return;

  x.speedMax = LONG_MAX;
  z.speedMax = LONG_MAX;

  long spindle    = spindlePosAvg;
  long spindleMin = LONG_MIN;
  long spindleMax = LONG_MAX;

  if (z.leftStop != LONG_MAX) (dupr > 0 ? spindleMax : spindleMin) = spindleFromPos(&z, z.leftStop);
  if (z.rightStop != LONG_MIN) (dupr > 0 ? spindleMin : spindleMax) = spindleFromPos(&z, z.rightStop);
  if (x.leftStop != LONG_MAX) {
    long lim = spindleFromPos(&z, round(x.leftStop / zToXRatio));
    if (zToXRatio < 0) (dupr > 0 ? spindleMin : spindleMax) = lim;
    else               (dupr > 0 ? spindleMax : spindleMin) = lim;
  }
  if (x.rightStop != LONG_MIN) {
    long lim = spindleFromPos(&z, round(x.rightStop / zToXRatio));
    if (zToXRatio < 0) (dupr > 0 ? spindleMax : spindleMin) = lim;
    else               (dupr > 0 ? spindleMin : spindleMax) = lim;
  }
  if (spindle > spindleMax) spindle = spindleMax;
  else if (spindle < spindleMin) spindle = spindleMin;

  stepToContinuous(&z, posFromSpindle(&z, spindle, true));
  stepToContinuous(&x, round(z.pos * zToXRatio));
}

void modeCut() {
  if (x.movingManually || turnPasses <= 0 ||
      x.leftStop == LONG_MAX || x.rightStop == LONG_MIN ||
      dupr == 0 || dupr * opDuprSign < 0) {
    setIsOnFromLoop(false);
    return;
  }

  long startStop = opDuprSign > 0 ? x.rightStop : x.leftStop;
  long endStop   = opDuprSign > 0 ? x.leftStop  : x.rightStop;

  if (opIndex == 0) {
    x.speedMax  = x.speedManualMove;
    long xPos   = startStop;
    stepToFinal(&x, xPos);
    if (x.pos == xPos) { opIndex = 1; opSubIndex = 0; }
  } else if (opIndex <= turnPasses) {
    if (opSubIndex == 0) {
      spindlePosAvg = spindlePos = spindleFromPos(&x, x.pos);
      opSubIndex = 1;
    }
    if (opSubIndex == 1) {
      x.speedMax  = LONG_MAX;
      long endPos = endStop - (endStop - startStop) / turnPasses * (turnPasses - opIndex);
      long xPos   = posFromSpindle(&x, spindlePosAvg, true);
      if (dupr > 0 && xPos > endPos) xPos = endPos;
      else if (dupr < 0 && xPos < endPos) xPos = endPos;
      stepToContinuous(&x, xPos);
      if (x.pos == endPos) opSubIndex = 2;
    }
    if (opSubIndex == 2) {
      x.speedMax = x.speedManualMove;
      stepToFinal(&x, startStop);
      if (x.pos == startStop) { opSubIndex = 0; opIndex++; }
    }
  } else {
    setIsOnFromLoop(false);
    beepFlag = true;
  }
}

void modeEllipse(Axis* main, Axis* aux) {
  if (main->movingManually || aux->movingManually || turnPasses <= 0 ||
      main->leftStop == LONG_MAX || main->rightStop == LONG_MIN ||
      aux->leftStop  == LONG_MAX || aux->rightStop  == LONG_MIN ||
      main->leftStop == main->rightStop ||
      aux->leftStop  == aux->rightStop  ||
      dupr == 0 || dupr != opDupr) {
    setIsOnFromLoop(false);
    return;
  }

  long mainStartStop = opDuprSign > 0 ? main->rightStop : main->leftStop;
  long mainEndStop   = opDuprSign > 0 ? main->leftStop  : main->rightStop;
  long auxStartStop  = aux->rightStop;
  long auxEndStop    = aux->leftStop;

  main->speedMax = main->speedManualMove;
  aux->speedMax  = aux->speedManualMove;

  if (opIndex == 0) {
    opIndex = 1; opSubIndex = 0;
    spindlePos = 0; spindlePosAvg = 0;
  } else if (opIndex <= turnPasses) {
    float pass0to1   = opIndex / float(turnPasses);
    long mainDelta   = round(pass0to1 * (mainEndStop - mainStartStop));
    long auxDelta    = round(pass0to1 * (auxEndStop  - auxStartStop));
    long spindleDelta = spindleFromPos(main, mainDelta);

    if (opSubIndex == 0) {
      long auxPos = auxStartStop;
      stepToFinal(aux, auxPos);
      if (aux->pos == auxPos) opSubIndex = 1;
    } else if (opSubIndex == 1) {
      long mainPos = mainEndStop - mainDelta;
      stepToFinal(main, mainPos);
      if (main->pos == mainPos) { opSubIndex = 2; spindlePos = 0; spindlePosAvg = 0; }
    } else if (opSubIndex == 2) {
      float progress0to1 = 0;
      if ((spindleDelta > 0 && spindlePosAvg >= spindleDelta) ||
          (spindleDelta < 0 && spindlePosAvg <= spindleDelta)) {
        progress0to1 = 1;
      } else {
        progress0to1 = spindlePosAvg / float(spindleDelta);
      }
      float mainCoeff = auxForward
        ? cos(HALF_PI * (3 + progress0to1))
        : (1 + sin(HALF_PI * (progress0to1 - 1)));
      long mainPos = mainEndStop - mainDelta + round(mainDelta * mainCoeff);
      float auxCoeff = auxForward
        ? (1 + sin(HALF_PI * (3 + progress0to1)))
        : sin(HALF_PI * progress0to1);
      long auxPos = auxStartStop + round(auxDelta * auxCoeff);
      stepToContinuous(main, mainPos);
      stepToContinuous(aux, auxPos);
      if (progress0to1 == 1 && main->pos == mainPos && aux->pos == auxPos) {
        opIndex++; opSubIndex = 0;
      }
    }
  } else if (opIndex == turnPasses + 1) {
    stepToFinal(aux, auxStartStop);
    if (aux->pos == auxStartStop) { setIsOnFromLoop(false); beepFlag = true; }
  }
}

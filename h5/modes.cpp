#include "modes.h"
#include "stepper.h"
#include "display.h"
#include <math.h>

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

// ============================================================
// TAPER PRESET TABLE
// Morse: ANSI B5.10 (taper-per-foot / 12 = diameter-change per unit length)
// Jacobs: ANSI B9.3 ((D_large - D_small) / length)
// Verify against your specific tooling before cutting.
// ============================================================
struct TaperPreset { const char* name; float coneRatio; };
static const TaperPreset TAPER_PRESETS[] = {
  {"MT0",  0.05205f},  // Morse taper 0
  {"MT1",  0.04988f},  // Morse taper 1
  {"MT2",  0.04995f},  // Morse taper 2
  {"MT3",  0.05020f},  // Morse taper 3
  {"MT4",  0.05194f},  // Morse taper 4
  {"MT5",  0.05263f},  // Morse taper 5
  {"MT6",  0.05214f},  // Morse taper 6
  {"MT7",  0.05200f},  // Morse taper 7
  {"JT0",  0.05688f},  // Jacobs taper 0
  {"JT1",  0.07010f},  // Jacobs taper 1
  {"JT2",  0.03758f},  // Jacobs taper 2
  {"JT3",  0.03461f},  // Jacobs taper 3
  {"JT4",  0.04485f},  // Jacobs taper 4
  {"JT5",  0.05081f},  // Jacobs taper 5
  {"JT6",  0.06270f},  // Jacobs taper 6
  {"JT33", 0.06350f},  // Jacobs taper 33
};
const int TAPER_PRESET_COUNT = 16;

const char* taperPresetName(int idx) {
  if (idx >= 0 && idx < TAPER_PRESET_COUNT) return TAPER_PRESETS[idx].name;
  return "?";
}

float taperPresetRatio(int idx) {
  if (idx >= 0 && idx < TAPER_PRESET_COUNT) return TAPER_PRESETS[idx].coneRatio;
  return 0.0f;
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
  if (mode == MODE_CONE || mode == MODE_TAPER) {
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
    startOffset        = starts == 1 ? 0 : round(encoderStepsFloat / starts);
    threadSpindleOffset = 0;
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

    // Depth fraction for this pass.
    // Threading flank modes use uniform (linear) depth per pass for consistent chip load.
    // Radial mode uses a quadratic series (heavier early passes, lighter finishing passes).
    float fraction = (turnPasses - ceil(opIndex / float(starts))) / turnPasses;
    if (mode == MODE_THREAD && threadCutMode == THREAD_CUT_RADIAL) fraction = fraction * fraction;
    long auxPos = auxEndStop - (auxEndStop - auxStartStop) * fraction;

    if (opSubIndex == 0) {
      stepToFinal(aux, auxPos);
      if (aux->pos == auxPos) {
        opSubIndex = 1;

        // Flank infeed: compute spindle-step offset so the tool enters the thread
        // groove at a shifted axial position, cutting primarily on the leading flank.
        // The offset is derived from the cumulative X depth via:
        //   ΔZ_du = cumulativeDepth_du × tan(infeedHalfAngle)
        //   ΔSpindle = ΔZ_du × encoderStepsFloat / |dupr|
        if (mode == MODE_THREAD && threadCutMode != THREAD_CUT_RADIAL && dupr != 0) {
          float halfAngleDeg = threadAngleTenths / 20.0f; // half of included angle, in degrees
          if (threadCutMode == THREAD_CUT_MODIFIED || threadCutMode == THREAD_CUT_ALTERNATING) {
            halfAngleDeg -= THREAD_FLANK_DELTA_DEG; // slight trailing-flank engagement
          }
          float halfAngleRad = halfAngleDeg * (float)M_PI / 180.0f;
          // Cumulative aux-axis depth from start position (in deci-microns)
          float depthDu = fabsf((float)(auxPos - auxStartStop)) * aux->screwPitch / aux->motorSteps;
          long  offset  = lroundf(depthDu * tanf(halfAngleRad) * encoderStepsFloat / fabsf((float)dupr));
          if (threadCutMode == THREAD_CUT_ALTERNATING) {
            // Alternate sign each pass: odd passes cut leading flank, even cut trailing flank.
            int passNum = (int)ceilf(opIndex / (float)starts);
            threadSpindleOffset = (passNum % 2 == 1) ? offset : -offset;
          } else {
            threadSpindleOffset = offset; // always shift in same direction for flank/modified
          }
        } else {
          threadSpindleOffset = 0;
        }

        spindlePosSync = spindleModulo(spindlePosGlobal - spindleFromPos(main, main->posGlobal)
                                       + startOffset * (opIndex - 1) + threadSpindleOffset);
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

void modeTaper() {
  modeCone();
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

// ============================================================
// GROOVE — circular / elliptical cross-section groove
//
// The groove cross-section is a semi-ellipse in the Z-X plane.
// Z stops define the groove width (ZP); X stops define the depth (XP).
// grooveToolRadiusDu is the radius of the round insert used.
//
// Tool-centre path semi-axes (after corner-radius compensation):
//   aEff (Z) = ZP/2  − toolRadius
//   bEff (X) = XP    − toolRadius
//
// For a circular groove set ZP = 2 × XP.
// ============================================================

void modeGroove() {
  if (z.movingManually || x.movingManually || turnPasses <= 0 ||
      z.leftStop == LONG_MAX || z.rightStop == LONG_MIN ||
      x.leftStop == LONG_MAX || x.rightStop == LONG_MIN) {
    setIsOnFromLoop(false);
    return;
  }

  // Direction: auxForward=true → external (cut toward x.leftStop from x.rightStop)
  long xSurface    = auxForward ? x.rightStop : x.leftStop;
  long xDeep       = auxForward ? x.leftStop  : x.rightStop;
  int  xDir        = (xDeep > xSurface) ? 1 : -1;
  long zCenter     = (z.leftStop + z.rightStop) / 2;

  // Groove dimensions in stepper steps
  long zHalfSteps  = abs(z.leftStop - z.rightStop) / 2; // ZP/2
  long xTotalSteps = abs(xDeep - xSurface);             // XP

  // Tool corner radius converted to each axis' step space
  long toolRz = lroundf(grooveToolRadiusDu * z.motorSteps / z.screwPitch);
  long toolRx = lroundf(grooveToolRadiusDu * x.motorSteps / x.screwPitch);

  // Effective semi-axes for the tool-centre path
  long aEff = zHalfSteps - toolRz; // Z semi-axis in steps
  long bEff = xTotalSteps - toolRx; // X depth in steps

  if (aEff <= 0 || bEff <= 0) {
    setIsOnFromLoop(false);
    return;
  }

  if (opIndex == 0) {
    // Phase 0: position to the right edge of the groove at the surface
    z.speedMax = z.speedManualMove;
    x.speedMax = x.speedManualMove;
    stepToFinal(&z, zCenter + aEff);
    stepToFinal(&x, xSurface);
    if (z.pos == zCenter + aEff && x.pos == xSurface) {
      opIndex = 1; opSubIndex = 0;
    }
  } else if (opIndex <= turnPasses) {
    // Phase 1..turnPasses: roughing — horizontal passes from right to left at
    // increasing depth, following the ellipse boundary at each depth level.
    long dSteps = bEff * opIndex / turnPasses; // current X depth (steps)

    // Z half-width at this depth: aEff × cos(θ) where sin(θ) = d/bEff
    float sinT   = (bEff > 0) ? (float)dSteps / bEff : 0.0f;
    float cosT   = sqrtf(fmaxf(0.0f, 1.0f - sinT * sinT));
    long  zwSteps = lroundf(aEff * cosT);

    long xTarget = xSurface + xDir * dSteps;

    if (opSubIndex == 0) {
      // Reposition Z to right edge at this depth level
      z.speedMax = z.speedManualMove;
      stepToFinal(&z, zCenter + zwSteps);
      if (z.pos == zCenter + zwSteps) opSubIndex = 1;
    } else if (opSubIndex == 1) {
      // Plunge X to depth
      x.speedMax = LONG_MAX;
      stepToFinal(&x, xTarget);
      if (x.pos == xTarget) opSubIndex = 2;
    } else if (opSubIndex == 2) {
      // Cut from right to left (the main material-removal stroke)
      z.speedMax = LONG_MAX;
      stepToFinal(&z, zCenter - zwSteps);
      if (z.pos == zCenter - zwSteps) opSubIndex = 3;
    } else if (opSubIndex == 3) {
      // Retract X back to surface
      x.speedMax = x.speedManualMove;
      stepToFinal(&x, xSurface);
      if (x.pos == xSurface) { opSubIndex = 0; opIndex++; }
    }
  } else if (opIndex == turnPasses + 1) {
    // Phase: finish pass — continuously trace the full ellipse contour
    // from the right edge to the left edge.  X is driven by Z position.
    z.speedMax = z.speedManualMove;
    x.speedMax = z.speedManualMove; // keep speeds matched for smooth path

    // Compute the required X for the current Z position (ellipse equation)
    float zOff = (float)(z.pos - zCenter);
    float cosT = (aEff > 0) ? (zOff / aEff) : 0.0f;
    cosT = fmaxf(-1.0f, fminf(1.0f, cosT));
    float sinT  = sqrtf(1.0f - cosT * cosT);
    long xTarget = xSurface + xDir * lroundf(bEff * sinT);

    stepToContinuous(&z, zCenter - aEff); // drive Z toward left edge
    stepToContinuous(&x, xTarget);        // X follows the ellipse

    if (z.pos == zCenter - aEff) {
      opIndex = turnPasses + 2; opSubIndex = 0;
    }
  } else {
    // Done: retract X, return Z to starting position
    x.speedMax = x.speedManualMove;
    z.speedMax = z.speedManualMove;
    stepToFinal(&x, xSurface);
    stepToFinal(&z, zCenter + aEff);
    if (x.pos == xSurface && z.pos == zCenter + aEff) {
      setIsOnFromLoop(false);
      beepFlag = true;
    }
  }
}

// ============================================================
// GROOVE STRAIGHT — V-belt / trapezoidal groove
//
// The groove has straight angled flanks at grooveStraightAngleTenths/10 degrees
// from vertical.  Z stops define the top width (ZP); X stops define the depth (XP).
// grooveToolRadiusDu is used as the tool half-width (not a corner radius here).
//
// Bottom half-width (steps): zHalfSteps − XP_du × tan(angle) × z.motorSteps / z.screwPitch
//
// Standard V-belt angles: SPZ/SPA/SPB/SPC → 34° or 38°.  Set via +/- at setup.
// ============================================================

void modeGrooveStraight() {
  if (z.movingManually || x.movingManually || turnPasses <= 0 ||
      z.leftStop == LONG_MAX || z.rightStop == LONG_MIN ||
      x.leftStop == LONG_MAX || x.rightStop == LONG_MIN) {
    setIsOnFromLoop(false);
    return;
  }

  long xSurface    = auxForward ? x.rightStop : x.leftStop;
  long xDeep       = auxForward ? x.leftStop  : x.rightStop;
  int  xDir        = (xDeep > xSurface) ? 1 : -1;
  long zCenter     = (z.leftStop + z.rightStop) / 2;
  long zHalfSteps  = abs(z.leftStop - z.rightStop) / 2;
  long xTotalSteps = abs(xDeep - xSurface);

  // Tool half-width in Z steps (grooveToolRadiusDu used as half tool width)
  long toolHalfZ = lroundf(grooveToolRadiusDu * z.motorSteps / z.screwPitch);

  // Flank angle
  float angleRad  = (grooveStraightAngleTenths / 10.0f) * (float)M_PI / 180.0f;
  float tanAngle  = tanf(angleRad);

  // Full-depth Z shift per unit X depth (in step/step, via du conversion)
  float xDuPerStep = x.screwPitch / x.motorSteps; // du per X step
  float zStepPerDu = z.motorSteps / z.screwPitch;  // Z steps per du
  float zShiftPerXStep = xDuPerStep * tanAngle * zStepPerDu;

  // Bottom half-width (steps) = ZP/2 − XP × tan(angle) [in step space]
  long zShiftFull   = lroundf(xTotalSteps * zShiftPerXStep);
  long bottomHalfZ  = zHalfSteps - zShiftFull;

  if (bottomHalfZ < toolHalfZ) {
    // Bottom is narrower than tool — cannot cut safely
    setIsOnFromLoop(false);
    return;
  }

  if (opIndex == 0) {
    // Position tool to groove centre at surface
    z.speedMax = z.speedManualMove;
    x.speedMax = x.speedManualMove;
    stepToFinal(&z, zCenter);
    stepToFinal(&x, xSurface);
    if (z.pos == zCenter && x.pos == xSurface) {
      opIndex = 1; opSubIndex = 0;
    }
  } else if (opIndex <= turnPasses) {
    // Roughing: plunge + horizontal sweep at each depth level.
    // The sweep clears only the groove-bottom area (not the flanks).
    long dSteps  = xTotalSteps * opIndex / turnPasses;
    long xTarget = xSurface + xDir * dSteps;

    // Z half-width available at this depth for the centre sweep
    long zShiftAtD = lroundf(dSteps * zShiftPerXStep);
    long halfWidthAtD = zHalfSteps - zShiftAtD; // groove inner half-width at this depth
    // Centre-clear region: subtract tool half-width from each side
    long sweepHalf = halfWidthAtD - toolHalfZ;
    if (sweepHalf < 0) sweepHalf = 0;

    if (opSubIndex == 0) {
      // Move to right side of centre clear zone
      z.speedMax = z.speedManualMove;
      stepToFinal(&z, zCenter + sweepHalf);
      if (z.pos == zCenter + sweepHalf) opSubIndex = 1;
    } else if (opSubIndex == 1) {
      // Plunge to depth
      x.speedMax = LONG_MAX;
      stepToFinal(&x, xTarget);
      if (x.pos == xTarget) opSubIndex = 2;
    } else if (opSubIndex == 2) {
      // Sweep left through the centre (cuts groove bottom)
      z.speedMax = LONG_MAX;
      stepToFinal(&z, zCenter - sweepHalf);
      if (z.pos == zCenter - sweepHalf) opSubIndex = 3;
    } else if (opSubIndex == 3) {
      // Retract X
      x.speedMax = x.speedManualMove;
      stepToFinal(&x, xSurface);
      if (x.pos == xSurface) { opSubIndex = 0; opIndex++; }
    }
  } else if (opIndex == turnPasses + 1) {
    // Right flank finish: simultaneous Z+X linear move from top corner to bottom corner.
    // Right flank runs from (zCenter + zHalfSteps, xSurface) to (zCenter + bottomHalfZ, xDeep).
    z.speedMax = x.speedManualMove;
    x.speedMax = x.speedManualMove;

    if (opSubIndex == 0) {
      // Position to top of right flank
      stepToFinal(&z, zCenter + zHalfSteps);
      stepToFinal(&x, xSurface);
      if (z.pos == zCenter + zHalfSteps && x.pos == xSurface) opSubIndex = 1;
    } else {
      // Drive Z inward; X tracks proportionally
      long zFlankHeight = zHalfSteps - bottomHalfZ; // positive, Z steps traversed on flank
      long zTravelled   = (zCenter + zHalfSteps) - z.pos; // how far Z has moved so far
      long xTarget      = (zFlankHeight > 0)
                          ? xSurface + xDir * lroundf((float)zTravelled * xTotalSteps / zFlankHeight)
                          : xDeep;
      if (xDir > 0) xTarget = min(xTarget, xDeep);
      else          xTarget = max(xTarget, xDeep);
      stepToContinuous(&z, zCenter + bottomHalfZ);
      stepToContinuous(&x, xTarget);
      if (z.pos == zCenter + bottomHalfZ && x.pos == xDeep) {
        opIndex = turnPasses + 2; opSubIndex = 0;
      }
    }
  } else if (opIndex == turnPasses + 2) {
    // Left flank finish: mirror of right flank.
    z.speedMax = x.speedManualMove;
    x.speedMax = x.speedManualMove;

    if (opSubIndex == 0) {
      // Retract X, reposition to top of left flank
      stepToFinal(&x, xSurface);
      if (x.pos == xSurface) {
        stepToFinal(&z, zCenter - zHalfSteps);
        if (z.pos == zCenter - zHalfSteps) opSubIndex = 1;
      }
    } else {
      long zFlankHeight = zHalfSteps - bottomHalfZ;
      long zTravelled   = z.pos - (zCenter - zHalfSteps); // Z moved rightward (toward center)
      long xTarget      = (zFlankHeight > 0)
                          ? xSurface + xDir * lroundf((float)zTravelled * xTotalSteps / zFlankHeight)
                          : xDeep;
      if (xDir > 0) xTarget = min(xTarget, xDeep);
      else          xTarget = max(xTarget, xDeep);
      stepToContinuous(&z, zCenter - bottomHalfZ);
      stepToContinuous(&x, xTarget);
      if (z.pos == zCenter - bottomHalfZ && x.pos == xDeep) {
        opIndex = turnPasses + 3; opSubIndex = 0;
      }
    }
  } else {
    // Done: retract X, return Z to centre
    x.speedMax = x.speedManualMove;
    z.speedMax = z.speedManualMove;
    stepToFinal(&x, xSurface);
    stepToFinal(&z, zCenter);
    if (x.pos == xSurface && z.pos == zCenter) {
      setIsOnFromLoop(false);
      beepFlag = true;
    }
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

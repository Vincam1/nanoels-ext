#include "spindle.h"
#include "display.h"   // spindleModulo, getPitchAxis, spindleFromPos
#include "stepper.h"   // stepperIsRunning

void processSpindleCounter() {
  int16_t count;
  pcnt_get_counter_value(PCNT_UNIT_0, &count);
  int delta = count - spindleCount;
  if (delta == 0) return;

  if (count >= PCNT_CLEAR || count <= -PCNT_CLEAR) {
    pcnt_counter_clear(PCNT_UNIT_0);
    spindleCount = 0;
  } else {
    spindleCount = count;
  }

  unsigned long microsNow = micros();
  if (spindleEncTimeIndex >= rpmBulk) {
    spindleEncTimeDiffBulk   = microsNow - spindleEncTimeAtIndex0;
    spindleEncTimeAtIndex0   = microsNow;
    spindleEncTimeIndex      = 0;
  }
  spindleEncTimeIndex += abs(delta);

  spindlePos       += delta;
  spindlePosGlobal += delta;
  if (spindlePosGlobal > encoderStepsInt)  spindlePosGlobal -= encoderStepsInt;
  else if (spindlePosGlobal < 0)           spindlePosGlobal += encoderStepsInt;

  if (spindlePos > spindlePosAvg) {
    spindlePosAvg = spindlePos;
  } else if (spindlePos < spindlePosAvg - encoderBacklash) {
    spindlePosAvg = spindlePos + encoderBacklash;
  }
  spindleEncTime = microsNow;

  if (spindlePosSync != 0) {
    spindlePosSync += delta;
    if (spindlePosSync % encoderStepsInt == 0) {
      spindlePosSync = 0;
      Axis* a = getPitchAxis();
      spindlePosAvg = spindlePos = spindleFromPos(a, a->pos);
    }
  }
}

void discountFullSpindleTurns() {
  if (dupr != 0 && !stepperIsRunning(&z) && (mode == MODE_NORMAL || mode == MODE_CONE)) {
    int spindlePosDiff = 0;
    if (z.pos == z.rightStop) {
      long stopSpindlePos = spindleFromPos(&z, z.rightStop);
      if (dupr > 0) {
        if (spindlePos < stopSpindlePos - encoderStepsInt) spindlePosDiff =  encoderStepsInt;
      } else {
        if (spindlePos > stopSpindlePos + encoderStepsInt) spindlePosDiff = -encoderStepsInt;
      }
    } else if (z.pos == z.leftStop) {
      long stopSpindlePos = spindleFromPos(&z, z.leftStop);
      if (dupr > 0) {
        if (spindlePos > stopSpindlePos + encoderStepsInt) spindlePosDiff = -encoderStepsInt;
      } else {
        if (spindlePos < stopSpindlePos - encoderStepsInt) spindlePosDiff =  encoderStepsInt;
      }
    }
    if (spindlePosDiff != 0) {
      spindlePos    += spindlePosDiff;
      spindlePosAvg += spindlePosDiff;
    }
  }
}

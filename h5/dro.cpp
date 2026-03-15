#include "dro.h"
#include "globals.h"
#include "stepper.h"

DRO_Scale zScale((gpio_num_t)Z_SCALE_A, (gpio_num_t)Z_SCALE_B, PCNT_UNIT_4, DEFAULT_Z_SCALE_PPM);
DRO_Scale xScale((gpio_num_t)X_SCALE_A, (gpio_num_t)X_SCALE_B, PCNT_UNIT_5, DEFAULT_X_SCALE_PPM);

void attachScales() {
  if (zDroActive) zScale.attach();
  if (xDroActive) xScale.attach();
}

void syncAxisToScale(Axis* a, DRO_Scale& scale) {
  // Convert scale mm → stepper steps using the axis lead-screw geometry.
  // screwPitch is in deci-microns (du); 1 mm = 10000 du.
  long scaleSteps = lroundf(scale.getPosition() * 10000.0f / a->screwPitch * a->motorSteps);
  long drift = scaleSteps - a->pos;
  if (drift == 0) return;
  bool hasSemaphore = xSemaphoreTake(a->mutex, 10) == pdTRUE;
  a->pos      = scaleSteps;
  a->motorPos += drift;
  if (hasSemaphore) xSemaphoreGive(a->mutex);
}

long measureBacklash(Axis* a, DRO_Scale& scale, bool direction) {
  if (!a->active || a->disabled) return -1;

  stepperEnable(a, true);
  setDir(a, direction);

  float   startPos  = scale.getPosition();
  long    pulses    = 0;
  long    maxPulses = (long)a->motorSteps / 2;  // half revolution maximum
  int     sign      = direction ? 1 : -1;

  while (pulses < maxPulses) {
    stepToFinal(a, a->pos + sign);
    waitForPendingPos0(a);
    pulses++;
    // 3 µm threshold — safely above X4 noise floor (≈1.25 µm/count) while
    // still resolving backlash of a few microns.
    if (fabsf(scale.getPosition() - startPos) > 0.003f) break;
  }

  stepperEnable(a, false);

  if (pulses >= maxPulses) return -1;
  return lroundf((float)pulses * a->screwPitch / a->motorSteps);
}

#pragma once

#include "DRO_Scale.h"
#include "config.h"
#include "types.h"

// Global scale instances — PCNT_UNIT_4 and PCNT_UNIT_5 are used so that the
// existing handwheel units (1-3) and spindle unit (0) are not disturbed.
extern DRO_Scale zScale;
extern DRO_Scale xScale;

// Called from taskAttachInterrupts() to start hardware counting if active.
void attachScales();

// Feature 2 — Sync stepper position to scale after a manual move settles.
// Computes the scale position in stepper steps and updates a->pos / a->motorPos
// to eliminate accumulated open-loop error. Must be called with pendingPos == 0.
void syncAxisToScale(Axis* a, DRO_Scale& scale);

// Feature 3 — Backlash calibration.
// Pulses the axis one step at a time in `direction` until the scale detects
// movement of at least 3 µm, or until motorSteps/2 pulses are exhausted.
// Returns the measured backlash in deci-microns, or -1 on failure (scale not
// active, axis disabled, or scale didn't move within the pulse limit).
// Caller must ensure pendingPos == 0 and the machine is OFF before calling.
long measureBacklash(Axis* a, DRO_Scale& scale, bool direction);

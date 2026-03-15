#pragma once

#include "DRO_Scale.h"
#include "config.h"

// Global scale instances — PCNT_UNIT_4 and PCNT_UNIT_5 are used so that the
// existing handwheel units (1-3) and spindle unit (0) are not disturbed.
extern DRO_Scale zScale;
extern DRO_Scale xScale;

// Called from taskAttachInterrupts() to start hardware counting if active.
void attachScales();

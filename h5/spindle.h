#pragma once

#include "globals.h"

// Process the latest hardware spindle encoder counter value.
// Called every iteration of loop().
void processSpindleCounter();

// When standing at a stop, discount full spindle turns to avoid
// waiting for the spindle to re-sync after direction reversal.
// Called every iteration of loop().
void discountFullSpindleTurns();

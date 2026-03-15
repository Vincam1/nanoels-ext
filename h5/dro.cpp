#include "dro.h"
#include "globals.h"

DRO_Scale zScale((gpio_num_t)Z_SCALE_A, (gpio_num_t)Z_SCALE_B, PCNT_UNIT_4, DEFAULT_Z_SCALE_PPM);
DRO_Scale xScale((gpio_num_t)X_SCALE_A, (gpio_num_t)X_SCALE_B, PCNT_UNIT_5, DEFAULT_X_SCALE_PPM);

void attachScales() {
  if (zDroActive) zScale.attach();
  if (xDroActive) xScale.attach();
}

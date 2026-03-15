#include "DRO_Scale.h"
#include <driver/pcnt.h>

volatile int32_t DRO_Scale::overflowCount[MAX_ENCODERS] = {0};

DRO_Scale::DRO_Scale(gpio_num_t pinA, gpio_num_t pinB, pcnt_unit_t unit, float scale, int filterValue) {
  aPinNumber = pinA;
  bPinNumber = pinB;
  pcntUnit   = unit;
  scaleFactor = scale;
  filter     = filterValue;
  puType     = up;
}

void IRAM_ATTR DRO_Scale::pcnt_intr_handler(void* arg) {
  pcnt_unit_t unit = (pcnt_unit_t)(intptr_t)arg;
  uint32_t status;
  pcnt_get_event_status(unit, &status);
  if (status & PCNT_EVT_H_LIM) {
    overflowCount[unit] += 32768;
  } else if (status & PCNT_EVT_L_LIM) {
    overflowCount[unit] -= 32768;
  }
}

void DRO_Scale::attach() {
  esp_rom_gpio_pad_select_gpio(aPinNumber);
  gpio_set_direction(aPinNumber, GPIO_MODE_INPUT);
  esp_rom_gpio_pad_select_gpio(bPinNumber);
  gpio_set_direction(bPinNumber, GPIO_MODE_INPUT);

  if (puType == up) {
    gpio_pullup_en(aPinNumber);
    gpio_pullup_en(bPinNumber);
  }

  pcnt_config_t pcnt_config = {};
  pcnt_config.pulse_gpio_num = aPinNumber;
  pcnt_config.ctrl_gpio_num  = bPinNumber;
  pcnt_config.channel        = PCNT_CHANNEL_0;
  pcnt_config.unit           = pcntUnit;
  pcnt_config.pos_mode       = PCNT_COUNT_INC;
  pcnt_config.neg_mode       = PCNT_COUNT_DEC;
  pcnt_config.lctrl_mode     = PCNT_MODE_REVERSE;
  pcnt_config.hctrl_mode     = PCNT_MODE_KEEP;
  pcnt_config.counter_h_lim  = 32767;
  pcnt_config.counter_l_lim  = -32768;
  pcnt_unit_config(&pcnt_config);

  if (filter > 0) {
    pcnt_set_filter_value(pcntUnit, filter);
    pcnt_filter_enable(pcntUnit);
  }

  pcnt_counter_clear(pcntUnit);
  pcnt_counter_resume(pcntUnit);
  pcnt_event_enable(pcntUnit, PCNT_EVT_H_LIM);
  pcnt_event_enable(pcntUnit, PCNT_EVT_L_LIM);

  static bool isrServiceInstalled = false;
  if (!isrServiceInstalled) {
    pcnt_isr_service_install(0);
    isrServiceInstalled = true;
  }
  pcnt_isr_handler_add(pcntUnit, pcnt_intr_handler, (void*)(intptr_t)pcntUnit);
  pcnt_intr_enable(pcntUnit);
}

float DRO_Scale::getRawPosition() {
  int16_t currentHardwareCount = 0;
  pcnt_get_counter_value(pcntUnit, &currentHardwareCount);
  int32_t total_count = overflowCount[pcntUnit] + currentHardwareCount;
  return total_count / scaleFactor;
}

float DRO_Scale::getPosition() {
  return getRawPosition() + positionOffset;
}

void DRO_Scale::setPosition(float desiredPosition) {
  positionOffset = desiredPosition - getRawPosition();
}

void DRO_Scale::clearCount() {
  overflowCount[pcntUnit] = 0;
  pcnt_counter_clear(pcntUnit);
  positionOffset = 0.0f;
}

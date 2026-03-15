#ifndef _DRO_SCALE_h
#define _DRO_SCALE_h

#include <Arduino.h>
#include <driver/gpio.h>
#include <driver/pcnt.h>

#define MAX_ENCODERS 8

enum encType  { full };
enum pullupType { up };

class DRO_Scale {
private:
  gpio_num_t  aPinNumber;
  gpio_num_t  bPinNumber;
  pullupType  puType;
  pcnt_unit_t pcntUnit;
  int         filter;
  float       scaleFactor;           // pulses per mm
  float       positionOffset = 0.0f; // offset for zeroing / known-position entry

  static volatile int32_t overflowCount[MAX_ENCODERS]; // ISR-managed overflow accumulator

  static void IRAM_ATTR pcnt_intr_handler(void* arg);

public:
  DRO_Scale(gpio_num_t pinA, gpio_num_t pinB, pcnt_unit_t unit, float scale, int filterValue = 100);

  void  attach();
  float getPosition();               // Returns position including offset (mm)
  float getRawPosition();            // Returns raw hardware position without offset (mm)
  void  setPosition(float desiredPosition); // Shift offset so getPosition() returns desiredPosition
  void  clearCount();                // Zero both raw count and offset
};

#endif

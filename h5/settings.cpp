#include "settings.h"
#include "storage.h"
#include "display.h"
#include "stepper.h"
#include "dro.h"

// ============================================================
// OPEN SETTINGS PAGE
// Navigate to Nextion page 2 and populate all fields with the
// current hardware configuration values.
// ============================================================

void openSettingsPage() {
  toScreen("page 2");

  // Encoder
  setText("tEncPPR",  String(encoderPPR));
  setText("tEncBL",   String(encoderBacklash));

  // Z axis
  setText("tScrewZ",  String(screwZDu));
  setText("tMotorZ",  String(motorStepsZ));
  setText("tBLZ",     String(backlashDuZ));

  // X axis
  setText("tScrewX",  String(screwXDu));
  setText("tMotorX",  String(motorStepsX));
  setText("tBLX",     String(backlashDuX));

  // Y axis
  setText("tMotorY",  String(motorStepsY));
  setText("tScrewY",  String(screwYDu));

  // Status line
  setText("tSettingsStatus", "Edit values then press Save");
}

// ============================================================
// READ A NUMBER BACK FROM A NEXTION TEXT COMPONENT
//
// Nextion doesn't natively send component values back over
// serial — only button press events. To retrieve edited values
// we send a "get componentId.txt" command and parse the
// response (0x70 packet: 0x70 + string bytes + 0xFF 0xFF 0xFF).
// ============================================================

long readNumberFromDisplay(const String& componentId, long defaultValue) {
  // Flush any stale bytes in the receive buffer
  while (Serial1.available()) Serial1.read();

  // Request the component value
  toScreen("get " + componentId + ".txt");

  // Wait up to 200ms for the response
  unsigned long start = millis();
  while (millis() - start < 200) {
    if (Serial1.available() >= 5) break;
    delay(1);
  }
  if (!Serial1.available()) return defaultValue;

  // Read response packet: 0x70 + ASCII string + 0xFF 0xFF 0xFF
  byte header = Serial1.read();
  if (header != 0x70) {
    while (Serial1.available()) Serial1.read();
    return defaultValue;
  }

  String result = "";
  while (Serial1.available()) {
    byte b = Serial1.read();
    if (b == 0xFF) {
      // Consume remaining two 0xFF terminators
      while (Serial1.available() && Serial1.peek() == 0xFF) Serial1.read();
      break;
    }
    result += (char)b;
  }

  result.trim();
  if (result.length() == 0) return defaultValue;
  return result.toInt();
}

// ============================================================
// SAVE SETTINGS FROM DISPLAY
// Read edited values back from Nextion components, write to
// Preferences, and reboot so new config takes effect.
// ============================================================

void saveSettingsFromDisplay() {
  setText("tSettingsStatus", "Saving...");

  // Read each editable field back from the display
  encoderPPR      = (int)readNumberFromDisplay("tEncPPR",  encoderPPR);
  encoderBacklash = (int)readNumberFromDisplay("tEncBL",   encoderBacklash);

  screwZDu        = readNumberFromDisplay("tScrewZ",  screwZDu);
  motorStepsZ     = readNumberFromDisplay("tMotorZ",  motorStepsZ);
  backlashDuZ     = readNumberFromDisplay("tBLZ",     backlashDuZ);

  screwXDu        = readNumberFromDisplay("tScrewX",  screwXDu);
  motorStepsX     = readNumberFromDisplay("tMotorX",  motorStepsX);
  backlashDuX     = readNumberFromDisplay("tBLX",     backlashDuX);

  motorStepsY     = readNumberFromDisplay("tMotorY",  motorStepsY);
  screwYDu        = readNumberFromDisplay("tScrewY",  screwYDu);

  // Sanity-check critical values so a typo can't brick the machine
  if (encoderPPR      < 1)    encoderPPR      = DEFAULT_ENCODER_PPR;
  if (motorStepsZ     < 1)    motorStepsZ     = DEFAULT_MOTOR_STEPS_Z;
  if (motorStepsX     < 1)    motorStepsX     = DEFAULT_MOTOR_STEPS_X;
  if (motorStepsY     < 1)    motorStepsY     = DEFAULT_MOTOR_STEPS_Y;
  if (screwZDu        < 100)  screwZDu        = DEFAULT_SCREW_Z_DU;
  if (screwXDu        < 100)  screwXDu        = DEFAULT_SCREW_X_DU;
  if (screwYDu        < 100)  screwYDu        = DEFAULT_SCREW_Y_DU;

  saveHardwareSettings();

  setText("tSettingsStatus", "Saved! Rebooting...");
  delay(1500);

  // Reboot so new hardware config is applied cleanly from scratch
  ESP.restart();
}

// ============================================================
// BACKLASH CALIBRATION VIA DRO SCALE
// ============================================================

static void measureAndSaveBacklash(Axis* a, DRO_Scale& scale, bool& droActive,
                                   long& backlashDu, const char* axisName) {
  if (isOn) {
    setText("tSettingsStatus", String(axisName) + " BL: turn OFF first");
    return;
  }
  if (!droActive) {
    setText("tSettingsStatus", String(axisName) + " BL: scale not active");
    return;
  }
  setText("tSettingsStatus", String(axisName) + " BL: measuring...");

  // Measure in the negative direction (reversal after a positive jog).
  long result = measureBacklash(a, scale, false);

  if (result < 0) {
    setText("tSettingsStatus", String(axisName) + " BL: failed - jog + first");
    return;
  }

  backlashDu       = result;
  a->backlashSteps = lroundf((float)backlashDu * a->motorSteps / a->screwPitch);
  saveHardwareSettings();

  setText("tSettingsStatus",
    String(axisName) + " BL=" + printDeciMicrons(backlashDu, 3) + " saved");
}

void measureAndSaveBacklashZ() {
  measureAndSaveBacklash(&z, zScale, zDroActive, backlashDuZ, "Z");
}

void measureAndSaveBacklashX() {
  measureAndSaveBacklash(&x, xScale, xDroActive, backlashDuX, "X");
}

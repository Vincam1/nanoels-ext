#pragma once

#include <driver/pcnt.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// ============================================================
// OPERATING MODES
// ============================================================
#define MODE_NORMAL  0
#define MODE_ASYNC   2
#define MODE_CONE    3
#define MODE_TURN    4
#define MODE_FACE    5
#define MODE_CUT     6
#define MODE_THREAD  7
#define MODE_ELLIPSE 8
#define MODE_GCODE   9
#define MODE_Y       10

// ============================================================
// MEASUREMENT UNITS
// ============================================================
#define MEASURE_METRIC 0
#define MEASURE_INCH   1
#define MEASURE_TPI    2

// ============================================================
// EMERGENCY STOP CODES
// ============================================================
#define ESTOP_NONE           0
#define ESTOP_POS            2
#define ESTOP_MARK_ORIGIN    3
#define ESTOP_ON_OFF         4
#define ESTOP_OFF_MANUAL_MOVE 5

// ============================================================
// MOVE STEP SIZES (deci-microns)
// ============================================================
#define MOVE_STEP_1     10000L  // 1mm
#define MOVE_STEP_2     1000L   // 0.1mm
#define MOVE_STEP_3     100L    // 0.01mm

#define MOVE_STEP_IMP_1 25400L  // 0.1"
#define MOVE_STEP_IMP_2 2540L   // 0.01"
#define MOVE_STEP_IMP_3 254L    // 0.001" (1 thou)

// ============================================================
// KEYBOARD / NEXTION BUTTON CODES
// ============================================================
#define B_LEFT       21  // Left arrow  — Z axis left
#define B_RIGHT      22  // Right arrow — Z axis right
#define B_UP         23  // Up arrow    — X axis forward
#define B_DOWN       24  // Down arrow  — X axis backward
#define B_FORWARD    85  // u — Y axis advance
#define B_BACK       74  // j — Y axis retreat
#define B_MINUS      60  // Numpad minus
#define B_PLUS       95  // Numpad plus
#define B_ON         30  // Enter — start
#define B_OFF        27  // ESC   — stop
#define B_STOPL      65  // a — set Z left stop
#define B_STOPR      68  // d — set Z right stop
#define B_STOPU      87  // w — set X forward stop
#define B_STOPD      83  // s — set X rear stop
#define B_STOPF      73  // i — set Y forward stop
#define B_STOPB      75  // k — set Y backward stop
#define B_MULTISTART 84  // t — multi-start thread
#define B_DISPL      12  // Win — cycle display info (angle/rpm/...)
#define B_STEP       64  // Tilde — cycle manual step size
#define B_MEASURE    77  // m — cycle metric/imperial/tpi
#define B_REVERSE    82  // r — reverse pitch sign
#define B_DIAMETER   79  // o — set X0 from diameter
#define B_0          48
#define B_1          49
#define B_2          50
#define B_3          51
#define B_4          52
#define B_5          53
#define B_6          54
#define B_7          55
#define B_8          56
#define B_9          57
#define B_BACKSPACE  28
#define B_MODE_GEARS   97  // F1
#define B_MODE_TURN    98  // F2
#define B_MODE_FACE    99  // F3
#define B_MODE_CONE   100  // F4
#define B_MODE_CUT    101  // F5
#define B_MODE_THREAD 102  // F6
#define B_MODE_ASYNC  103  // F7
#define B_MODE_ELLIPSE 104 // F8
#define B_MODE_GCODE  105  // F9
#define B_MODE_Y      106  // F10
#define B_MODE        107  // F11 — cycle modes
#define B_X           88   // x — zero X axis
#define B_Z           90   // z — zero Z axis
#define B_Y           72   // h — zero Y axis
#define B_X_ENA       67   // c — enable/disable X
#define B_Z_ENA       81   // q — enable/disable Z
#define B_Y_ENA       89   // y — enable/disable Y

// Settings page button codes (Nextion page 2)
#define B_SETTINGS_OPEN   108  // Open HMI settings page
#define B_SETTINGS_SAVE   109  // Save settings to flash and reboot
#define B_SETTINGS_CANCEL 110  // Cancel and return to main page

// DRO toggle — tap the tZ / tX display area on Nextion to switch between
// scale readout (green) and stepper position (white). Nextion button IDs 52/53.
#define B_Z_DRO  111  // Toggle Z axis DRO mode
#define B_X_DRO  112  // Toggle X axis DRO mode

// Backlash calibration — settings page (page 2) buttons, Nextion IDs 12/13.
// Jog the axis in the positive direction first, then tap to measure backlash
// on the reversal. Result is saved directly to Preferences.
#define B_MEASURE_BL_Z  113  // Measure and save Z backlash via scale
#define B_MEASURE_BL_X  114  // Measure and save X backlash via scale

// ============================================================
// PREFERENCES STORAGE KEYS
// Runtime state (axis positions, mode, etc.)
// ============================================================
#define PREF_VERSION          "v"
#define PREF_DUPR             "d"
#define PREF_POS_Z            "zp"
#define PREF_LEFT_STOP_Z      "zls"
#define PREF_RIGHT_STOP_Z     "zrs"
#define PREF_ORIGIN_POS_Z     "zpo"
#define PREF_POS_GLOBAL_Z     "zpg"
#define PREF_MOTOR_POS_Z      "zpm"
#define PREF_DISABLED_Z       "zd"
#define PREF_POS_X            "xp"
#define PREF_LEFT_STOP_X      "xls"
#define PREF_RIGHT_STOP_X     "xrs"
#define PREF_ORIGIN_POS_X     "xpo"
#define PREF_POS_GLOBAL_X     "xpg"
#define PREF_MOTOR_POS_X      "xpm"
#define PREF_DISABLED_X       "xd"
#define PREF_POS_Y            "y1p"
#define PREF_LEFT_STOP_Y      "y1ls"
#define PREF_RIGHT_STOP_Y     "y1rs"
#define PREF_ORIGIN_POS_Y     "y1po"
#define PREF_POS_GLOBAL_Y     "y1pg"
#define PREF_MOTOR_POS_Y      "y1pm"
#define PREF_DISABLED_Y       "y1d"
#define PREF_SPINDLE_POS      "sp"
#define PREF_SPINDLE_POS_AVG  "spa"
#define PREF_OUT_OF_SYNC      "oos"
#define PREF_SPINDLE_POS_GLOBAL "spg"
#define PREF_SHOW_ANGLE       "ang"
#define PREF_SHOW_TACHO       "rpm"
#define PREF_STARTS           "sta"
#define PREF_MODE             "mod"
#define PREF_MEASURE          "mea"
#define PREF_CONE_RATIO       "cr"
#define PREF_TURN_PASSES      "tp"
#define PREF_MOVE_STEP        "ms"
#define PREF_AUX_FORWARD      "af"

// Hardware configuration keys (user-settable via HMI settings page)
#define PREF_HW_ENC_PPR       "hw_eppr"
#define PREF_HW_ENC_BL        "hw_ebl"
#define PREF_HW_SCREW_Z       "hw_szdu"
#define PREF_HW_MOTOR_Z       "hw_mz"
#define PREF_HW_INV_Z         "hw_iz"
#define PREF_HW_INV_ZE        "hw_ize"
#define PREF_HW_REST_Z        "hw_rz"
#define PREF_HW_TRAVEL_Z      "hw_tz"
#define PREF_HW_BL_Z          "hw_blz"
#define PREF_HW_SCREW_X       "hw_sxdu"
#define PREF_HW_MOTOR_X       "hw_mx"
#define PREF_HW_INV_X         "hw_ix"
#define PREF_HW_INV_XE        "hw_ixe"
#define PREF_HW_REST_X        "hw_rx"
#define PREF_HW_TRAVEL_X      "hw_tx"
#define PREF_HW_BL_X          "hw_blx"
#define PREF_HW_ACTIVE_Y      "hw_ay"
#define PREF_HW_ROTARY_Y      "hw_roty"
#define PREF_HW_MOTOR_Y       "hw_my"
#define PREF_HW_SCREW_Y       "hw_sydu"
#define PREF_HW_SPD_START_Y   "hw_ssy"
#define PREF_HW_ACCEL_Y       "hw_acy"
#define PREF_HW_SPD_MAN_Y     "hw_smany"
#define PREF_HW_INV_Y         "hw_iy"
#define PREF_HW_INV_YE        "hw_iye"
#define PREF_HW_REST_Y        "hw_ryy"
#define PREF_HW_TRAVEL_Y      "hw_tyy"
#define PREF_HW_BL_Y          "hw_bly"

// ============================================================
// DATA STRUCTURES
// ============================================================

struct CircleBuffer {
  char*  buffer;
  size_t head;
  size_t tail;
  size_t size;
};

struct Axis {
  SemaphoreHandle_t mutex;

  char  name;
  bool  active;
  bool  rotational;
  float motorSteps;   // Steps per motor revolution
  float screwPitch;   // Lead screw pitch in deci-microns (10^-7 m)

  long  pos;               // Tool position relative to origin, in stepper steps
  long  savedPos;
  float fractionalPos;     // Fractional steps accumulated but not yet issued
  long  originPos;         // Stepper position of the origin point
  long  savedOriginPos;
  long  posGlobal;         // Absolute stepper position (never zeroed)
  long  savedPosGlobal;
  int   pendingPos;        // Steps to issue as soon as possible
  long  motorPos;          // Actual motor position (differs from pos during backlash travel)
  long  savedMotorPos;
  bool  continuous;        // true if movement is expected to continue indefinitely

  long leftStop;           // Left/forward soft limit in stepper steps
  long savedLeftStop;
  long nextLeftStop;       // Pending left stop value (applied by main loop)
  bool nextLeftStopFlag;

  long rightStop;          // Right/rear soft limit in stepper steps
  long savedRightStop;
  long nextRightStop;
  bool nextRightStopFlag;

  long speed;              // Current motor speed in steps/second
  long speedStart;         // Initial speed (steps/second)
  long speedMax;           // Speed cap for current operation
  long speedManualMove;    // Maximum speed for manual moves
  long acceleration;       // Acceleration in steps/second^2
  long decelerateSteps;    // Steps before target to begin decelerating

  bool direction;
  bool directionInitialized;
  unsigned long stepStartUs;
  int  stepperEnableCounter;
  bool disabled;
  bool savedDisabled;

  bool invertStepper;      // Invert motor direction
  bool invertEnable;       // Invert enable pin polarity
  bool needsRest;          // true = open-loop driver (disable when idle)
  bool movingManually;
  long estopSteps;         // Steps beyond which position is considered out of bounds
  long backlashSteps;      // Steps to travel in reverse to re-engage the lead screw
  long gcodeRelativePos;   // Reference position for relative GCode moves

  int ena;                 // Enable pin
  int dir;                 // Direction pin
  int step;                // Step pin

  int pulseA;
  int pulseB;
  int pulseCount;
  pcnt_unit_t pulseUnit;
};

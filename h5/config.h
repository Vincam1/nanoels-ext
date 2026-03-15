#pragma once

// https://github.com/kachurovskiy/nanoels

// ============================================================
// USER-CONFIGURABLE HARDWARE DEFAULTS
//
// These values are used on FIRST BOOT only. After that, the
// values saved in flash (Preferences) take precedence.
// They can be updated at runtime via the HMI Settings page
// without reflashing firmware.
// ============================================================

// Spindle rotary encoder
#define DEFAULT_ENCODER_PPR      1200  // Steps per revolution. Fractional values not supported.
#define DEFAULT_ENCODER_BACKLASH 3     // Impulses encoder can issue without actual spindle movement

// Z axis (main lead screw)
#define DEFAULT_SCREW_Z_DU       40000L // 4mm SFU1204 ball screw in deci-microns (10^-7 m)
#define DEFAULT_MOTOR_STEPS_Z    800L   // Stepper steps per revolution
#define DEFAULT_INVERT_Z         false  // Flip if carriage moves opposite to expected direction
#define DEFAULT_INVERT_Z_ENABLE  false  // Flip if Z enable pin is active-high instead of active-low
#define DEFAULT_NEEDS_REST_Z     false  // true for open-loop drivers, false for closed-loop
#define DEFAULT_MAX_TRAVEL_MM_Z  300L   // Maximum travel in mm (used for estop calculation)
#define DEFAULT_BACKLASH_DU_Z    0L     // Backlash compensation in deci-microns

// X axis (cross-slide)
#define DEFAULT_SCREW_X_DU       40000L
#define DEFAULT_MOTOR_STEPS_X    800L
#define DEFAULT_INVERT_X         true
#define DEFAULT_INVERT_X_ENABLE  false
#define DEFAULT_NEEDS_REST_X     false
#define DEFAULT_MAX_TRAVEL_MM_X  100L
#define DEFAULT_BACKLASH_DU_X    0L

// Y axis (optional: dividing head / rotary)
// Throughout Y config, 1mm = 1 degree, so 1du = 0.0001 degree.
#define DEFAULT_ACTIVE_Y         false  // Set true if Y axis is physically connected
#define DEFAULT_ROTARY_Y         true   // true = rotary (degrees), false = linear (mm)
#define DEFAULT_MOTOR_STEPS_Y    300L
#define DEFAULT_SCREW_Y_DU       20000L // 2 degrees per worm gear turn
#define DEFAULT_SPEED_START_Y    1600L
#define DEFAULT_ACCELERATION_Y   16000L
#define DEFAULT_SPEED_MANUAL_MOVE_Y 3200L
#define DEFAULT_INVERT_Y         false
#define DEFAULT_INVERT_Y_ENABLE  false
#define DEFAULT_NEEDS_REST_Y     false
#define DEFAULT_MAX_TRAVEL_MM_Y  360L
#define DEFAULT_BACKLASH_DU_Y    0L

// Manual handwheels (ignored if not installed)
#define PULSE_PER_REVOLUTION     600.0f // PPR of handwheel encoders

// Manual stepping button timing
#define STEP_TIME_MS             500L   // Time in ms for one manual step
#define DELAY_BETWEEN_STEPS_MS   80L    // Pause between steps in ms

// WiFi / Web UI
#define WIFI_ENABLED             true
#define DEFAULT_SSID             "your-wifi-name"
#define DEFAULT_PASSWORD         "your-password"
#define INCOMING_BUFFER_SIZE     100000L
#define OUTGOING_BUFFER_SIZE     100000L

// ============================================================
// FIXED SYSTEM CONSTANTS (cannot be changed at runtime)
// ============================================================

// Encoder hardware filter - pulses shorter than this are ignored (1-1023 clock cycles)
#define ENCODER_FILTER           1

// Hardware pulse counter limits
#define PCNT_LIM                 31000
#define PCNT_CLEAR               30000  // Reset counter here to avoid overflow; must be < PCNT_LIM

// Motion limits
#define DUPR_MAX                 254000L  // Maximum pitch: 1 inch
#define STARTS_MAX               124      // Maximum thread starts
#define PASSES_MAX               999L     // Maximum turn/face passes
#define SAFE_DISTANCE_DU         5000L    // Retract distance between passes: 0.5mm

// Timing
#define SAVE_DELAY_US            5000000L // Wait 5s after last activity before writing flash
#define DIRECTION_SETUP_DELAY_US 5L       // Stepper driver direction settle time (microseconds)
#define STEPPED_ENABLE_DELAY_MS  100L     // Delay after stepper enable before issuing steps

// Display formatting
#define TPI_ROUND_EPSILON        0.03f    // Round TPI to integer if within this tolerance

// GCode
#define LINEAR_INTERPOLATION_PRECISION 0.1f // Chunk size for G0/G1 (smaller = smoother, slower)
#define GCODE_WAIT_EPSILON_STEPS 10L
#define SPINDLE_PAUSES_GCODE     true     // Pause GCode when spindle stops
#define GCODE_MIN_RPM            30       // Minimum RPM to run GCode
#define GCODE_FEED_DEFAULT_DU_SEC 20000L  // Default feed rate in du/sec
#define GCODE_FEED_MIN_DU_SEC    167.0f   // Minimum feed rate (F1)

// Async timer
#define TIMER_FREQ               1000000  // 1MHz

// Filesystem
#define FORMAT_LITTLEFS_IF_FAILED true

// ============================================================
// THREAD CUTTING STRATEGY DEFAULTS
// ============================================================
#define DEFAULT_THREAD_CUT_MODE       0     // THREAD_CUT_RADIAL
#define DEFAULT_THREAD_ANGLE_TENTHS   600   // 60° (metric ISO); use 550 for 55° inch/BSW
// Modification angle for THREAD_CUT_MODIFIED (degrees subtracted from half-angle)
// Industry recommendation: 2–3°; results in slight trailing-flank engagement for better finish.
#define THREAD_FLANK_DELTA_DEG        2.0f

// ============================================================
// GROOVE MODE DEFAULTS
// ============================================================
// Tool corner radius (deci-microns). 1000 = 0.1 mm (typical MRMN 1mm-radius insert = 10000).
#define DEFAULT_GROOVE_TOOL_RADIUS_DU 10000.0f
// Straight groove (V-belt) flank angle × 10.  340 = 34°, 380 = 38°.
#define DEFAULT_GROOVE_STRAIGHT_ANGLE_TENTHS 340

// Storage versioning - increment when storage format changes (wipes old prefs)
#define PREFERENCES_VERSION      1
#define PREF_NAMESPACE           "h5"

// Firmware versioning
#define SOFTWARE_VERSION         14
#define HARDWARE_VERSION         5

// ============================================================
// PIN DEFINITIONS (hardware-specific, cannot change at runtime)
// ============================================================

// Spindle encoder pins — swap ENC_A / ENC_B if rotation direction is wrong
#define ENC_A     13
#define ENC_B     14

// Z axis stepper
#define Z_ENA     41
#define Z_DIR     42
#define Z_STEP    35

// Z axis handwheel encoder
#define Z_PULSE_A 18
#define Z_PULSE_B 8

// X axis stepper
#define X_ENA     16
#define X_DIR     15
#define X_STEP    7

// X axis handwheel encoder
#define X_PULSE_A 47
#define X_PULSE_B 21

// Y axis stepper
#define Y_ENA     1
#define Y_DIR     2
#define Y_STEP    17

// Y axis handwheel encoder
#define Y_PULSE_A 45
#define Y_PULSE_B 48

// PS2 keyboard
#define KEY_DATA  37
#define KEY_CLOCK 36

// Axis display names (also used as GCode axis letters in switch cases — must stay as #define)
#define NAME_Z 'Z'
#define NAME_X 'X'
#define NAME_Y 'Y'

// Linear scale DRO — GPIO pins (from PCB netlist)
#define Z_SCALE_A              9      // Z scale quadrature A
#define Z_SCALE_B              12     // Z scale quadrature B
#define X_SCALE_A              40     // X scale quadrature A
#define X_SCALE_B              39     // X scale quadrature B

// Pulses per mm — must be calibrated for the specific scale installed.
// 800 ppm corresponds to a typical 5µm-resolution magnetic linear scale
// with X4 (full quadrature) decoding: 4 counts per 5 µm cycle = 800 counts/mm.
// For a 1µm scale use 4000; for a 10µm scale use 400. Calibrate empirically.
#define DEFAULT_Z_SCALE_PPM    800.0f
#define DEFAULT_X_SCALE_PPM    800.0f

// Set to true if a linear scale is physically connected and should be read.
// Leave false (default) when no scale is installed — avoids floating-input noise.
#define DEFAULT_Z_DRO_ACTIVE   false
#define DEFAULT_X_DRO_ACTIVE   false

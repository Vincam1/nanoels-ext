#include "globals.h"

// ============================================================
// RUNTIME HARDWARE CONFIGURATION
// Initial values are the compile-time defaults from config.h.
// These get overwritten from Preferences during setup().
// ============================================================

// Encoder
int   encoderPPR      = DEFAULT_ENCODER_PPR;
int   encoderBacklash = DEFAULT_ENCODER_BACKLASH;

// Derived encoder constants (recomputed after loading encoderPPR)
int   encoderStepsInt   = DEFAULT_ENCODER_PPR * 2;
float encoderStepsFloat = DEFAULT_ENCODER_PPR * 2;
long  rpmBulk           = DEFAULT_ENCODER_PPR * 2;

// Z axis hardware settings
long  screwZDu      = DEFAULT_SCREW_Z_DU;
long  motorStepsZ   = DEFAULT_MOTOR_STEPS_Z;
bool  invertZ       = DEFAULT_INVERT_Z;
bool  invertZEnable = DEFAULT_INVERT_Z_ENABLE;
bool  needsRestZ    = DEFAULT_NEEDS_REST_Z;
long  maxTravelMmZ  = DEFAULT_MAX_TRAVEL_MM_Z;
long  backlashDuZ   = DEFAULT_BACKLASH_DU_Z;

// X axis hardware settings
long  screwXDu      = DEFAULT_SCREW_X_DU;
long  motorStepsX   = DEFAULT_MOTOR_STEPS_X;
bool  invertX       = DEFAULT_INVERT_X;
bool  invertXEnable = DEFAULT_INVERT_X_ENABLE;
bool  needsRestX    = DEFAULT_NEEDS_REST_X;
long  maxTravelMmX  = DEFAULT_MAX_TRAVEL_MM_X;
long  backlashDuX   = DEFAULT_BACKLASH_DU_X;

// Y axis hardware settings
bool  activeY           = DEFAULT_ACTIVE_Y;
bool  rotaryY           = DEFAULT_ROTARY_Y;
long  motorStepsY       = DEFAULT_MOTOR_STEPS_Y;
long  screwYDu          = DEFAULT_SCREW_Y_DU;
long  speedStartY       = DEFAULT_SPEED_START_Y;
long  accelerationY     = DEFAULT_ACCELERATION_Y;
long  speedManualMoveY  = DEFAULT_SPEED_MANUAL_MOVE_Y;
bool  invertY           = DEFAULT_INVERT_Y;
bool  invertYEnable     = DEFAULT_INVERT_Y_ENABLE;
bool  needsRestY        = DEFAULT_NEEDS_REST_Y;
long  maxTravelMmY      = DEFAULT_MAX_TRAVEL_MM_Y;
long  backlashDuY       = DEFAULT_BACKLASH_DU_Y;

// ============================================================
// AXIS OBJECTS
// ============================================================
Axis z;
Axis x;
Axis y;

// ============================================================
// RUNTIME STATE
// ============================================================

SemaphoreHandle_t motionMutex = nullptr;

// Spindle / encoder
unsigned long spindleEncTime           = 0;
unsigned long spindleEncTimeDiffBulk   = 0;
unsigned long spindleEncTimeAtIndex0   = 0;
int           spindleEncTimeIndex      = 0;
long          spindlePos               = 0;
long          spindlePosAvg            = 0;
long          savedSpindlePosAvg       = 0;
long          savedSpindlePos          = 0;
int           spindleCount             = 0;
int           spindlePosSync           = 0;
int           savedSpindlePosSync      = 0;
long          spindlePosGlobal         = 0;
long          savedSpindlePosGlobal    = 0;

// Display / RPM
bool          showAngle       = false;
bool          showTacho       = false;
bool          savedShowAngle  = false;
bool          savedShowTacho  = false;
int           shownRpm        = 0;
unsigned long shownRpmTime    = 0;

#define LCD_HASH_INITIAL -3845709  // Random seed unlikely to collide with real hashes
long          lcdHashLine0    = LCD_HASH_INITIAL;
long          lcdHashLine1    = LCD_HASH_INITIAL;
long          lcdHashLine2    = LCD_HASH_INITIAL;
long          lcdHashLine3    = LCD_HASH_INITIAL;
bool          splashScreen    = false;

// On/off state
bool          isOn            = false;
bool          nextIsOn        = false;
bool          nextIsOnFlag    = false;
unsigned long resetMillis     = 0;
int           emergencyStop   = 0;
bool          beepFlag        = false;

// Pitch / threading
long  dupr          = 0;
long  savedDupr     = 0;
long  nextDupr      = 0;
bool  nextDuprFlag  = false;
int   starts        = 1;
int   savedStarts   = 0;
int   nextStarts    = 1;
bool  nextStartsFlag = false;

// Mode
volatile int mode       = -1;
int          nextMode   = 0;
bool         nextModeFlag = false;
int          savedMode  = -1;

// Measurement
int measure       = MEASURE_METRIC;
int savedMeasure  = MEASURE_METRIC;

// Cone
float coneRatio        = 1.0f;
float savedConeRatio   = 0.0f;
float nextConeRatio    = 0.0f;
bool  nextConeRatioFlag = false;

// Taper preset (-1 = none/custom, 0-7 = MT0-MT7, 8-14 = JT0-JT6, 15 = JT33)
int  taperPreset        = 2;    // Default: MT2
int  savedTaperPreset   = -2;   // Force write on first run

// Pass / automation
int   turnPasses        = 3;
int   savedTurnPasses   = 0;
long  setupIndex        = 0;
bool  auxForward        = true;
bool  savedAuxForward   = false;
long  opIndex           = 0;
bool  opIndexAdvanceFlag = false;
long  opSubIndex        = 0;
int   opDuprSign        = 1;
long  opDupr            = 0;

// Threading cutting strategy
int  threadCutMode           = DEFAULT_THREAD_CUT_MODE;
int  savedThreadCutMode      = -1;  // force write on first save
int  threadAngleTenths       = DEFAULT_THREAD_ANGLE_TENTHS;
int  savedThreadAngleTenths  = -1;
long threadSpindleOffset     = 0;   // computed per pass, not persisted

// Groove mode parameters
float grooveToolRadiusDu             = DEFAULT_GROOVE_TOOL_RADIUS_DU;
float savedGrooveToolRadiusDu        = -1.0f;
int   grooveStraightAngleTenths      = DEFAULT_GROOVE_STRAIGHT_ANGLE_TENTHS;
int   savedGrooveStraightAngleTenths = -1;

// Manual step
long moveStep       = 0;
long savedMoveStep  = 0;

// Manual button state
bool buttonLeftPressed    = false;
bool buttonRightPressed   = false;
bool buttonUpPressed      = false;
bool buttonDownPressed    = false;
bool buttonOffPressed     = false;
bool buttonBackPressed    = false;
bool buttonForwardPressed = false;

// Numpad
bool inNumpad         = false;
int  numpadDigits[20] = {};
int  numpadIndex      = 0;

// GCode
String gcodeCommand              = "";
long   gcodeFeedDuPerSec         = GCODE_FEED_DEFAULT_DU_SEC;
bool   gcodeInitialized          = false;
bool   gcodeAbsolutePositioning  = true;
bool   gcodeInBrace              = false;
bool   gcodeInSemicolon          = false;
bool   wsInKeycode               = false;
int    wsKeycode                 = 0;
String keycodeCommand            = "";
bool   gcodeInSave               = false;
bool   gcodeInSaveFirstLine      = false;
String gcodeSaveName             = "";
String gcodeSaveValue            = "";
int    gcodeProgramIndex         = 0;
int    gcodeProgramCount         = 0;
String gcodeProgram              = "";
int    gcodeProgramCharIndex     = 0;

// WiFi / WebSocket buffers
CircleBuffer  inBuffer;
CircleBuffer  outBuffer;
String        wifiStatus         = "No WiFi";
unsigned long wifiStatusMillis   = 0;

// Timing
unsigned long saveTime              = 0;
unsigned long lastDisplayUpdateTime = 0;
unsigned long keypadTimeUs          = 0;

// Async timer
hw_timer_t* async_timer  = nullptr;
bool        timerAttached = false;

// Keyboard
PS2KeyAdvanced keyboard;

// Nextion protocol buffer
const int NEXTION_BUFFER_LENGTH = 256;
byte      nextionBuffer[256];
int       nextionBufferIndex    = 0;
byte      lastNextionPageId     = 255;

// Multi-start button timing
unsigned long multistartPressMillis = 0;

// DRO (linear scale) state
bool zDroActive = DEFAULT_Z_DRO_ACTIVE;
bool xDroActive = DEFAULT_X_DRO_ACTIVE;
bool zDroMode   = false;
bool xDroMode   = false;

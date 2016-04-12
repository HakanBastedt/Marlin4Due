/****************************************************************************************
* Arduino Due pin assignment
*
* for HB LASER experiméntal board
****************************************************************************************/

//

#define X_STEP_PIN         63
#define X_DIR_PIN          62
#define X_ENABLE_PIN       12
#define X_MIN_PIN          -1
#define X_MAX_PIN          49

#define Y_STEP_PIN         65
#define Y_DIR_PIN          64
#define Y_ENABLE_PIN        9
#define Y_MIN_PIN          50
#define Y_MAX_PIN          -1 //38

#define Z_STEP_PIN         1 // MUST
#define Z_DIR_PIN          1 // MUST
#define Z_ENABLE_PIN       1 // MUST
#define Z_MIN_PIN          -1 // 26
#define Z_MAX_PIN          -1

#define E0_STEP_PIN        1 // MUST
#define E0_DIR_PIN         1 // MUST
#define E0_ENABLE_PIN      1 // MUST

#define E1_STEP_PIN        -1
#define E1_DIR_PIN         -1
#define E1_ENABLE_PIN      -1

#define E2_STEP_PIN        -1
#define E2_DIR_PIN         -1
#define E2_ENABLE_PIN      -1

#define SDPOWER            -1
#define SDSS               4
#define LED_PIN            13

#define BEEPER             -1

#define FAN_PIN             -1

#define PS_ON_PIN          -1

#define KILL_PIN           -1 //Dawson

#define LASER_0_PIN         4
#define LASER_1_PIN         5

#define HEATER_BED_PIN     -1    // BED //Dawson - OTW, not on the right power feed!

#define HEATER_0_PIN       11 // MOSFET 1, Park them here
#define HEATER_1_PIN       10 // MOSFET 2
#define HEATER_2_PIN       -1 // Dawson - use this for stepper fan control

#define TEMP_BED_PIN       -1   // ANALOG NUMBERING

#define TEMP_0_PIN          1   // MUST
#define TEMP_1_PIN         -1  // 2    // ANALOG NUMBERING
#define TEMP_2_PIN         -1  // 3     // ANALOG NUMBERING

#define TEMP_3_PIN         -1   // ANALOG NUMBERING
#define TEMP_4_PIN         -1   // ANALOG NUMBERING

// --------------------------------------------------------------------------
// 
// --------------------------------------------------------------------------


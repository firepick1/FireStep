#ifndef PINS_H
#define PINS_H

#define NOPIN 255
typedef uint8_t PinType;		// pin specification

enum PinConfig {
    PC0_NOPIN = 0,
    PC1_EMC02 = 1,
    PC2_RAMPS_1_4 = 2,
};

#define MAX_PIN				   69

///////////////////// PC0_NOPIN ////////////////
// No pin assignments
////////////////////////////////////////////////

#define PC0_X_STEP_PIN          NOPIN
#define PC0_X_DIR_PIN           NOPIN
#define PC0_X_ENABLE_PIN        NOPIN
#define PC0_X_MIN_PIN           NOPIN
#define PC0_X_MAX_PIN           NOPIN

#define PC0_Y_STEP_PIN          NOPIN
#define PC0_Y_DIR_PIN           NOPIN
#define PC0_Y_ENABLE_PIN        NOPIN
#define PC0_Y_MIN_PIN           NOPIN
#define PC0_Y_MAX_PIN           NOPIN

#define PC0_Z_STEP_PIN          NOPIN
#define PC0_Z_DIR_PIN           NOPIN
#define PC0_Z_ENABLE_PIN        NOPIN
#define PC0_Z_MIN_PIN           NOPIN
#define PC0_Z_MAX_PIN           NOPIN

#define PC0_STATUS_PIN			NOPIN

///////////// PC1_EMC02 //////////////
// https://github.com/firepick-delta/Marlin/blob/delta/openpnp/Marlin/pins.h
//////////////////////////////////////

#define PC1_SERVO1			   4
#define PC1_SERVO2			   5
#define PC1_SERVO3			   6
#define PC1_SERVO4			   7

#define PC1_PROBE_PIN		   PC1_SERVO3
#define PC1_STATUS_PIN	 	   PC1_SERVO4
#define PC1_LED_PIN            13

//Delta Motor "X" ***********
#define PC1_X_STEP_PIN         38 //X is the BACK motor!
#define PC1_X_DIR_PIN          36
#define PC1_X_ENABLE_PIN       40
#define PC1_X_MIN_PIN          63 // A9
#define PC1_X_MAX_PIN          NOPIN

//Delta Motor "Y" ***********
#define PC1_Y_STEP_PIN         56 //Y is the FRONT LEFT motor!
#define PC1_Y_DIR_PIN          57
#define PC1_Y_ENABLE_PIN       54
#define PC1_Y_MIN_PIN          64
#define PC1_Y_MAX_PIN          NOPIN

//Delta Motor "Z" ***********
#define PC1_Z_STEP_PIN         9 //Z is the FRONT RIGHT motor!
#define PC1_Z_DIR_PIN          8
#define PC1_Z_ENABLE_PIN       2
#define PC1_Z_MIN_PIN          65
#define PC1_Z_MAX_PIN          NOPIN

//All modular tools
#define PC1_TOOL_STEP_PIN      30
#define PC1_TOOL_DIR_PIN       32

#define PC1_TOOL1_ENABLE_PIN   44
#define PC1_TOOL2_ENABLE_PIN   52
#define PC1_TOOL3_ENABLE_PIN   24
#define PC1_TOOL4_ENABLE_PIN   34 // UNUSED since FireStep has 6 axes, not 7

#define PC1_TOOL1_DOUT         44
#define PC1_TOOL2_DOUT         50
#define PC1_TOOL3_DOUT         22
#define PC1_TOOL4_DOUT         26

#define PC1_PWR_SUPPLY_PIN     28


///////////////// PC2_RAMPS_1_4 ////////////////
// Arduino Mega2560 with RAMPS v1.4 shield
////////////////////////////////////////////////

#define PC2_SERVO1			   4
#define PC2_SERVO2			   5
#define PC2_SERVO3			   6
#define PC2_SERVO4			   11

#define PC2_PROBE_PIN			2
#define PC2_STATUS_PIN		PC2_SERVO4

#define PC2_ANALOG_SPEED_PIN	5 /* ADC5 (A5) */

#define PC2_X_STEP_PIN         54 //A0
#define PC2_X_DIR_PIN          55 //A1
#define PC2_X_ENABLE_PIN       38
#define PC2_X_MIN_PIN           3 //3
#define PC2_X_MAX_PIN          NOPIN

#define PC2_Y_STEP_PIN         60 //A6
#define PC2_Y_DIR_PIN          61 //A7
#define PC2_Y_ENABLE_PIN       56 //A2
#define PC2_Y_MIN_PIN          14 //seems fried on current 2560 //14
#define PC2_Y_MAX_PIN          NOPIN

#define PC2_Z_STEP_PIN         46
#define PC2_Z_DIR_PIN          48
#define PC2_Z_ENABLE_PIN       62 //A8
#define PC2_Z_MIN_PIN          18 //18
#define PC2_Z_MAX_PIN          NOPIN

#define PC2_E0_STEP_PIN        26 // RepRap
#define PC2_E0_DIR_PIN         28 // RepRap
#define PC2_E0_ENABLE_PIN      24 // RepRap

#define PC2_E1_STEP_PIN        36 // RepRap
#define PC2_E1_DIR_PIN         34 // RepRap
#define PC2_E1_ENABLE_PIN      30 // RepRap

#define PC2_LED_PIN            13
#define PC2_FAN_PIN            NOPIN
#define PC2_PS_ON_PIN          12

//MOSFETS:
#define PC2_HEATER_0_PIN       10   // D10 EXTRUDER 1
#define PC2_HEATER_1_PIN       NOPIN
#define PC2_HEATER_2_PIN       NOPIN
#define PC2_HEATER_BED_PIN     8    // BED +12V2, 11A

//ANALOG / THERMISTORS:
#define PC2_TEMP_0_PIN         13   // A13 
#define PC2_TEMP_1_PIN         15   // A15 
#define PC2_TEMP_2_PIN         NOPIN
#define PC2_TEMP_BED_PIN       14   // A14 

#endif

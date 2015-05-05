#ifndef RAMPS_1_4_H
#define RAMPS_1_4_H

//                   +----------------+
//                   | 5V GND D1  D0  |
// RAMPS AUX1 FRONT  | 5V GND D57 D58 |
//                   +----------------+
#define DISPLAY_PIN 57
#define ANALOG_SPEED_PIN	5 /* ADC5 (A5) */

//****************************************************************************************
// RAMPS 1.4
//****************************************************************************************
#define KNOWN_BOARD 1

#define LARGE_FLASH true

#define X_STEP_PIN         54 //A0
#define X_DIR_PIN          55 //A1
#define X_ENABLE_PIN       38
#define X_MIN_PIN           3 //3
#define X_MAX_PIN          -1 //2

#define Y_STEP_PIN         60 //A6
#define Y_DIR_PIN          61 //A7
#define Y_ENABLE_PIN       56 //A2
#define Y_MIN_PIN          14 //seems fried on current 2560 //14
#define Y_MAX_PIN          -1 //15

#define Z_STEP_PIN         46
#define Z_DIR_PIN          48
#define Z_ENABLE_PIN       62 //A8
#define Z_MIN_PIN          18 //18
#define Z_MAX_PIN          -1 //19 (auto Z leveling probe)

#define E0_STEP_PIN        26
#define E0_DIR_PIN         28
#define E0_ENABLE_PIN      24

#define E1_STEP_PIN        36
#define E1_DIR_PIN         34
#define E1_ENABLE_PIN      30

#define LED_PIN            13

#define FAN_PIN            -1 // (Sprinter config)

#define PS_ON_PIN          12

//MOSFETS:
#define HEATER_0_PIN       10   // D10 EXTRUDER 1
#define HEATER_1_PIN       -1   // D9  (Use this for VACUUM instead!)
#define HEATER_2_PIN       -1   // N/C on RAMPS
#define HEATER_BED_PIN     8    // BED +12V2, 11A

//ANALOG / THERMISTORS:
#define TEMP_0_PIN         13   // A13 
#define TEMP_1_PIN         15   // A15 
#define TEMP_2_PIN         -1   // N/C on RAMPS
#define TEMP_BED_PIN       14   // A14 

#endif

#ifndef EMC01_H
#define EMC01_H

#define LED_PIN_RED NOPIN
#define LED_PIN_GRN NOPIN
#define ANALOG_SPEED_PIN	NOPIN

//***********************************
// Stuff specific to FirePick Delta (most of this section is a hack)
//***********************************
//#define VACUUM_PIN         30 // Modular Tool #1 for now...
//#define NEOPIXEL_PIN       16 //For LED ring light on end effector
//#define SERVO0_PIN         11 //X [0]
//#define SERVO1_PIN         6  //Y [1]
//#define SERVO2_PIN         5  //Z [2]
//#define SERVO4_PIN         4  //  [3]

//****************************************************************************************
// FirePick Delta EMC01 pin assignment (custom PCB based on ATMega1284P)
//****************************************************************************************
#define KNOWN_BOARD 1

#define LARGE_FLASH true

//Delta Motor "X" ***********
#define X_STEP_PIN         23 //X is the BACK motor!
#define X_DIR_PIN          22
#define X_ENABLE_PIN       14
#define X_MIN_PIN          21
#define X_MAX_PIN          -1

//Delta Motor "Y" ***********
#define Y_STEP_PIN         4 //Y is the FRONT LEFT motor!
#define Y_DIR_PIN          3
#define Y_ENABLE_PIN       14
#define Y_MIN_PIN          2
#define Y_MAX_PIN          -1

//Delta Motor "Z" ***********
#define Z_STEP_PIN         13 //Z is the FRONT RIGHT motor!
#define Z_DIR_PIN          12
#define Z_ENABLE_PIN       14
#define Z_MIN_PIN          11
#define Z_MAX_PIN          -1

//Modular Tool #1 ***********
#define E0_STEP_PIN        20
#define E0_DIR_PIN         15
#define E0_ENABLE_PIN      1
#define VACUUM_PIN         30 // Modular Tool #1 for now...

//Modular Tool #2 ***********
#define E1_STEP_PIN        20
#define E1_DIR_PIN         15
#define E1_ENABLE_PIN      0
#define HEATER_0_PIN       28 //Digital numbering
#define TEMP_0_PIN         A2 //Analog numbering

//Modular Tool #3 ***********
#define E2_STEP_PIN        20
#define E2_DIR_PIN         15
#define E2_ENABLE_PIN      19
#define HEATER_1_PIN       27 //Digital numbering
#define TEMP_1_PIN         A5 //Analog numbering

//Modular Tool #4 ***********
#define E3_STEP_PIN        20
#define E3_DIR_PIN         15
#define E3_ENABLE_PIN      18
#define HEATER_2_PIN       25
#define TEMP_2_PIN         A7 //Analog numbering

#define HEATER_BED_PIN     -1 //25 //Digital numbering
#define TEMP_BED_PIN       -1 //A7 //Analog numbering

//Vestigial Leftovers *******
#define LED_PIN            -1 //Not used on EMC01
#define FAN_PIN            -1 //Not used on EMC01
#define PS_ON_PIN          -1 //Not currently used, but might be on next pcb rev...

#endif

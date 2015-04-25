#ifndef PINS_H
#define PINS_H

#define NOPIN 255

#define LED_PIN_RED 13
#define LED_PIN_GRN 12

#define PIN_X_DIR	2
#define PIN_X		3
#define PIN_Y_DIR	5
#define PIN_Y		6
#define PIN_Z_DIR	9
#define PIN_Z		10

#define ANALOG_SPEED_PIN	5 /* ADC5 (A5) */

#ifdef PRR
/* DIECIMILA */
#define PIN_X_LIM	14
#define PIN_Y_LIM	15
#define PIN_Z_LIM	16
#else
/* MEGA */
#define PIN_X_LIM	54
#define PIN_Y_LIM	55
#define PIN_Z_LIM	56
#endif

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
//#ifdef MOTHERBOARD == 639 //RAMPS board
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
  
//#endif //EMC01 pinout

//****************************************************************************************
// RAMPS 1.4
//****************************************************************************************
//#ifdef MOTHERBOARD == 33 //RAMPS board
//  #define KNOWN_BOARD 1
//  
//  #define LARGE_FLASH true
//  
//  #define X_STEP_PIN         54 //A0
//  #define X_DIR_PIN          55 //A1
//  #define X_ENABLE_PIN       38
//  #define X_MIN_PIN           3 //3
//  #define X_MAX_PIN          -1 //2
//  
//  #define Y_STEP_PIN         60 //A6
//  #define Y_DIR_PIN          61 //A7
//  #define Y_ENABLE_PIN       56 //A2
//  #define Y_MIN_PIN          14 //seems fried on current 2560 //14
//  #define Y_MAX_PIN          -1 //15
//  
//  #define Z_STEP_PIN         46
//  #define Z_DIR_PIN          48
//  #define Z_ENABLE_PIN       62 //A8
//  #define Z_MIN_PIN          18 //18
//  #define Z_MAX_PIN          -1 //19 (auto Z leveling probe)
//  
//  #define E0_STEP_PIN        26
//  #define E0_DIR_PIN         28
//  #define E0_ENABLE_PIN      24
//  
//  #define E1_STEP_PIN        36
//  #define E1_DIR_PIN         34
//  #define E1_ENABLE_PIN      30
//  
//  #define LED_PIN            13
//  
//  #define FAN_PIN            -1 // (Sprinter config)
//  
//  #define PS_ON_PIN          12
//  
//  //MOSFETS:
//  #define HEATER_0_PIN       10   // D10 EXTRUDER 1
//  #define HEATER_1_PIN       -1   // D9  (Use this for VACUUM instead!)
//  #define HEATER_2_PIN       -1   // N/C on RAMPS
//  #define HEATER_BED_PIN     8    // BED +12V2, 11A
//  
//  //ANALOG / THERMISTORS:
//  #define TEMP_0_PIN         13   // A13 
//  #define TEMP_1_PIN         15   // A15 
//  #define TEMP_2_PIN         -1   // N/C on RAMPS
//  #define TEMP_BED_PIN       14   // A14 
//#endif //RAMPS pinout

typedef uint8_t PinType;		// pin specification


#endif

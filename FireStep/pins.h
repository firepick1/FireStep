#ifndef PINS_H
#define PINS_H

#define NOPIN 255
typedef uint8_t PinType;		// pin specification

//****************************************************************************************
// The NOPIN configuration
//****************************************************************************************

  #define PC0_X_STEP_PIN         NOPIN
  #define PC0_X_DIR_PIN          NOPIN
  #define PC0_X_ENABLE_PIN       NOPIN
  #define PC0_X_MIN_PIN          NOPIN
  #define PC0_X_MAX_PIN          NOPIN
  
  #define PC0_Y_STEP_PIN         NOPIN
  #define PC0_Y_DIR_PIN          NOPIN
  #define PC0_Y_ENABLE_PIN       NOPIN
  #define PC0_Y_MIN_PIN          NOPIN
  #define PC0_Y_MAX_PIN          NOPIN
  
  #define PC0_Z_STEP_PIN         NOPIN
  #define PC0_Z_DIR_PIN          NOPIN
  #define PC0_Z_ENABLE_PIN       NOPIN
  #define PC0_Z_MIN_PIN          NOPIN
  #define PC0_Z_MAX_PIN          NOPIN

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
//#if MOTHERBOARD == 639 //RAMPS board
//#ifdef BOARD_EMC01
  //#define KNOWN_BOARD 1
  
  //#define LARGE_FLASH true
  
  //Delta Motor "X" ***********
  #define PC1_X_STEP_PIN         23 //X is the BACK motor!
  #define PC1_X_DIR_PIN          22
  #define PC1_X_ENABLE_PIN       14
  #define PC1_X_MIN_PIN          21
  #define PC1_X_MAX_PIN          NOPIN
  
  //Delta Motor "Y" ***********
  #define PC1_Y_STEP_PIN         4 //Y is the FRONT LEFT motor!
  #define PC1_Y_DIR_PIN          3
  #define PC1_Y_ENABLE_PIN       14
  #define PC1_Y_MIN_PIN          2
  #define PC1_Y_MAX_PIN          NOPIN
  
  //Delta Motor "Z" ***********
  #define PC1_Z_STEP_PIN         13 //Z is the FRONT RIGHT motor!
  #define PC1_Z_DIR_PIN          12
  #define PC1_Z_ENABLE_PIN       14
  #define PC1_Z_MIN_PIN          11
  #define PC1_Z_MAX_PIN          NOPIN
  
  //Modular Tool #1 ***********
  #define PC1_E0_STEP_PIN        20
  #define PC1_E0_DIR_PIN         15
  #define PC1_E0_ENABLE_PIN      1
  #define PC1_VACUUM_PIN         30 // Modular Tool #1 for now...
  
  //Modular Tool #2 ***********
  #define PC1_E1_STEP_PIN        20
  #define PC1_E1_DIR_PIN         15
  #define PC1_E1_ENABLE_PIN      0
  #define PC1_HEATER_0_PIN       28 //Digital numbering
  #define PC1_TEMP_0_PIN         A2 //Analog numbering
  
  //Modular Tool #3 ***********
  #define PC1_E2_STEP_PIN        20
  #define PC1_E2_DIR_PIN         15
  #define PC1_E2_ENABLE_PIN      19
  #define PC1_HEATER_1_PIN       27 //Digital numbering
  #define PC1_TEMP_1_PIN         A5 //Analog numbering

  //Modular Tool #4 ***********
  #define PC1_E3_STEP_PIN        20
  #define PC1_E3_DIR_PIN         15
  #define PC1_E3_ENABLE_PIN      18
  #define PC1_HEATER_2_PIN       25
  #define PC1_TEMP_2_PIN         A7 //Analog numbering

  #define PC1_HEATER_BED_PIN     NOPIN
  #define PC1_TEMP_BED_PIN       NOPIN

  //Vestigial Leftovers *******
  #define PC1_LED_PIN            NOPIN
  #define PC1_FAN_PIN            NOPIN
  #define PC1_PS_ON_PIN          NOPIN
 
//#endif //EMC01 pinout

//****************************************************************************************
// RAMPS 1.4
//****************************************************************************************
//#if MOTHERBOARD == 33 //RAMPS board
//#ifdef BOARD_RAMPS
  //#define KNOWN_BOARD 1
  
  // RAMPS AUX1 BACK   [ 5V GND D1  D0  ]
  // RAMPS AUX1 FRNT   [ 5V GND D57 D58 ]
  #define PC2_DISPLAY_PIN 57  
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
  
  #define PC2_E0_STEP_PIN        26
  #define PC2_E0_DIR_PIN         28
  #define PC2_E0_ENABLE_PIN      24
  
  #define PC2_E1_STEP_PIN        36
  #define PC2_E1_DIR_PIN         34
  #define PC2_E1_ENABLE_PIN      30
  
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
//#endif //RAMPS pinout

#endif

#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include <ph5.h>
#include "Arduino.h"
#include "MachineThread.h"
#include "NeoPixel.h"

///////////////////// CHOOSE DEFAULT PIN CONFIGURATION ///////////
//#define PIN_CONFIG PC2_RAMPS_1_4
#define PIN_CONFIG PC1_EMC02

firestep::MachineThread machineThread; // FireStep command interpreter

/////////// NeoPixel display driver /////////////
#define NEOPIXEL_LEDS 16
#if PIN_CONFIG == PC1_EMC02
firestep::NeoPixel neoPixel(NEOPIXEL_LEDS, PC1_DISPLAY_PIN); // EMC02
#define LED_PIN PC1_LED_PIN
#else
firestep::NeoPixel neoPixel(NEOPIXEL_LEDS, PC2_DISPLAY_PIN); // RAMPS1.4
#define LED_PIN PC2_LED_PIN
#endif

void setup() { // run once, when the sketch starts
    // Serial I/O has lowest priority, so you may need to
    // decrease baud rate to fix Serial I/O problems.
    //Serial.begin(38400); // short USB cables
    Serial.begin(19200); // long USB cables

    // Bind in NeoPixel display driver
    machineThread.machine.pDisplay = &neoPixel;

    // Initialize
    machineThread.setup(PIN_CONFIG);
    
    //FirePick Delta specific stuff
    pinMode(PC1_TOOL1_ENABLE_PIN,OUTPUT);
    pinMode(PC1_TOOL2_ENABLE_PIN,OUTPUT);
    pinMode(PC1_TOOL3_ENABLE_PIN,OUTPUT);
    pinMode(PC1_TOOL4_ENABLE_PIN,OUTPUT);
    pinMode(PC1_PWR_SUPPLY_PIN,OUTPUT);
    pinMode(PC1_TOOL1_DOUT,OUTPUT);
    pinMode(PC1_TOOL2_DOUT,OUTPUT);
    pinMode(PC1_TOOL3_DOUT,OUTPUT);
    pinMode(PC1_TOOL4_DOUT,OUTPUT);
    pinMode(PC1_SERVO1,OUTPUT);
    pinMode(PC1_SERVO2,OUTPUT);
    pinMode(PC1_SERVO3,OUTPUT);
    pinMode(PC1_SERVO4,OUTPUT);
    digitalWrite(PC1_TOOL1_ENABLE_PIN,HIGH);
    digitalWrite(PC1_TOOL2_ENABLE_PIN,HIGH);
    digitalWrite(PC1_TOOL3_ENABLE_PIN,HIGH);
    digitalWrite(PC1_TOOL4_ENABLE_PIN,HIGH);
    digitalWrite(PC1_PWR_SUPPLY_PIN,LOW);
    digitalWrite(PC1_TOOL1_DOUT,LOW);
    digitalWrite(PC1_TOOL2_DOUT,LOW);
    digitalWrite(PC1_TOOL3_DOUT,LOW);
    digitalWrite(PC1_TOOL4_DOUT,LOW);
    digitalWrite(PC1_SERVO1,LOW);
    digitalWrite(PC1_SERVO2,LOW);
    digitalWrite(PC1_SERVO3,LOW);
    digitalWrite(PC1_SERVO4,LOW);
    
    
    firestep::threadRunner.setup(LED_PIN);
}

void loop() {	// run over and over again
    firestep::threadRunner.run();
}


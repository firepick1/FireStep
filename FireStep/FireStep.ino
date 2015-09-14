#include "ArduinoJson.h"
#include <Adafruit_NeoPixel.h>

#include "Mega2560.h"
//#include <ArduinoJson.h>
#include <ph5.h>
//#include "Arduino.h"
#include "MachineThread.h"
#include "NeoPixel.h"

///////////////////// CHOOSE DEFAULT PIN CONFIGURATION ///////////
//#define PIN_CONFIG PC2_RAMPS_1_4
#define PIN_CONFIG PC1_EMC02

firestep::MachineThread machineThread; // FireStep command interpreter

/////////// NeoPixel display driver /////////////
#define NEOPIXEL_LEDS 16
firestep::NeoPixel neoPixel(NEOPIXEL_LEDS);

#if PIN_CONFIG == PC1_EMC02
#define LED_PIN PC1_LED_PIN
#else
#define LED_PIN PC2_LED_PIN
#endif

Mega2560 mega2560;

void setup() { // run once, when the sketch starts
    // Serial I/O has lowest priority, so you may need to
    // decrease baud rate to fix Serial I/O problems.
    //Serial.begin(38400); // short USB cables
    Serial.begin(19200); // long USB cables

    // Bind in NeoPixel display driver
    machineThread.machine.pDisplay = &neoPixel;

    // Initialize
    machineThread.setup(&mega2560, PIN_CONFIG);

    firestep::threadRunner.setup(LED_PIN);
}

void loop() {	// run over and over again
    firestep::threadRunner.run();
}


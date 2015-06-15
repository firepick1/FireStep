#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include <ph5.h>
#include "Arduino.h"
#include "MachineThread.h"
#include "NeoPixel.h"

firestep::MachineThread machineThread; // FireStep command interpreter

/////////// NeoPixel display driver (Choose ONE)/////////////
#define NEOPIXEL_LEDS 16
firestep::NeoPixel neoPixel(NEOPIXEL_LEDS, PC2_DISPLAY_PIN); // RAMPS1.4

void setup() { // run once, when the sketch starts
    // Serial I/O has lowest priority, so you may need to
    // decrease baud rate to fix Serial I/O problems.
    Serial.begin(38400);

    // Bind in NeoPixel display driver
    machineThread.machine.pDisplay = &neoPixel;

    // Initialize
    machineThread.setup(PC2_RAMPS_1_4);

    firestep::threadRunner.setup(PC2_LED_PIN);
}

void loop() {	// run over and over again
    firestep::threadRunner.run();
}


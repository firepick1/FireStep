#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include <ph5.h>
#include "Arduino.h"
#include "MachineThread.h"
#include "NeoPixel.h"

firestep::MachineThread machineThread; // FireStep command interpreter

/////////// NeoPixel display driver (Choose ONE)/////////////
#define NEOPIXEL_LEDS 16
#ifdef TEST
#define PIN_CONFIG PC2_RAMPS_1_4
#else
#define PIN_CONFIG PC1_EMC02
#fi

#if PIN_CONFIG == PC1_EMC02
firestep::NeoPixel neoPixel(NEOPIXEL_LEDS, PC1_DISPLAY_PIN); // EMC02
#define LED_PIN PC1_LED_PIN
#else
firestep::NeoPixel neoPixel(NEOPIXEL_LEDS, PC2_DISPLAY_PIN); // RAMPS1.4
#define LED_PIN PC2_LED_PIN
#fi

void setup() { // run once, when the sketch starts
    // Serial I/O has lowest priority, so you may need to
    // decrease baud rate to fix Serial I/O problems.
    Serial.begin(38400);

    // Bind in NeoPixel display driver
    machineThread.machine.pDisplay = &neoPixel;

    // Initialize
    machineThread.setup(PIN_CONFIG);

    firestep::threadRunner.setup(LED_PIN);
}

void loop() {	// run over and over again
    firestep::threadRunner.run();
}


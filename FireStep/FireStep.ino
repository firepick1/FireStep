#include <Adafruit_NeoPixel.h>

#include <ArduinoJson.h>
#include <ph5.h>
#include "Arduino.h"
#include "MachineThread.h"
#include "NeoPixel.h"
#include "git_tag.h"

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

void setup() { // run once, when the sketch starts
    // Serial I/O has lowest priority, so you may need to
    // decrease baud rate to fix Serial I/O problems.
    //Serial.begin(38400); // short USB cables
    Serial.begin(19200); // long USB cables
	char buf[100];
	strcpy_P(buf, GIT_TAG);
	Serial.println(buf);

    // Bind in NeoPixel display driver
    machineThread.machine.pDisplay = &neoPixel;

    // Initialize
    machineThread.setup(PIN_CONFIG);

    firestep::threadRunner.setup(LED_PIN);
}

void loop() {	// run over and over again
    firestep::threadRunner.run();
}


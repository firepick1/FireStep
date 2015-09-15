// Arduino Oddness...
// THe following MUST be included explicitly in FireStep.ino
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include <ph5.h>
#include "Mega2560.h"
#include "MachineThread.h"
#include "NeoPixel.h"

///////////////////// CHOOSE DEFAULT PIN CONFIGURATION ///////////
//#define PIN_CONFIG PC2_RAMPS_1_4
#define PIN_CONFIG PC1_EMC02

/////////// NeoPixel display driver /////////////
#define NEOPIXEL_LEDS 16
firestep::NeoPixel neoPixel(NEOPIXEL_LEDS);

#if PIN_CONFIG == PC1_EMC02
#define LED_PIN PC1_LED_PIN
#else
#define LED_PIN PC2_LED_PIN
#endif

using namespace firestep;

Mega2560 mega2560;

#define MACHINE_ACTIVE
#ifdef MACHINE_ACTIVE
Machine machine(&mega2560);
#endif
#define MACHINETHREAD_ACTIVE
#ifdef MACHINETHREAD_ACTIVE
MachineThread machineThread(machine); 
#endif

//#define SELFTEST

void setup() { // run once, when the sketch starts
    // Serial I/O has lowest priority, so you may need to
    // decrease baud rate to fix Serial I/O problems.
    //Serial.begin(38400); // short USB cables
    Serial.begin(19200); // long USB cables

	mega2560.setup();

#ifdef MACHINE_ACTIVE
    machine.pDisplay = &neoPixel; // Inject NeoPixel driver
#endif
#ifdef MACHINETHREAD_ACTIVE
    machineThread.setup(PIN_CONFIG); // Set up FireStep pins
    threadRunner.setup(&mega2560, LED_PIN);
#endif
#ifdef SELFTEST
	delay(3000); // wait for user to open serial window
	mega2560.selftest();
#endif
}

void loop() {	// run over and over again
#ifdef SELFTEST
	static int secs=0;
	mega2560.serial_println(secs++);
	delay(1000);
#else
    threadRunner.run();
#endif
}


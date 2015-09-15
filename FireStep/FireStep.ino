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

namespace firestep {int __heap_start, *__brkval;}

const char startup[] PROGMEM = 			{ "Startup	: FireStep" };
const char startup_println[] PROGMEM = 	{ "Startup	: mega2560.serial_println()" };
const char startup_pinMode[] PROGMEM = 	{ "Startup	: mega2560.pinMode(4, OUTPUT)" };
const char startup_led_on[] PROGMEM = 	{ "Startup	: mega2560.digitalWrite(4, HIGH)" };
const char startup_enableTicks[] PROGMEM = 	{ "Startup	: mega2560.enableTicks()" };
const char startup_ticks[] PROGMEM = 	{ "Startup	: mega2560.ticks() " };
const char startup_delay[] PROGMEM = 	{ "Startup	: mega2560.delay(1000)" };
const char startup_led_off[] PROGMEM = 	{ "Startup	: mega2560.digitalWrite(4, LOW)" };
const char startup_size_mach[] PROGMEM = 	{ "Startup	: sizeof(Machine) " };
const char startup_size_mt[] PROGMEM = 	{ "Startup	: sizeof(MachineThread) " };
const char startup_error[] PROGMEM = 	{ "Startup	: ERROR" };
const char startup_done[] PROGMEM = 	{ "Startup	: complete" };

/**
 * Perform some simple startup tests
 */
bool startup_IDuino(firestep::IDuinoPtr pDuino) {
	char buf[100];

	strcpy_P(buf, startup);
	Serial.println();
	Serial.println(buf);

	// Test serial 
	strcpy_P(buf, startup_println);
	pDuino->serial_println(buf);
	strcpy_P(buf, startup_pinMode);
	pDuino->serial_println(buf);
	pDuino->pinMode(PC1_SERVO1, OUTPUT);
	strcpy_P(buf, startup_led_on);
	pDuino->serial_println(buf);

	// Turn on pin #4 (SERVO1) LED
	pDuino->digitalWrite(PC1_SERVO1, HIGH);
	strcpy_P(buf, startup_enableTicks);
	pDuino->serial_println(buf);

	// Start ticks()
	pDuino->enableTicks(true);
	if (!pDuino->isTicksEnabled()) {
		strcpy_P(buf, startup_error);
		pDuino->serial_println(buf);
		return false;
	}
	Ticks tStart = pDuino->ticks();
	strcpy_P(buf, startup_delay);
	pDuino->serial_print(buf);
	pDuino->serial_println((int)tStart);
	pDuino->delay(1000);
	Ticks tEnd = pDuino->ticks();
	strcpy_P(buf, startup_delay);
	pDuino->serial_print(buf);
	pDuino->serial_println((int)tEnd);
	Ticks tElapsed = tEnd - tStart;
	if (tElapsed < MS_TICKS(1000)) {
		strcpy_P(buf, startup_error);
		pDuino->serial_println(buf);
		return false;
	}

	// Turn off pin #4 (SERVO1)
	strcpy_P(buf, startup_led_off);
	pDuino->serial_println(buf);
	pDuino->digitalWrite(PC1_SERVO1, LOW);

	// Sizes
	strcpy_P(buf, startup_size_mach);
	pDuino->serial_print(buf);
	int bytes = (int) sizeof(Machine);
	pDuino->serial_println(bytes);
	bytes = (int) sizeof(MachineThread);
	strcpy_P(buf, startup_size_mt);
	pDuino->serial_print(buf);
	pDuino->serial_println(bytes);

	strcpy_P(buf, startup_done);
	pDuino->serial_println(buf);

	return true;
}

void setup() { // run once, when the sketch starts
    // Serial I/O has lowest priority, so you may need to
    // decrease baud rate to fix Serial I/O problems.
    //Serial.begin(38400); // short USB cables
    Serial.begin(19200); // long USB cables

	mega2560.setup();
	startup_IDuino(&mega2560);

#ifdef MACHINE_ACTIVE
    machine.pDisplay = &neoPixel; // Inject NeoPixel driver
#endif
#ifdef MACHINETHREAD_ACTIVE
    machineThread.setup(PIN_CONFIG); // Set up FireStep pins
    threadRunner.setup(&mega2560, LED_PIN);
#endif
}

void loop() {	// run over and over again
#ifdef MACHINETHREAD_ACTIVE
    threadRunner.run();
#else
	mega2560.serial_print('.');
	delay(1000);
#endif
}


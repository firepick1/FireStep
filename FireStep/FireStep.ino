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
Machine machine(&mega2560);
MachineThread machineThread(machine); 
namespace firestep {int __heap_start, *__brkval;}

const char msg_startup[] PROGMEM = 			{ "Startup	: FireStep" };
const char msg_startup_println[] PROGMEM = 	{ "Startup	: mega2560.serial_println()" };
const char msg_startup_pinMode[] PROGMEM = 	{ "Startup	: mega2560.pinMode(4, OUTPUT)" };
const char msg_startup_led_on[] PROGMEM = 	{ "Startup	: mega2560.digitalWrite(4, HIGH)" };
const char msg_startup_ticks[] PROGMEM = 	{ "Startup	: mega2560.ticks()" };
const char msg_startup_delay[] PROGMEM = 	{ "Startup	: mega2560.delay(5000)" };
const char msg_startup_led_off[] PROGMEM = 	{ "Startup	: mega2560.digitalWrite(4, LOW)" };

void setup() { // run once, when the sketch starts
    // Serial I/O has lowest priority, so you may need to
    // decrease baud rate to fix Serial I/O problems.
    //Serial.begin(38400); // short USB cables
    Serial.begin(19200); // long USB cables

	char buf[100];
	strcpy_P(buf, msg_startup1);
	Serial.println(buf);
	strcpy_P(buf, msg_startup_ok);
	mega2560.serial_println(buf);
	strcpy_P(buf, msg_startup_pinMode);
	mega2560.serial_println(buf);
	mega2560.pinMode(PC1_SERVO1, OUTPUT);
	strcpy_P(buf, msg_startup_led_on);
	mega2560.serial_println(buf);
	mega2560.digitalWrite(PC1_SERVO1, HIGH);
	strcpy_P(buf, msg_startup_ticks);
	mega2560.serial_println(buf);
	mega2560.serial_println(mega2560.ticks());
	strcpy_P(buf, msg_startup_delay);
	mega2560.serial_println(buf);
	mega2560.delay(5000);
	strcpy_P(buf, msg_startup_led_off);
	mega2560.serial_println(buf);
	mega2560.digitalWrite(PC1_SERVO1, LOW);
	strcpy_P(buf, msg_startup_ticks);
	mega2560.serial_println(buf);
	mega2560.serial_println(mega2560.ticks());

    // Bind in NeoPixel display driver
    machineThread.machine.pDisplay = &neoPixel;

    // Initialize
    machineThread.setup(PIN_CONFIG);

    threadRunner.setup(&mega2560, LED_PIN);
}

void loop() {	// run over and over again
    //threadRunner.run();
}


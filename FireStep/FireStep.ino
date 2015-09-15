// Arduino Oddness...
// THe following MUST be included explicitly in FireStep.ino
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include <ph5.h>
#include "Mega2560.h"
#include "MachineThread.h"
#include "NeoPixel.h"
#include "ProgMem.h"

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

#define SELFTEST

void setup() { // run once, when the sketch starts
    // Serial I/O has lowest priority, so you may need to
    // decrease baud rate to fix Serial I/O problems.
    //Serial.begin(38400); // short USB cables
    Serial.begin(19200); // long USB cables

	mega2560.setup();

    machine.pDisplay = &neoPixel; // Inject NeoPixel driver
    machineThread.setup(PIN_CONFIG); // Set up FireStep pins
    threadRunner.setup(&mega2560, LED_PIN);

	delay(3000); // wait for user to open serial window
	machine.pDuino->serial.print("...");
#ifdef SELFTEST
	mega2560.selftest();
	for (ThreadPtr pThread = firestep::pThreadList; pThread; pThread = pThread->pNext) {
		char buf[100];
		machine.pDuino->PM_strcpy(buf, firestep::thread_list);
		machine.pDuino->serial_print(buf);
		machine.pDuino->serial_println(pThread->id);
	}
#endif
}

void loop() {	// run over and over again
#ifdef SELFTEST
	static int secs=0;
	mega2560.serial_print(' ');
	mega2560.serial_print(secs++);
	if (secs % 60 == 0) {
		mega2560.serial_println();
	}
	for (int i=0; i<10; i++) {
		//machineThread.machine.pDisplay->show();
		machineThread.displayStatus();
		delay(100);
	}
#else
    threadRunner.run();
#endif
}


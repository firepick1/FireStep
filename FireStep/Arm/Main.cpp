/* Main.cpp - Part of FireStep
 *
 * Copyright (C) 2015 Paul Jones
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include <cr_section_macros.h>

//#include <Adafruit_NeoPixel.h>
//#include "NeoPixel.h"
#include <ArduinoJson.h>
#include <ph5.h>
#include "fireduino.h"
#include "../MachineThread.h"
#include "usb_helper.h"

///////////////////// CHOOSE DEFAULT PIN CONFIGURATION ///////////
//#define PIN_CONFIG PC2_RAMPS_1_4
#define PIN_CONFIG PC1_EMC02

firestep::MachineThread machineThread; // FireStep command interpreter

/////////// NeoPixel display driver /////////////
//#define NEOPIXEL_LEDS 16
//firestep::NeoPixel neoPixel(NEOPIXEL_LEDS);
//
//#if PIN_CONFIG == PC1_EMC02
//#define LED_PIN PC1_LED_PIN
//#else
//#define LED_PIN PC2_LED_PIN
//#endif

HardwareSerial Debug;
UsbSerial Serial;

int main (void)
{
	// Read clock settings and update SystemCoreClock variable
	SystemCoreClockUpdate();

	// Enable and setup SysTick Timer for 1 ms
	us_elapsed = 0;
	SysTick_Config(SystemCoreClock / 1000);

	// Turn on GPIO and IOCON blocks
	Chip_GPIO_Init(LPC_GPIO);
	Chip_IOCON_Init(LPC_IOCON);

	// Disable buffering on stdout
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	// Initialize peripherals
	usb_init();
	Serial.begin(115200);
	Debug.begin(115200);
	fprintf(stderr, "Coretex M3 running at %ld MHz\r\n", (long) (SystemCoreClock / 1e6));

	// Load eeprom
	fprintf(stderr, "Opening eeprom file...");
	init_eeprom();
	fprintf(stderr, "Done!\r\n");

	// Bind in NeoPixel display driver
	//machineThread.machine.pDisplay = &neoPixel;

	// Initialize
	machineThread.setup(PIN_CONFIG);

	firestep::threadRunner.setup(LED_PIN);

	// Run over and over again
	firestep::threadRunner.run();

	return 0;
}

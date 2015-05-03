#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include "Arduino.h"
#include "MachineThread.h"
#include "NeoPixel.h"

firestep::MachineThread machineThread;
firestep::NeoPixel neoPixel;

void setup() // run once, when the sketch starts
{
	Serial.begin(115200); 

	machineThread.setup();
	machine.pDisplay = &neoPixel;
	firestep::threadRunner.setup(LED_PIN_RED, LED_PIN_GRN);
}

void loop()	// run over and over again
{
	firestep::threadRunner.run();
}


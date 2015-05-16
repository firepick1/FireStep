#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include "Arduino.h"
#include "MachineThread.h"
#include "NeoPixel.h"

firestep::MachineThread machineThread; // FireStep command interpreter
firestep::NeoPixel neoPixel(16, PC2_DISPLAY_PIN); // NeoPixel display driver

void setup() // run once, when the sketch starts
{
  // Serial I/O has lowest priority, so you may need to 
  // decrease baud rate to fix Serial I/O problems.
  Serial.begin(38400); 

  // Bind in NeoPixel display driver
  machineThread.machine.pDisplay = &neoPixel;	

  // Initialize
  machineThread.setup();

  firestep::threadRunner.setup(PC2_LED_PIN);
}

void loop()	// run over and over again
{
  firestep::threadRunner.run();
}


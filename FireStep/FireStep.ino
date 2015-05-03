#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include "Arduino.h"
#include "MachineThread.h"
#include "NeoPixel.h"

firestep::MachineThread machineThread;
firestep::NeoPixel neoPixel(16, DISPLAY_PIN);

void setup() // run once, when the sketch starts
{
  Serial.begin(38400); // decrease if Serial drops characters
  machineThread.setup();
  neoPixel.begin();
  machineThread.machine.pDisplay = &neoPixel;
  firestep::threadRunner.setup(LED_PIN_RED, LED_PIN_GRN);
}

void loop()	// run over and over again
{
  firestep::threadRunner.run();
}


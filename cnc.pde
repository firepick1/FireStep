#include "WProgram.h"
#include "SerialTypes.h"
#include "Machine.h"
#include "Thread.h"

MachineThread mainThread;

void setup()										// run once, when the sketch starts
{
	//Serial.begin(115200); // too fast
	Serial.begin(57600);

	mainThread.Setup();

	ThreadSetup(LED_PIN_RED, LED_PIN_GRN);
	monitor.verbose = false;
}

void loop()										 // run over and over again
{
	ThreadRunner();
}

byte lastByte;

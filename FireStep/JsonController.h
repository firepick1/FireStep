#ifndef JSONCONTROLLER_H
#define JSONCONTROLLER_H

#include "version.h"
#include "Arduino.h"
#include "Thread.h"
#include "Status.h"
#include "Machine.h"
#include "JsonCommand.h"

namespace firestep {

typedef class JsonController {	
	private:
		void processRawSteps(Quad<StepCoord> &steps);
		Ticks lastProcessed;
	protected:
		Machine &machine;
		Status processStepperPosition(JsonCommand &jcmd, JsonObject& jobj, const char* key);
		Status processMotor(JsonCommand &jcmd, JsonObject& jobj, const char* key, char group);
		Status processAxis(JsonCommand &jcmd, JsonObject& jobj, const char* key, char group);
		Status processStroke(JsonCommand &jcmd, JsonObject& jobj, const char* key);
		Status processSys(JsonCommand& jcmd, JsonObject& jobj, const char* key);
		Status processTest(JsonCommand& jcmd, JsonObject& jobj, const char* key);
		Status processDisplay(JsonCommand& jcmd, JsonObject& jobj, const char* key);
		Status processPin(JsonObject& jobj, const char *key, PinType &pin, int16_t mode, int16_t value=LOW);
		Status initializeStroke(JsonCommand &jcmd, JsonObject& stroke);
		Status traverseStroke(JsonCommand &jcmd, JsonObject &stroke);

    public:
        JsonController(Machine& machine);
	public: 
		Status setup();
		Status process(JsonCommand& jcmd);
		Status cancel(JsonCommand &jcmd, Status status);
		Ticks getLastProcessed() { return lastProcessed; }
} JsonController;

} // namespace firestep

#endif

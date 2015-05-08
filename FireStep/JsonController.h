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
		void processRawSteps(Machine& machine, Quad<StepCoord> &steps);
		Ticks lastProcessed;
	protected:
		Status processStepperPosition(Machine& machine, JsonCommand &jcmd, JsonObject& jobj, const char* key);
		Status processMotor(Machine& machine, JsonCommand &jcmd, JsonObject& jobj, const char* key, char group);
		Status processAxis(Machine& machine, JsonCommand &jcmd, JsonObject& jobj, const char* key, char group);
		Status processStroke(Machine& machine, JsonCommand &jcmd, JsonObject& jobj, const char* key);
		Status processSys(Machine& machine, JsonCommand& jcmd, JsonObject& jobj, const char* key);
		Status processTest(Machine& machine, JsonCommand& jcmd, JsonObject& jobj, const char* key);
		Status processDisplay(Machine& machine, JsonCommand& jcmd, JsonObject& jobj, const char* key);
		Status processPin(Machine &machine, JsonObject& jobj, const char *key, PinType &pin, int16_t mode, int16_t value=LOW);
		Status initializeStroke(Machine& machine, JsonCommand &jcmd, JsonObject& stroke);
		Status traverseStroke(Machine& machine, JsonCommand &jcmd, JsonObject &stroke);

    public:
        JsonController();
	public: 
		Status setup();
		Status process(Machine& machine, JsonCommand& jcmd);
		Status cancel(JsonCommand &jcmd, Status status);
		Ticks getLastProcessed() { return lastProcessed; }
} JsonController;

} // namespace firestep

#endif

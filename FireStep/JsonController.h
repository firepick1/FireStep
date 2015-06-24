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
        Status initializeStrokeArray(JsonCommand &jcmd, JsonObject& stroke,
                                     const char *key, MotorIndex iMotor, int16_t &slen);
        Status processRawSteps(Quad<StepCoord> &steps);
        void sendResponse(JsonCommand& jcmd);
    protected:
        Machine &machine;
        Status initializeStroke(JsonCommand &jcmd, JsonObject& stroke);
        Status initializeHome(JsonCommand& jcmd, JsonObject& jobj, const char* key, bool clear);
        Status processAxis(JsonCommand &jcmd, JsonObject& jobj, const char* key, char group);
        Status processDisplay(JsonCommand& jcmd, JsonObject& jobj, const char* key);
        Status processHome(JsonCommand& jcmd, JsonObject& jobj, const char* key);
		Status processIOPin(JsonCommand& jcmd, JsonObject& jobj, const char* key);
		Status processIO(JsonCommand& jcmd, JsonObject& jobj, const char* key);
        Status processMotor(JsonCommand &jcmd, JsonObject& jobj, const char* key, char group);
        Status processPin(JsonObject& jobj, const char *key, PinType &pin, int16_t mode, int16_t value = LOW);
        Status processStepperPosition(JsonCommand &jcmd, JsonObject& jobj, const char* key);
        Status processStroke(JsonCommand &jcmd, JsonObject& jobj, const char* key);
        Status processSys(JsonCommand& jcmd, JsonObject& jobj, const char* key);
        Status processTest(JsonCommand& jcmd, JsonObject& jobj, const char* key);
        Status traverseStroke(JsonCommand &jcmd, JsonObject &stroke);
        Status processObj(JsonCommand& jcmd, JsonObject&jobj);

    public:
        JsonController(Machine& machine);
    public:
        Status setup();
        Status process(JsonCommand& jcmd);
        Status cancel(JsonCommand &jcmd, Status cause);
} JsonController;

} // namespace firestep

#endif

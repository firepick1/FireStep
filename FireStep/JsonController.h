#ifndef JSONCONTROLLER_H
#define JSONCONTROLLER_H

#include "version.h"
#include "Arduino.h"
#include "Thread.h"
#include "Status.h"
#include "Machine.h"
#include "JsonCommand.h"
#include "DeltaCalculator.h"

namespace firestep {

typedef class JsonController {
private:
    Status initializeStrokeArray(JsonCommand &jcmd, JsonObject& stroke,
                                 const char *key, MotorIndex iMotor, int16_t &slen);
    Status processRawSteps(Quad<StepCoord> &steps);
protected:
    Machine &machine;

protected:
	int axisOf(char c);
	Status initializeStroke(JsonCommand &jcmd, JsonObject& stroke);
    Status initializeHome(JsonCommand& jcmd, JsonObject& jobj, const char* key, bool clear);
    Status initializeProbe(JsonCommand& jcmd, JsonObject& jobj, const char* key, bool clear);
    Status processAxis(JsonCommand &jcmd, JsonObject& jobj, const char* key, char group);
    Status processDisplay(JsonCommand& jcmd, JsonObject& jobj, const char* key);
    Status processHome(JsonCommand& jcmd, JsonObject& jobj, const char* key);
    Status processIOPin(JsonCommand& jcmd, JsonObject& jobj, const char* key);
    Status processIO(JsonCommand& jcmd, JsonObject& jobj, const char* key);
    Status processEEPROMValue(JsonCommand& jcmd, JsonObject& jobj,
                              const char* key, const char *addr);
    Status processEEPROM(JsonCommand& jcmd, JsonObject& jobj, const char* key);
    Status processMotor(JsonCommand &jcmd, JsonObject& jobj, const char* key, char group);
    Status processPin(JsonObject& jobj, const char *key,
                      PinType &pin, int16_t mode, int16_t value = LOW);

    Status processStroke(JsonCommand &jcmd, JsonObject& jobj, const char* key);
    Status processSys(JsonCommand& jcmd, JsonObject& jobj, const char* key);
    Status processTest(JsonCommand& jcmd, JsonObject& jobj, const char* key);
    Status traverseStroke(JsonCommand &jcmd, JsonObject &stroke);

public:
    JsonController(Machine& machine);
    void sendResponse(JsonCommand& jcmd, Status status);
    Status processObj(JsonCommand& jcmd, JsonObject&jobj);
	virtual const char *name();
    virtual Status processMove(JsonCommand& jcmd, JsonObject& jobj, const char* key);
    virtual Status processProbe(JsonCommand& jcmd, JsonObject& jobj, const char* key);
    virtual Status processPosition(JsonCommand &jcmd, JsonObject& jobj, const char* key);
    virtual Status processDimension(JsonCommand& jcmd, JsonObject& jobj, const char* key);
	JsonController& operator=(JsonController& that);
    Status cancel(JsonCommand &jcmd, Status cause);
} JsonController;


} // namespace firestep

#endif

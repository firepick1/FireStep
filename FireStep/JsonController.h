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
    virtual Status initializeStroke(JsonCommand &jcmd, JsonObject& stroke);
    virtual Status initializeHome(JsonCommand& jcmd, JsonObject& jobj, const char* key, bool clear)=0;
    virtual Status processAxis(JsonCommand &jcmd, JsonObject& jobj, const char* key, char group);
    virtual Status processDisplay(JsonCommand& jcmd, JsonObject& jobj, const char* key);
    virtual Status processIOPin(JsonCommand& jcmd, JsonObject& jobj, const char* key);
    virtual Status processIO(JsonCommand& jcmd, JsonObject& jobj, const char* key);
    virtual Status processEEPROMValue(JsonCommand& jcmd, JsonObject& jobj,
                              const char* key, const char *addr);
    virtual Status processEEPROM(JsonCommand& jcmd, JsonObject& jobj, const char* key);
    virtual Status processMotor(JsonCommand &jcmd, JsonObject& jobj, const char* key, char group);
    virtual Status processPin(JsonObject& jobj, const char *key,
                      PinType &pin, int16_t mode, int16_t value = LOW);

    virtual Status processStroke(JsonCommand &jcmd, JsonObject& jobj, const char* key);
    virtual Status processSys(JsonCommand& jcmd, JsonObject& jobj, const char* key);
    virtual Status processTest(JsonCommand& jcmd, JsonObject& jobj, const char* key);
    virtual Status traverseStroke(JsonCommand &jcmd, JsonObject &stroke);

public:
    JsonController(Machine& machine);
    void sendResponse(JsonCommand& jcmd, Status status);
    Status processObj(JsonCommand& jcmd, JsonObject&jobj);
    JsonController& operator=(JsonController& that);
    Status cancel(JsonCommand &jcmd, Status cause);

public:
    virtual const char *name() = 0;
    virtual void onTopologyChanged() {};
    virtual Status processCalibrate(JsonCommand& jcmd, JsonObject& jobj, const char* key);
    virtual Status processHome(JsonCommand& jcmd, JsonObject& jobj, const char* key) = 0;
    virtual Status processMove(JsonCommand& jcmd, JsonObject& jobj, const char* key) = 0;
    virtual Status processProbe(JsonCommand& jcmd, JsonObject& jobj, const char* key) = 0;
    virtual Status processPosition(JsonCommand &jcmd, JsonObject& jobj, const char* key)=0;
    virtual Status processDimension(JsonCommand& jcmd, JsonObject& jobj, const char* key) = 0;
} JsonController;


} // namespace firestep

#endif

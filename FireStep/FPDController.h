#ifndef FPDCONTROLLER_H
#define FPDCONTROLLER_H

#include "JsonController.h"

namespace firestep {

typedef class FPDCalibrateHome {
private:
    Status status;
    Machine& machine;
public:
    PH5TYPE zCenter;
    PH5TYPE zRim;
    PH5TYPE eTheta;
    PH5TYPE homeAngle;
    ZPlane bed;

public:
    FPDCalibrateHome(Machine& machine);
    Status calibrate();
    Status save();
} FPDCalibrateHome;

typedef class FPDController : public JsonController {
protected:
    Status initializeProbe_MTO_FPD(JsonCommand& jcmd, JsonObject& jobj, const char* key, bool clear);
    virtual Status initializeHome(JsonCommand& jcmd, JsonObject& jobj, const char* key, bool clear);
    Status finalizeProbe_MTO_FPD(JsonCommand& jcmd, JsonObject& jobj, const char* key);
    Status finalizeHome();
    Status processCalibrateCore(JsonCommand &jcmd, JsonObject& jobj, const char* key, FPDCalibrateHome &cal);
    virtual Status processMark(JsonCommand& jcmd, JsonObject& jobj, const char* key);
    virtual Status processCalibrate(JsonCommand &jcmd, JsonObject& jobj, const char* keycal);
    virtual Status processHome(JsonCommand& jcmd, JsonObject& jobj, const char* key);
    virtual Status processPosition(JsonCommand &jcmd, JsonObject& jobj, const char* key);
    virtual Status processProbe(JsonCommand& jcmd, JsonObject& jobj, const char* key);
    virtual Status processDimension(JsonCommand& jcmd, JsonObject& jobj, const char* key);
    virtual	Status processMove(JsonCommand& jcmd, JsonObject& jobj, const char* key);

public:
    FPDController(Machine& machine);
    XYZ3D getXYZ3D();
    virtual const char * name();
    virtual void onTopologyChanged();
} FPDController;

} // namespace firestep

#endif

#ifndef FPDCONTROLLER_H
#define FPDCONTROLLER_H

#include "JsonController.h"

namespace firestep {

enum CalibrateMode {
	CAL_NONE = 0,
	CAL_HOME = 0x1,
	CAL_BED = 0x2,
	CAL_GEAR = 0x10,
	CAL_GEAR1 = 0x20,
	CAL_GEAR2 = 0x40,
	CAL_GEAR3 = 0x80
};

typedef class FPDCalibrateHome {
private:
    Machine& machine;
public:
    PH5TYPE zCenter;
    PH5TYPE zRim;
    PH5TYPE eTheta;
    PH5TYPE homeAngle;
	PH5TYPE eGear;
	PH5TYPE gearRatio;
	PH5TYPE saveWeight;
    ZPlane bed;
	CalibrateMode mode;

public:
    FPDCalibrateHome(Machine& machine);
    Status calibrate();
} FPDCalibrateHome;

typedef class FPDController : public JsonController {
protected:
    Status initializeProbe_MTO_FPD(JsonCommand& jcmd, JsonObject& jobj, const char* key, bool clear);
    virtual Status initializeHome(JsonCommand& jcmd, JsonObject& jobj, const char* key, bool clear);
    Status finalizeProbe_MTO_FPD(JsonCommand& jcmd, JsonObject& jobj, const char* key);
    Status finalizeHome(JsonCommand& jcmd, JsonObject& jobj, const char * key);
    Status processCalibrateCore(JsonCommand &jcmd, JsonObject& jobj, const char* key, FPDCalibrateHome &cal, bool output);
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

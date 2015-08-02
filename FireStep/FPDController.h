#ifndef FPDCONTROLLER_H
#define FPDCONTROLLER_H

#include "JsonController.h"

namespace firestep {

typedef class FPDController : public JsonController {
protected:

    Status initializeProbe_MTO_FPD(JsonCommand& jcmd, JsonObject& jobj, const char* key, bool clear);
    Status finalizeProbe_MTO_FPD(JsonCommand& jcmd, JsonObject& jobj, const char* key);
	Status finalizeHome();

public:
    FPDController(Machine& machine);
    XYZ3D getXYZ3D();
    virtual const char * name();
	virtual void onTopologyChanged();
    virtual Status processHome(JsonCommand& jcmd, JsonObject& jobj, const char* key);
    virtual Status processPosition(JsonCommand &jcmd, JsonObject& jobj, const char* key);
    virtual Status processProbe(JsonCommand& jcmd, JsonObject& jobj, const char* key);
    virtual Status processDimension(JsonCommand& jcmd, JsonObject& jobj, const char* key);
    virtual	Status processMove(JsonCommand& jcmd, JsonObject& jobj, const char* key);
} FPDController;

} // namespace firestep

#endif

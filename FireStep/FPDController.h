#ifndef FPDCONTROLLER_H
#define FPDCONTROLLER_H

#include "JsonController.h"

namespace firestep {

typedef class FPDController : public JsonController {
protected:
    virtual Status processPosition(JsonCommand &jcmd, JsonObject& jobj, const char* key);
    virtual Status processProbe(JsonCommand& jcmd, JsonObject& jobj, const char* key);
    virtual Status processDimension(JsonCommand& jcmd, JsonObject& jobj, const char* key);
    virtual	Status processMove(JsonCommand& jcmd, JsonObject& jobj, const char* key);

    Status initializeProbe_MTO_FPD(JsonCommand& jcmd, JsonObject& jobj, const char* key, bool clear);
    Status finalizeProbe_MTO_FPD(JsonCommand& jcmd, JsonObject& jobj, const char* key);

public:
    FPDController(Machine& machine);
    const char * name();
} FPDController;

} // namespace firestep

#endif

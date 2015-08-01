#ifndef FPDCONTROLLER_H
#define FPDCONTROLLER_H

#include "JsonController.h"

namespace firestep {

typedef class FPDController : public JsonController {
protected:
    Status processPosition(JsonCommand &jcmd, JsonObject& jobj, const char* key);
    Status processProbe_MTO_FPD(JsonCommand& jcmd, JsonObject& jobj, const char* key);

    Status initializeProbe_MTO_FPD(JsonCommand& jcmd, JsonObject& jobj, const char* key, bool clear);
    Status finalizeProbe_MTO_FPD(JsonCommand& jcmd, JsonObject& jobj, const char* key);
    Status processDimension_MTO_FPD(JsonCommand& jcmd, JsonObject& jobj, const char* key);

public:
    FPDController(Machine& machine);
} FPDController;

} // namespace firestep

#endif

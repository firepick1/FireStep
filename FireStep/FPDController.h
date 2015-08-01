#ifndef FPDCONTROLLER_H
#define FPDCONTROLLER_H

#include "JsonController.h"

namespace firestep {

typedef class FPDController : public JsonController {
private:
    Status processRawSteps(Quad<StepCoord> &steps);

protected:
    Status processSys(JsonCommand& jcmd, JsonObject& jobj, const char* key);
    Status processObj(JsonCommand& jcmd, JsonObject&jobj);

    Status initializeProbe_MTO_FPD(JsonCommand& jcmd, JsonObject& jobj, const char* key, bool clear);
    Status processProbe_MTO_FPD(JsonCommand& jcmd, JsonObject& jobj, const char* key);
    Status processPosition_MTO_FPD(JsonCommand &jcmd, JsonObject& jobj, const char* key);
    Status finalizeProbe_MTO_FPD(JsonCommand& jcmd, JsonObject& jobj, const char* key);
    Status processDimension_MTO_FPD(JsonCommand& jcmd, JsonObject& jobj, const char* key);

public:
    FPDController(Machine& machine);
public:
    Status process(JsonCommand& jcmd);
} FPDController;

} // namespace firestep

#endif

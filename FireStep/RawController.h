#ifndef RAWCONTROLLER_H
#define RAWCONTROLLER_H

#include "JsonController.h"

namespace firestep {

typedef class RawController : public JsonController {
private:
    Status initializeProbe(JsonCommand& jcmd, JsonObject& jobj, const char* key, bool clear);
    virtual Status initializeHome(JsonCommand& jcmd, JsonObject& jobj, const char* key, bool clear);
protected:
public:
    RawController(Machine& machine);
    virtual const char *name();
    virtual Status processHome(JsonCommand& jcmd, JsonObject& jobj, const char* key);
    virtual Status processMove(JsonCommand& jcmd, JsonObject& jobj, const char* key);
    virtual Status processProbe(JsonCommand& jcmd, JsonObject& jobj, const char* key);
    virtual Status processPosition(JsonCommand &jcmd, JsonObject& jobj, const char* key);
    virtual Status processDimension(JsonCommand& jcmd, JsonObject& jobj, const char* key);
} RawController;


} // namespace firestep

#endif

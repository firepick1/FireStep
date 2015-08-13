#ifndef JSONCOMMAND_H
#define JSONCOMMAND_H

#include "version.h"
#include "Arduino.h"
#include "Thread.h"
#ifdef TEST
#include "ArduinoJson.h"
#else
#include <ArduinoJson.h>
#endif
#include "Status.h"
#include "Machine.h"

namespace firestep {

#define MAX_JSON (STROKE_SEGMENTS*2*4 + 200)
//#if defined(TEST)
//#define JSON_REQUEST_BUFFER JSON_OBJECT_SIZE(25)
//#else
#define JSON_REQUEST_BUFFER JSON_OBJECT_SIZE(150)
//#endif
#define JSON_RESPONSE_BUFFER 200

typedef class JsonCommand {
    friend class JsonController;
    friend class FPDController;
private:
    bool parsed;
    char json[MAX_JSON];
    char *pJsonFree;
    StaticJsonBuffer<JSON_REQUEST_BUFFER> jbRequest;
    StaticJsonBuffer<JSON_RESPONSE_BUFFER> jbResponse;
    Quad<StepCoord> move;
    StepCoord stepRate; // steps per second
    JsonVariant jRequestRoot;
    Ticks tStart;
    char error[8];

private:
    Status parseCore();
    Status parseInput(const char *jsonIn, Status status);
public:
    JsonCommand();
    JsonVariant jResponseRoot;
    int8_t cmdIndex;
    void clear();
    inline JsonVariant& requestRoot() {
        return jRequestRoot;
    }
    inline JsonObject & response() {
        return jResponseRoot;
    }
    Status parse(const char *jsonIn, Status status=STATUS_WAIT_IDLE);
    bool isValid();
    inline Status getStatus() {
        return (Status) (int32_t) jResponseRoot["s"];
    }
    inline void setTicks() {
        jResponseRoot["t"] = (ticks() - tStart) / (float) TICKS_PER_SECOND;
    }
    inline void setStatus(Status status) {
        jResponseRoot["s"] = status;
    }
    const char *getError();
    Status setError(Status status, const char *err);
    size_t requestAvailable();
    size_t responseAvailable();
    size_t requestCapacity();
    size_t responseCapacity();
    void responseClear();
    char * allocate(size_t length);
} JsonCommand;

} // namespace firestep

#endif

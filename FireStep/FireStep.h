#ifndef FIRESTEP_H
#define FIRESTEP_H

#ifdef FIRESTEP_IMPL
#include "firestep/AnalogRead.h"
#include "firestep/DeltaCalculator.h"
#include "firestep/Display.h"
#include "firestep/FPDController.h"
#include "firestep/FireUtils.h"
#include "firestep/JsonCommand.h"
#include "firestep/JsonController.h"
#include "firestep/MCU.h"
#include "firestep/Machine.h"
#include "firestep/MachineThread.h"
#include "firestep/NeoPixel.h"
#include "firestep/ProcessField.h"
#include "firestep/ProgMem.h"
#include "firestep/Quad.h"
#include "firestep/RawController.h"
#include "firestep/Status.h"
#include "firestep/Stroke.h"
#include "firestep/Thread.h"
#include "firestep/build.h"
#include "firestep/pins.h"
#include "firestep/version.h"
#else
#include "Status.h"
#endif

namespace firestep {

/**
 * Abstract base class and interface for a FireStep implementation
 */
typedef class IFireStep {
protected:
    bool ready;
protected:
    virtual int errorResponse(int rc, std::string &jsonResponse,float seconds=0) {
        char buf[255];
        snprintf(buf, sizeof(buf), "{\"s\":%d,\"t\":%.3f}", rc, seconds);
        jsonResponse = std::string(buf);
        return rc;
    }

protected:
    virtual int executeCore(const std::string &jsonRequest, std::string &jsonResponse) {
        return errorResponse(STATUS_NOT_IMPLEMENTED, jsonResponse);
    }

public: // construction
    IFireStep() : ready(false) {};
    virtual ~IFireStep() {};

public: // use
    bool isReady() {
        return ready;
    }

public: 
    virtual int reset() {
        return STATUS_NOT_IMPLEMENTED;
    }

public: 
    virtual int open() {
        ready = true;
        return 0;
    }

public: 
    virtual int execute(std::string jsonRequest, std::string &jsonResponse) {
        if (ready) {
            return errorResponse(STATUS_OPEN, jsonResponse);
        }
        return executeCore(jsonRequest, jsonResponse);
    }

public: 
    virtual int close() {
        ready = false;
        return 0;
    }

} IFireStep;

}

#endif

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
private:
    bool opened;
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
    IFireStep() : opened(false) {};
    virtual ~IFireStep() {};

public: // use
    bool isOpen() {
        return opened;
    }

public: 
    virtual int reset() {
        return STATUS_NOT_IMPLEMENTED;
    }

public: 
    virtual int open() {
        opened = true;
        return 0;
    }

public: 
	/**
	 * Process FireStep request, and return response.
	 * Return 0 if response was generated .
	 * Return STATUS_TIMEOUT if request was sent but no response received
	 * Return STATUS_OPEN if open() must be called.
	 * Return STATUS_REQUEST_LF if request does not end with LF.
	 * Return other error codes as per implementation to indicate
	 * an inability to respond. 
	 *
	 * Implementations should override executeCore()
	 */
    int execute(std::string request, std::string &response) {
        if (!isOpen()) {
			LOGERROR("IFireStep::execute() expected prior call to open()");
            return errorResponse(STATUS_OPEN, response);
        }
		if (request.size() == 0 || request[request.size()-1] != '\n') {
			LOGERROR("IFireStep::execute() request must end with LF");
			return STATUS_REQUEST_LF;
		}
		LOGINFO2("IFireStep::execute() bytes:%ld write:%s", (long) request.size(), request.c_str());
		response = ""; // clear
        int rc = executeCore(request, response);
		if (rc == 0) {
			if (response.size() > 0) {
				LOGINFO2("IFireStep::execute() bytes:%ld read:%s", (long) response.size(), response.c_str());
			} else {
				LOGERROR("IFireStep::execute() timeout");
				std::cerr << "IFireSTep::execute() timeout" << std::endl;
				rc = STATUS_TIMEOUT;
			}
		}
		return rc;
    }

public: 
    virtual int close() {
        opened = false;
        return 0;
    }

} IFireStep;

}

#endif

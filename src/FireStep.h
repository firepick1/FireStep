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

typedef class IFireStep {
protected:
	bool ready;
protected:
	virtual std::string osError(int rc, float seconds=0, int errBase=STATUS_LINUX) {
		char buf[255];
		snprintf(buf, sizeof(buf), "{\"s\":%d,\"r\":{},\"t\":%.3f}", 
			errBase-(rc<0?-rc:rc), seconds);
		return std::string(buf);
	}
public:
	virtual std::string executeCore(std::string &json) {
		return osError(-38); // ENOSYS: not implemented
	}
public: // creation
    IFireStep() : ready(false) {};
    virtual ~IFireStep() {};
public: // api
	bool isReady() { return ready; };
	virtual int startup() { 
		ready = true; 
		return 0;
	}
    std::string execute(std::string json) {
		if (ready) {
			return osError(-71); // EPROTO: must call startup() before execute()
		} 
		return executeCore(json);
	}
	virtual int shutdown() { 	
		ready = false; 
		return 0;
	}
} IFireStep;

}

#endif

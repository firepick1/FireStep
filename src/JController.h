#ifndef JCONTROLLER_H
#define JCONTROLLER_H

#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include "FireLog.h"
#include "FireUtils.hpp"
#include "version.h"
#include "Arduino.h"
#include "SerialTypes.h"
#include "Thread.h"
#include "ArduinoJson.h"
#include "JCommand.h"
#include "Machine.h"

namespace firestep {

typedef class JController {
    private:
		Machine& machine;

	protected:
		Status processMachinePosition(JCommand &jcmd, JsonObject& jobj, const char* key);
		Status processMotor(JCommand &jcmd, JsonObject& jobj, const char* key, char group);
		Status processAxis(JCommand &jcmd, JsonObject& jobj, const char* key, char group);

    public:
        JController(Machine &machine);
	public: 
		void process(JCommand& jcmd);
} JController;

} // namespace firestep

#endif

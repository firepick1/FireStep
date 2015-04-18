#include "build.h"
#include "version.h"
#include "JController.h"

using namespace firestep;

JController::JController(Machine &machine) :
	machine(machine) {
}

Status JController::processAxis(JCommand &jcmd, JsonObject& jobj, const char* key, char group) {
	Status status = STATUS_COMPLETED;
	const char *s;
	int iAxis;
	switch (group) {
		default:
		case 'x':
			iAxis = 0;
			break;
		case 'y':
			iAxis = 1;
			break;
		case 'z':
			iAxis = 2;
			break;
	}
	if (strcmp("tm",key)==0 || strcmp("tm",key+1)==0) {
		if ((s=jobj[key]) && *s==0) { // query
			status = (jobj[key] = (double) machine.axis[iAxis].travelMax).success() ? status : STATUS_ERROR;
		} else {
			machine.axis[iAxis].travelMax = (float)(double) jobj["tm"];
		}
	}
	return status;
}

void JController::process(JCommand& jcmd) {
	const char *s;
	JsonObject& root = jcmd.root();
	JsonVariant node;
	node = root;
	Status status = STATUS_COMPLETED;

	int i = 0;
	for (JsonObject::iterator it=root.begin(); it!=root.end(); ++it, i++) {
		if (strcmp("sys", it->key) == 0) {
			if ((s=it->value) && *s==0) {
				node = root["sys"] = jcmd.createJsonObject();
				node["fb"] = BUILD;
				node["fv"] = VERSION_MAJOR*100 + VERSION_MINOR + VERSION_PATCH/100.0;
				status = node.success() ? STATUS_COMPLETED : STATUS_ERROR;
			}
		} else {
			switch (it->key[0]) {
			case 'x':
			case 'y':
			case 'z':
				status = processAxis(jcmd, root, it->key, it->key[0]);
				break;
			}
		}
	}

	jcmd.setStatus(status);
}


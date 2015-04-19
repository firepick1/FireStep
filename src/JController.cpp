#include "build.h"
#include "version.h"
#include "JController.h"

using namespace firestep;

JController::JController(Machine &machine) :
	machine(machine) {
}

template<class TF, class TJ>
Status processField(JsonObject& jobj, const char* key, TF& field) {
	Status status = STATUS_COMPLETED;
	const char *s;
	if ((s=jobj[key]) && *s==0) { // query
		status = (jobj[key] = (TJ) field).success() ? status : STATUS_ERROR;
	} else {
		field = (TF)(TJ)jobj[key];
	}
	return status;
}

template
Status processField<int16_t,long>(JsonObject& jobj, const char *key, int16_t& field);

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
		case 'a':
			iAxis = 3;
			break;
		case 'b':
			iAxis = 4;
			break;
		case 'c':
			iAxis = 5;
			break;
	}
	if (strlen(key) == 1) {
		if ((s=jobj[key]) && *s==0) {
			JsonObject& node = jcmd.createJsonObject();
			jobj[key] = node;
			node["am"] = "";
			node["tn"] = "";
			node["tm"] = "";
		}
		JsonObject& kidObj = jobj[key];
		if (kidObj.success()) {
			for (JsonObject::iterator it=kidObj.begin(); it!=kidObj.end(); ++it) {
				status = processAxis(jcmd, kidObj, it->key, group);
				if (status != STATUS_COMPLETED) {
					return status;
				}
			}
		} 
	} else if (strcmp("am",key)==0 || strcmp("am",key+1)==0) {
		status = processField<uint8_t,long>(jobj, key, machine.axis[iAxis].mode);
	} else if (strcmp("tn",key)==0 || strcmp("tn",key+1)==0) {
		status = processField<int16_t,long>(jobj, key, machine.axis[iAxis].travelMin);
	} else if (strcmp("tm",key)==0 || strcmp("tm",key+1)==0) {
		status = processField<int16_t,long>(jobj, key, machine.axis[iAxis].travelMax);
	}
	return status;
}

void JController::process(JCommand& jcmd) {
	const char *s;
	JsonObject& root = jcmd.root();
	JsonVariant node;
	node = root;
	Status status = STATUS_COMPLETED;

	for (JsonObject::iterator it=root.begin(); it!=root.end(); ++it) {
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
			case 'a':
			case 'b':
			case 'c':
				status = processAxis(jcmd, root, it->key, it->key[0]);
				break;
			default:
				status = STATUS_UNRECOGNIZED_NAME;
				break;
			}
		}
	}

	jcmd.setStatus(status);
}


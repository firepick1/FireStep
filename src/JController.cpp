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
template Status processField<int16_t,long>(JsonObject& jobj, const char *key, int16_t& field);
template Status processField<uint8_t,long>(JsonObject& jobj, const char *key, uint8_t& field);
template Status processField<float,double>(JsonObject& jobj, const char *key, float& field);

int axisOf(char c) {
	switch (c) {
		default:
			return -1;
		case 'x':
			return 0;
		case 'y':
			return 1;
		case 'z':
			return 2;
		case 'a':
			return 3;
		case 'b':
			return 4;
		case 'c':
			return 5;
	}
}

Status JController::processMachinePosition(JCommand &jcmd, JsonObject& jobj, const char* key) {
	Status status = STATUS_COMPLETED;
	const char *s;
	if (strlen(key) == 3) {
		if ((s=jobj[key]) && *s==0) {
			JsonObject& node = jcmd.createJsonObject();
			jobj[key] = node;
			node["x"] = "";
			node["y"] = "";
			node["z"] = "";
			node["a"] = "";
			node["b"] = "";
			node["c"] = "";
		}
		JsonObject& kidObj = jobj[key];
		if (!kidObj.success()) {
			return STATUS_ERROR;
		}
		for (JsonObject::iterator it=kidObj.begin(); it!=kidObj.end(); ++it) {
			status = processMachinePosition(jcmd, kidObj, it->key);
			if (status != STATUS_COMPLETED) {
				return status;
			}
		} 
	} else if (strcmp("x",key)==0 || strcmp("mpox",key)==0) {
		status = processField<POSITION_TYPE,long>(jobj, key, machine.machinePosition.x);
	} else if (strcmp("y",key)==0 || strcmp("mpoy",key)==0) {
		status = processField<POSITION_TYPE,long>(jobj, key, machine.machinePosition.y);
	} else if (strcmp("z",key)==0 || strcmp("mpoz",key)==0) {
		status = processField<POSITION_TYPE,long>(jobj, key, machine.machinePosition.z);
	} else if (strcmp("a",key)==0 || strcmp("mpoa",key)==0) {
		status = processField<POSITION_TYPE,long>(jobj, key, machine.machinePosition.a);
	} else if (strcmp("b",key)==0 || strcmp("mpob",key)==0) {
		status = processField<POSITION_TYPE,long>(jobj, key, machine.machinePosition.b);
	} else if (strcmp("c",key)==0 || strcmp("mpoc",key)==0) {
		status = processField<POSITION_TYPE,long>(jobj, key, machine.machinePosition.c);
	}
	return status;
}

Status JController::processMotor(JCommand &jcmd, JsonObject& jobj, const char* key, char group) {
	Status status = STATUS_COMPLETED;
	const char *s;
	int iMotor = group - '1';
	ASSERT(0 <= iMotor);
	ASSERT(iMotor < MOTOR_COUNT);
	if (strlen(key) == 1) {
		if ((s=jobj[key]) && *s==0) {
			JsonObject& node = jcmd.createJsonObject();
			jobj[key] = node;
			node["ma"] = "";
			node["sa"] = "";
			node["mi"] = "";
			node["po"] = "";
			node["pm"] = "";
		}
		JsonObject& kidObj = jobj[key];
		if (kidObj.success()) {
			for (JsonObject::iterator it=kidObj.begin(); it!=kidObj.end(); ++it) {
				status = processMotor(jcmd, kidObj, it->key, group);
				if (status != STATUS_COMPLETED) {
					return status;
				}
			}
		} 
	} else if (strcmp("ma",key)==0 || strcmp("ma",key+1)==0) {
		status = processField<uint8_t,long>(jobj, key, machine.motor[iMotor].axisMap);
	} else if (strcmp("sa",key)==0 || strcmp("sa",key+1)==0) {
		status = processField<float,double>(jobj, key, machine.motor[iMotor].stepAngle);
	} else if (strcmp("mi",key)==0 || strcmp("mi",key+1)==0) {
		status = processField<uint8_t,long>(jobj, key, machine.motor[iMotor].microsteps);
	} else if (strcmp("po",key)==0 || strcmp("po",key+1)==0) {
		status = processField<uint8_t,long>(jobj, key, machine.motor[iMotor].polarity);
	} else if (strcmp("pm",key)==0 || strcmp("pm",key+1)==0) {
		status = processField<uint8_t,long>(jobj, key, machine.motor[iMotor].powerManagementMode);
	}
	return status;
}

Status JController::processAxis(JCommand &jcmd, JsonObject& jobj, const char* key, char group) {
	Status status = STATUS_COMPLETED;
	const char *s;
	int iAxis = axisOf(group);
	if (iAxis < 0) {
		return STATUS_ERROR;
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
		} else if (strncmp("mpo", it->key, 3) == 0) {
			status = processMachinePosition(jcmd, root, it->key);
		} else {
			switch (it->key[0]) {
			case '1':
			case '2':
			case '3':
			case '4':
				status = processMotor(jcmd, root, it->key, it->key[0]);
				break;
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


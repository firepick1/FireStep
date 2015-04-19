#include "Arduino.h"
#include <cstring>
#include <cstdio>
#include "build.h"
#include "version.h"
#include "JsonController.h"

using namespace firestep;


/////////////////////// JsonCommand ////////////

JsonCommand::JsonCommand() {
    parsed = false;
    memset(json, 0, sizeof(json));
    memset(error, 0, sizeof(error));
    pJsonFree = json;
	jsonResp = jsonBuffer.createObject();
	jsonResp["s"] = STATUS_EMPTY;
}

const char * JsonCommand::getError() {
	return error;
}

void JsonCommand::setError(const char *err) {
	snprintf(error, sizeof(error), "%s", err);
	jsonResp["e"] = error;
}

bool JsonCommand::parseCore() {
    jsonRoot = jsonBuffer.parseObject(json);
    if (jsonRoot.success()) {
        jsonResp["s"] = STATUS_JSON_PARSED;
		jsonResp["r"] = jsonRoot;
    } else {
        jsonResp["s"] = STATUS_JSON_SYNTAX_ERROR;
    }
	parsed = true;

    return true;
}

/**
 * Parse the JSON provided or a JSON line read from Serial
 * Return true if parsing is complete.
 * Check isValid() and getStatus() for parsing status.
 */
bool JsonCommand::parse(const char *jsonIn) {
    if (parsed) {
        return true;
    }
    if (jsonIn) {
        snprintf(json, sizeof(json), "%s", jsonIn);
        if (strcmp(jsonIn, json) != 0) {
            parsed = true;
            jsonResp["s"] = STATUS_JSON_TOO_LONG;
            return true;
        }
        return parseCore();
    }
    while (Serial.available()) {
        if (pJsonFree - json >= MAX_JSON - 1) {
            parsed = true;
            jsonResp["s"] = STATUS_JSON_TOO_LONG;
            return true;
        }
        char c = Serial.read();
        if (c == '\n') {
            return parseCore();
        } else {
            *pJsonFree++ = c;
        }
    }
    return false;
}

bool JsonCommand::isValid() {
    return parsed && jsonRoot.success();
}

//////////////////// JsonController ///////////////

JsonController::JsonController(Machine &machine) :
	machine(machine) {
}

template<class TF, class TJ>
Status processField(JsonObject& jobj, const char* key, TF& field) {
	Status status = STATUS_OK;
	const char *s;
	if ((s=jobj[key]) && *s==0) { // query
		status = (jobj[key] = (TJ) field).success() ? status : STATUS_FIELD_ERROR;
	} else {
		field = (TF)(TJ)jobj[key];
		jobj[key] = (TJ) field;
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

Status JsonController::processMachinePosition(JsonCommand &jcmd, JsonObject& jobj, const char* key) {
	Status status = STATUS_OK;
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
			return STATUS_POSITION_ERROR;
		}
		for (JsonObject::iterator it=kidObj.begin(); it!=kidObj.end(); ++it) {
			status = processMachinePosition(jcmd, kidObj, it->key);
			if (status != STATUS_OK) {
				return status;
			}
		} 
	} else if (strcmp("x",key)==0 || strcmp("spox",key)==0) {
		status = processField<POSITION_TYPE,long>(jobj, key, machine.machinePosition.x);
	} else if (strcmp("y",key)==0 || strcmp("spoy",key)==0) {
		status = processField<POSITION_TYPE,long>(jobj, key, machine.machinePosition.y);
	} else if (strcmp("z",key)==0 || strcmp("spoz",key)==0) {
		status = processField<POSITION_TYPE,long>(jobj, key, machine.machinePosition.z);
	} else if (strcmp("a",key)==0 || strcmp("spoa",key)==0) {
		status = processField<POSITION_TYPE,long>(jobj, key, machine.machinePosition.a);
	} else if (strcmp("b",key)==0 || strcmp("spob",key)==0) {
		status = processField<POSITION_TYPE,long>(jobj, key, machine.machinePosition.b);
	} else if (strcmp("c",key)==0 || strcmp("spoc",key)==0) {
		status = processField<POSITION_TYPE,long>(jobj, key, machine.machinePosition.c);
	}
	return status;
}

Status JsonController::processStroke(JsonCommand &jcmd, JsonObject& jobj, const char* key) {
    Status status = jcmd.getStatus();
	JsonObject &stroke = jobj[key];
	if (!stroke.success()) {
		return STATUS_JSON_STROKE_ERROR;
	}

    if (status == STATUS_JSON_PARSED) {
        int s1len = 0;
        int s2len = 0;
        int s3len = 0;
        int s4len = 0;
        for (JsonObject::iterator it = stroke.begin(); it != stroke.end(); ++it) {
            if (strcmp("s1", it->key) == 0) {
                JsonArray &jarr = stroke[it->key];
                if (jarr.success()) {
                    for (JsonArray::iterator it = jarr.begin(); it != jarr.end(); ++it) {
                        if (*it < -127 || 127 < *it) {
                            return STATUS_S1_RANGE_ERROR;
                        }
                        machine.stroke.seg[s1len++].dv[0] = (int8_t) (long) * it;
                    }
                }
            } else if (strcmp("s2", it->key) == 0) {
                JsonArray &jarr = stroke[it->key];
                if (jarr.success()) {
                    for (JsonArray::iterator it = jarr.begin(); it != jarr.end(); ++it) {
                        if (*it < -127 || 127 < *it) {
                            return STATUS_S2_RANGE_ERROR;
                        }
                        machine.stroke.seg[s2len++].dv[1] = (int8_t) (long) * it;
                    }
                }
            } else if (strcmp("s3", it->key) == 0) {
                JsonArray &jarr = stroke[it->key];
                if (jarr.success()) {
                    for (JsonArray::iterator it = jarr.begin(); it != jarr.end(); ++it) {
                        if (*it < -127 || 127 < *it) {
                            return STATUS_S3_RANGE_ERROR;
                        }
                        machine.stroke.seg[s3len++].dv[2] = (int8_t) (long) * it;
                    }
                }
            } else if (strcmp("s4", it->key) == 0) {
                JsonArray &jarr = stroke[it->key];
                if (jarr.success()) {
                    for (JsonArray::iterator it = jarr.begin(); it != jarr.end(); ++it) {
                        if (*it < -127 || 127 < *it) {
                            return STATUS_S4_RANGE_ERROR;
                        }
                        machine.stroke.seg[s4len++].dv[3] = (int8_t) (long) * it;
                    }
                }
            }
        }
        if (s1len && s2len && s1len != s2len) {
            return STATUS_S1S2LEN_ERROR;
        }
        if (s1len && s3len && s1len != s3len) {
            return STATUS_S1S3LEN_ERROR;
        }
        if (s1len && s4len && s1len != s4len) {
            return STATUS_S1S4LEN_ERROR;
        }
        machine.stroke.length = s1len ? s1len : (s2len ? s2len : (s3len ? s3len : s4len));
        machine.stroke.curSeg = 0;
        if (machine.stroke.length == 0) {
            return STATUS_STROKE_NULL_ERROR;
        }
        status = STATUS_PROCESSING;
    } else if (status == STATUS_PROCESSING) {
        if (machine.stroke.curSeg < machine.stroke.length) {
			StrokeSegment &seg = machine.stroke.seg[machine.stroke.curSeg];
			stroke["s1"] = seg.dv[0];
			stroke["s2"] = seg.dv[1];
			stroke["s3"] = seg.dv[2];
			stroke["s4"] = seg.dv[3];
            machine.stroke.curSeg++;
        }
        if (machine.stroke.curSeg >= machine.stroke.length) {
            status = STATUS_OK;
        }
    }
    return status;
}

Status JsonController::processMotor(JsonCommand &jcmd, JsonObject& jobj, const char* key, char group) {
    Status status = STATUS_OK;
    const char *s;
    int iMotor = group - '1';
    ASSERT(0 <= iMotor);
    ASSERT(iMotor < MOTOR_COUNT);
    if (strlen(key) == 1) {
        if ((s = jobj[key]) && *s == 0) {
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
            for (JsonObject::iterator it = kidObj.begin(); it != kidObj.end(); ++it) {
                status = processMotor(jcmd, kidObj, it->key, group);
                if (status != STATUS_OK) {
                    return status;
                }
            }
        }
    } else if (strcmp("ma", key) == 0 || strcmp("ma", key + 1) == 0) {
        status = processField<uint8_t, long>(jobj, key, machine.motor[iMotor].axisMap);
    } else if (strcmp("sa", key) == 0 || strcmp("sa", key + 1) == 0) {
        status = processField<float, double>(jobj, key, machine.motor[iMotor].stepAngle);
    } else if (strcmp("mi", key) == 0 || strcmp("mi", key + 1) == 0) {
        status = processField<uint8_t, long>(jobj, key, machine.motor[iMotor].microsteps);
    } else if (strcmp("po", key) == 0 || strcmp("po", key + 1) == 0) {
        status = processField<uint8_t, long>(jobj, key, machine.motor[iMotor].polarity);
    } else if (strcmp("pm", key) == 0 || strcmp("pm", key + 1) == 0) {
        status = processField<uint8_t, long>(jobj, key, machine.motor[iMotor].powerManagementMode);
    }
    return status;
}

Status JsonController::processAxis(JsonCommand &jcmd, JsonObject& jobj, const char* key, char group) {
    Status status = STATUS_OK;
    const char *s;
    int iAxis = axisOf(group);
    if (iAxis < 0) {
        return STATUS_AXIS_ERROR;
    }
    if (strlen(key) == 1) {
        if ((s = jobj[key]) && *s == 0) {
            JsonObject& node = jcmd.createJsonObject();
            jobj[key] = node;
            node["am"] = "";
            node["tn"] = "";
            node["tm"] = "";
        }
        JsonObject& kidObj = jobj[key];
        if (kidObj.success()) {
            for (JsonObject::iterator it = kidObj.begin(); it != kidObj.end(); ++it) {
                status = processAxis(jcmd, kidObj, it->key, group);
                if (status != STATUS_OK) {
                    return status;
                }
            }
        }
    } else if (strcmp("am", key) == 0 || strcmp("am", key + 1) == 0) {
        status = processField<uint8_t, long>(jobj, key, machine.axis[iAxis].mode);
    } else if (strcmp("tn", key) == 0 || strcmp("tn", key + 1) == 0) {
        status = processField<int16_t, long>(jobj, key, machine.axis[iAxis].travelMin);
    } else if (strcmp("tm", key) == 0 || strcmp("tm", key + 1) == 0) {
        status = processField<int16_t, long>(jobj, key, machine.axis[iAxis].travelMax);
    }
    return status;
}

void JsonController::process(JsonCommand& jcmd) {
    const char *s;
    JsonObject& root = jcmd.root();
    JsonVariant node;
    node = root;
    Status status = STATUS_OK;

    for (JsonObject::iterator it = root.begin(); it != root.end(); ++it) {
        if (strcmp("str", it->key) == 0) {
            status = processStroke(jcmd, root, it->key);
        } else if (strcmp("sys", it->key) == 0) {
            if ((s = it->value) && *s == 0) {
                node = root["sys"] = jcmd.createJsonObject();
                node["fb"] = BUILD;
                node["fv"] = VERSION_MAJOR * 100 + VERSION_MINOR + VERSION_PATCH / 100.0;
                status = node.success() ? STATUS_OK : STATUS_SYS_ERROR;
            }
        } else if (strncmp("spo", it->key, 3) == 0) {
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
                jcmd.setError(it->key);
                status = STATUS_UNRECOGNIZED_NAME;
                break;
            }
        }
    }

    jcmd.setStatus(status);
}


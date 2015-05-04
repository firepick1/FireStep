#include "Arduino.h"
#ifdef CMAKE
#include <cstring>
#include <cstdio>
#endif
#include "version.h"
#include "JsonController.h"

using namespace firestep;


/////////////////////// JsonCommand ////////////

JsonCommand::JsonCommand() {
	clear();
}

size_t JsonCommand::responseAvailable() {
	return jbResponse.capacity() - jbResponse.size();
}

size_t JsonCommand::requestAvailable() {
	return jbRequest.capacity() - jbRequest.size();
}

void JsonCommand::clear() {
    parsed = false;
    memset(json, 0, sizeof(json));
    memset(error, 0, sizeof(error));
    pJsonFree = json;
	jbRequest.clear();
	jbResponse.clear();
	jResponseRoot = jbResponse.createObject();
	jResponseRoot["s"] = STATUS_EMPTY;
	jResponseRoot.asObject().createNestedObject("r");
}

const char * JsonCommand::getError() {
	return error;
}

Status JsonCommand::setError(Status status, const char *err) {
	snprintf(error, sizeof(error), "%s", err);
	jResponseRoot["s"] = status;
	jResponseRoot["e"] = error;
	return status;
}

Status JsonCommand::parseCore() {
	JsonObject &jobj = jbRequest.parseObject(json);
	parsed = true;
    jRequestRoot = jobj;
    if (jobj.success()) {
		int kids = 0;
		for (JsonObject::iterator it=jobj.begin(); it!=jobj.end(); ++it) {
			kids++;
		}
		if (kids < 1) {
			return setError(STATUS_JSON_MEM, "mem");
		}
        jResponseRoot["s"] = STATUS_BUSY_PARSED;
		jResponseRoot["r"] = jRequestRoot;
    } else {
		return setError(STATUS_JSON_PARSE_ERROR, "json");
    }

	return STATUS_BUSY_PARSED;
}

/**
 * Parse the JSON provided or a JSON line read from Serial
 * Return true if parsing is complete.
 * Check isValid() and getStatus() for parsing status.
 */
Status JsonCommand::parse(const char *jsonIn) {
    if (parsed) {
        return STATUS_BUSY_PARSED;
    }
    if (jsonIn) {
        snprintf(json, sizeof(json), "%s", jsonIn);
        if (strcmp(jsonIn, json) != 0) {
            parsed = true;
			return setError(STATUS_JSON_TOO_LONG,"p1");
        }
        return parseCore();
	} else {
		while (Serial.available()) {
			if (pJsonFree - json >= MAX_JSON - 1) {
				parsed = true;
				return setError(STATUS_JSON_TOO_LONG,"p2");
			}
			char c = Serial.read();
			if (c == '\n') {
				return parseCore();
			} else {
				*pJsonFree++ = c;
			}
		}
		return STATUS_WAIT_EOL;
	}
}

bool JsonCommand::isValid() {
    return parsed && jRequestRoot.success();
}

//////////////////// JsonController ///////////////

JsonController::JsonController() {}

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
template Status processField<uint16_t,long>(JsonObject& jobj, const char *key, uint16_t& field);
template Status processField<int32_t,long>(JsonObject& jobj, const char *key, int32_t& field);
template Status processField<uint8_t,long>(JsonObject& jobj, const char *key, uint8_t& field);
template Status processField<float,double>(JsonObject& jobj, const char *key, float& field);
template Status processField<bool,bool>(JsonObject& jobj, const char *key, bool& field);

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

Status JsonController::processStepperPosition(Machine &machine, JsonCommand &jcmd, JsonObject& jobj, const char* key) {
	Status status = STATUS_OK;
	const char *s;
	if (strlen(key) == 3) {
		if ((s=jobj[key]) && *s==0) {
			JsonObject& node = jobj.createNestedObject(key);
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
			status = processStepperPosition(machine, jcmd, kidObj, it->key);
			if (status != STATUS_OK) {
				return status;
			}
		} 
	} else if (strcmp("x",key)==0 || strcmp("spox",key)==0) {
		status = processField<StepCoord,long>(jobj, key, machine.axis[0].position);
	} else if (strcmp("y",key)==0 || strcmp("spoy",key)==0) {
		status = processField<StepCoord,long>(jobj, key, machine.axis[1].position);
	} else if (strcmp("z",key)==0 || strcmp("spoz",key)==0) {
		status = processField<StepCoord,long>(jobj, key, machine.axis[2].position);
	} else if (strcmp("a",key)==0 || strcmp("spoa",key)==0) {
		status = processField<StepCoord,long>(jobj, key, machine.axis[3].position);
	} else if (strcmp("b",key)==0 || strcmp("spob",key)==0) {
		status = processField<StepCoord,long>(jobj, key, machine.axis[4].position);
	} else if (strcmp("c",key)==0 || strcmp("spoc",key)==0) {
		status = processField<StepCoord,long>(jobj, key, machine.axis[5].position);
	}
	return status;
}

Status JsonController::initializeStroke(Machine &machine, JsonCommand &jcmd, JsonObject& stroke) {
	int s1len = 0;
	int s2len = 0;
	int s3len = 0;
	int s4len = 0;
	bool us_ok = false;
	bool dp_ok = false;
	for (JsonObject::iterator it = stroke.begin(); it != stroke.end(); ++it) {
		if (strcmp("us", it->key) == 0) {
			Status status = processField<int32_t, long>(stroke, it->key, machine.stroke.planMicros);
			if (status != STATUS_OK) {
				return jcmd.setError(status, it->key);
			}
			us_ok = true;
		} else if (strcmp("dp", it->key) == 0) {
			JsonArray &jarr = stroke[it->key];
			if (!jarr.success()) {
				return jcmd.setError(STATUS_FIELD_ARRAY_ERROR, it->key);
			}
			if (!jarr[0].success()) {
				return jcmd.setError(STATUS_JSON_ARRAY_LEN, it->key);
			}
			dp_ok = true;
			for (int i=0; i<4 && jarr[i].success(); i++) {
				machine.stroke.dEndPos.value[i] = jarr[i];
			}
		} else if (strcmp("sc", it->key) == 0) {
			Status status = processField<StepDV, long>(stroke, it->key, machine.stroke.scale);
			if (status != STATUS_OK) {
				return jcmd.setError(status, it->key);
			}
		} else if (strcmp("s1", it->key) == 0) {
			JsonArray &jarr = stroke[it->key];
			if (!jarr.success()) {
				return jcmd.setError(STATUS_FIELD_ARRAY_ERROR, it->key);
			}
			for (JsonArray::iterator it2 = jarr.begin(); it2 != jarr.end(); ++it2) {
				if (*it2 < -127 || 127 < *it2) {
					return STATUS_S1_RANGE_ERROR;
				}
				machine.stroke.seg[s1len++].value[0] = (int8_t) (long) * it2;
			}
			stroke[it->key] = (long) 0;
		} else if (strcmp("s2", it->key) == 0) {
			JsonArray &jarr = stroke[it->key];
			if (!jarr.success()) {
				return jcmd.setError(STATUS_FIELD_ARRAY_ERROR, it->key);
			}
			for (JsonArray::iterator it2 = jarr.begin(); it2 != jarr.end(); ++it2) {
				if (*it2 < -127 || 127 < *it2) {
					return STATUS_S2_RANGE_ERROR;
				}
				machine.stroke.seg[s2len++].value[1] = (int8_t) (long) * it2;
			}
			stroke[it->key] = 0;
		} else if (strcmp("s3", it->key) == 0) {
			JsonArray &jarr = stroke[it->key];
			if (!jarr.success()) {
				return jcmd.setError(STATUS_FIELD_ARRAY_ERROR, it->key);
			}
			for (JsonArray::iterator it2 = jarr.begin(); it2 != jarr.end(); ++it2) {
				if (*it2 < -127 || 127 < *it2) {
					return STATUS_S3_RANGE_ERROR;
				}
				machine.stroke.seg[s3len++].value[2] = (int8_t) (long) * it2;
			}
			stroke[it->key] = 0;
		} else if (strcmp("s4", it->key) == 0) {
			JsonArray &jarr = stroke[it->key];
			if (!jarr.success()) {
				return jcmd.setError(STATUS_FIELD_ARRAY_ERROR, it->key);
			}
			for (JsonArray::iterator it2 = jarr.begin(); it2 != jarr.end(); ++it2) {
				if (*it2 < -127 || 127 < *it2) {
					return STATUS_S4_RANGE_ERROR;
				}
				machine.stroke.seg[s4len++].value[3] = (int8_t) (long) * it2;
			}
			stroke[it->key] = 0;
		} else {
			return jcmd.setError(STATUS_UNRECOGNIZED_NAME, it->key);
		}
	}
	if (!us_ok) {
		return jcmd.setError(STATUS_FIELD_REQUIRED, "us");
	}
	if (!dp_ok) {
		return jcmd.setError(STATUS_FIELD_REQUIRED, "dp");
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
	if (machine.stroke.length == 0) {
		return STATUS_STROKE_NULL_ERROR;
	}
	machine.stroke.start(ticks());
	return STATUS_BUSY_MOVING;
}

Status JsonController::traverseStroke(Machine &machine, JsonCommand &jcmd, JsonObject &stroke) {
	Status status =  machine.stroke.traverse(ticks(), machine);

	Quad<StepCoord> &pos = machine.stroke.position();
	stroke["s1"] = pos.value[0];
	stroke["s2"] = pos.value[1];
	stroke["s3"] = pos.value[2];
	stroke["s4"] = pos.value[3];

	return status;
}

Status JsonController::processStroke(Machine &machine, JsonCommand &jcmd, JsonObject& jobj, const char* key) {
    Status status = jcmd.getStatus();
	JsonObject &stroke = jobj[key];
	if (!stroke.success()) {
		return STATUS_JSON_STROKE_ERROR;
	}

    if (status == STATUS_BUSY_PARSED) {
		status = initializeStroke(machine, jcmd, stroke);
    } else if (status == STATUS_BUSY_MOVING) {
        if (machine.stroke.curSeg < machine.stroke.length) {
			status = traverseStroke(machine, jcmd, stroke);
        }
        if (machine.stroke.curSeg >= machine.stroke.length) {
            status = STATUS_OK;
        }
    }
    return status;
}

Status JsonController::processMotor(Machine &machine, JsonCommand &jcmd, JsonObject& jobj, const char* key, char group) {
    Status status = STATUS_OK;
    const char *s;
    int iMotor = group - '1';
	if (iMotor < 0 || MOTOR_COUNT <= iMotor) {
		return STATUS_MOTOR_INDEX;
	}
    if (strlen(key) == 1) {
        if ((s = jobj[key]) && *s == 0) {
            JsonObject& node = jobj.createNestedObject(key);
            node["ma"] = "";
        }
        JsonObject& kidObj = jobj[key];
        if (kidObj.success()) {
            for (JsonObject::iterator it = kidObj.begin(); it != kidObj.end(); ++it) {
                status = processMotor(machine, jcmd, kidObj, it->key, group);
                if (status != STATUS_OK) {
                    return status;
                }
            }
        }
    } else if (strcmp("ma", key) == 0 || strcmp("ma", key + 1) == 0) {
        status = processField<uint8_t, long>(jobj, key, machine.motor[iMotor].axisMap);
    }
    return status;
}

Status JsonController::processAxis(Machine &machine, JsonCommand &jcmd, JsonObject& jobj, const char* key, char group) {
    Status status = STATUS_OK;
    const char *s;
    int iAxis = axisOf(group);
    if (iAxis < 0) {
        return STATUS_AXIS_ERROR;
    }
    if (strlen(key) == 1) {
        if ((s = jobj[key]) && *s == 0) {
            JsonObject& node = jobj.createNestedObject(key);
            node["am"] = "";
			node["in"] = "";
			node["ln"] = "";
            node["mi"] = "";
            node["pd"] = "";
            node["pe"] = "";
            node["pm"] = "";
            node["pn"] = "";
            node["po"] = "";
            node["ps"] = "";
            node["sa"] = "";
            node["tm"] = "";
            node["tn"] = "";
        }
        JsonObject& kidObj = jobj[key];
        if (kidObj.success()) {
            for (JsonObject::iterator it = kidObj.begin(); it != kidObj.end(); ++it) {
                status = processAxis(machine, jcmd, kidObj, it->key, group);
                if (status != STATUS_OK) {
                    return status;
                }
            }
        }
    } else if (strcmp("am", key) == 0 || strcmp("am", key + 1) == 0) {
        status = processField<uint8_t, long>(jobj, key, machine.axis[iAxis].mode);
    } else if (strcmp("in", key) == 0 || strcmp("in", key + 1) == 0) {
        status = processField<bool, long>(jobj, key, machine.axis[iAxis].invertDir);
    } else if (strcmp("ln", key) == 0 || strcmp("ln", key + 1) == 0) {
        status = processField<bool, bool>(jobj, key, machine.axis[iAxis].atMin);
    } else if (strcmp("mi", key) == 0 || strcmp("mi", key + 1) == 0) {
        status = processField<uint8_t, long>(jobj, key, machine.axis[iAxis].microsteps);
    } else if (strcmp("pd", key) == 0 || strcmp("pd", key + 1) == 0) {
        status = processField<PinType, long>(jobj, key, machine.axis[iAxis].pinDir);
    } else if (strcmp("pe", key) == 0 || strcmp("pe", key + 1) == 0) {
        status = processField<PinType, long>(jobj, key, machine.axis[iAxis].pinEnable);
    } else if (strcmp("pm", key) == 0 || strcmp("pm", key + 1) == 0) {
        status = processField<uint8_t, long>(jobj, key, machine.axis[iAxis].powerManagementMode);
    } else if (strcmp("pn", key) == 0 || strcmp("pn", key + 1) == 0) {
        status = processField<PinType, long>(jobj, key, machine.axis[iAxis].pinMin);
    } else if (strcmp("po", key) == 0 || strcmp("po", key + 1) == 0) {
        status = processField<StepCoord, long>(jobj, key, machine.axis[iAxis].position);
    } else if (strcmp("ps", key) == 0 || strcmp("ps", key + 1) == 0) {
        status = processField<PinType, long>(jobj, key, machine.axis[iAxis].pinStep);
    } else if (strcmp("sa", key) == 0 || strcmp("sa", key + 1) == 0) {
        status = processField<float, double>(jobj, key, machine.axis[iAxis].stepAngle);
    } else if (strcmp("tm", key) == 0 || strcmp("tm", key + 1) == 0) {
        status = processField<StepCoord, long>(jobj, key, machine.axis[iAxis].travelMax);
    } else if (strcmp("tn", key) == 0 || strcmp("tn", key + 1) == 0) {
        status = processField<StepCoord, long>(jobj, key, machine.axis[iAxis].travelMin);
    }
    return status;
}

int freeRam () {
#ifdef TEST
	return 1000;
#else
    extern int __heap_start, *__brkval;
    int v;
    return (int)(size_t)&v - (__brkval == 0 ? (int)(size_t)&__heap_start : (int)(size_t)__brkval);
#endif
}

Status JsonController::processSys(Machine &machine, JsonCommand& jcmd, JsonObject& jobj, const char* key) {
    Status status = STATUS_OK;
    if (strcmp("sys", key) == 0) {
		const char *s;
		if ((s = jobj[key]) && *s == 0) {
			JsonObject& node = jobj.createNestedObject(key);
			node["dl"] = "";
			node["ds"] = "";
			node["fr"] = "";
			node["li"] = "";
			node["tc"] = "";
			node["v"] = "";
		}	
        JsonObject& kidObj = jobj[key];
        if (kidObj.success()) {
            for (JsonObject::iterator it = kidObj.begin(); it != kidObj.end(); ++it) {
                status = processSys(machine, jcmd, kidObj, it->key);
                if (status != STATUS_OK) {
                    return status;
                }
            }
        }
    } else if (strcmp("dl", key) == 0 || strcmp("sysdl", key) == 0) {
		uint8_t level = machine.pDisplay->getLevel();
        status = processField<uint8_t, long>(jobj, key, level);
		if (level != machine.pDisplay->getLevel()) {
			machine.pDisplay->setLevel(level);
		}
    } else if (strcmp("ds", key) == 0 || strcmp("sysds", key) == 0) {
		const char *s;
		bool isAssignment = (!(s=jobj[key]) || *s!=0);
        status = processField<uint8_t, long>(jobj, key, machine.pDisplay->status);
		if (isAssignment) {
			switch (machine.pDisplay->status) {
				case DISPLAY_WAIT_IDLE:
					status = STATUS_WAIT_IDLE;
					break;
				case DISPLAY_WAIT_ERROR:
					status = STATUS_WAIT_ERROR;
					break;
				case DISPLAY_WAIT_OPERATOR:
					status = STATUS_WAIT_OPERATOR;
					break;
				case DISPLAY_BUSY_MOVING:
					status = STATUS_WAIT_MOVING;
					break;
				case DISPLAY_BUSY:
					status = STATUS_WAIT_BUSY;
					break;
				case DISPLAY_WAIT_CAMERA:
					status = STATUS_WAIT_CAMERA;
					break;
			}
		}
    } else if (strcmp("fr", key) == 0 || strcmp("sysfr", key) == 0) {
        jobj[key] = freeRam();
    } else if (strcmp("v", key) == 0 || strcmp("sysv", key) == 0) {
        jobj[key] = VERSION_MAJOR * 100 + VERSION_MINOR + VERSION_PATCH / 100.0;
    } else if (strcmp("li", key) == 0 || strcmp("sysli", key) == 0) {
        status = processField<bool, bool>(jobj, key, machine.invertLim);
    } else if (strcmp("tc", key) == 0 || strcmp("systc", key) == 0) {
		jobj[key] = threadClock.ticks;
	} else {
		status = jcmd.setError(STATUS_UNRECOGNIZED_NAME, key);
    }
    return status;
}

Status JsonController::processDisplay(Machine &machine, JsonCommand& jcmd, JsonObject& jobj, const char* key) {
    Status status = STATUS_OK;
    if (strcmp("dpy", key) == 0) {
		const char *s;
		if ((s = jobj[key]) && *s == 0) {
			JsonObject& node = jobj.createNestedObject(key);
			node["cb"] = "";
			node["cg"] = "";
			node["cr"] = "";
			node["dl"] = "";
			node["ds"] = "";
		}	
        JsonObject& kidObj = jobj[key];
        if (kidObj.success()) {
            for (JsonObject::iterator it = kidObj.begin(); it != kidObj.end(); ++it) {
                status = processDisplay(machine, jcmd, kidObj, it->key);
                if (status != STATUS_OK) {
                    return status;
                }
            }
        }
    } else if (strcmp("cb", key) == 0 || strcmp("dpycb", key) == 0) {
        status = processField<uint8_t, long>(jobj, key, machine.pDisplay->cameraB);
    } else if (strcmp("cg", key) == 0 || strcmp("dpycg", key) == 0) {
        status = processField<uint8_t, long>(jobj, key, machine.pDisplay->cameraG);
    } else if (strcmp("cr", key) == 0 || strcmp("dpycr", key) == 0) {
        status = processField<uint8_t, long>(jobj, key, machine.pDisplay->cameraR);
    } else if (strcmp("dl", key) == 0 || strcmp("dpydl", key) == 0) {
        status = processField<uint8_t, long>(jobj, key, machine.pDisplay->level);
    } else if (strcmp("ds", key) == 0 || strcmp("dpyds", key) == 0) {
		const char *s;
		bool isAssignment = (!(s=jobj[key]) || *s!=0);
        status = processField<uint8_t, long>(jobj, key, machine.pDisplay->status);
		if (isAssignment) {
			switch (machine.pDisplay->status) {
				case DISPLAY_WAIT_IDLE:
					status = STATUS_WAIT_IDLE;
					break;
				case DISPLAY_WAIT_ERROR:
					status = STATUS_WAIT_ERROR;
					break;
				case DISPLAY_WAIT_OPERATOR:
					status = STATUS_WAIT_OPERATOR;
					break;
				case DISPLAY_BUSY_MOVING:
					status = STATUS_WAIT_MOVING;
					break;
				case DISPLAY_BUSY:
					status = STATUS_WAIT_BUSY;
					break;
				case DISPLAY_WAIT_CAMERA:
					status = STATUS_WAIT_CAMERA;
					break;
			}
		}
	} else {
		status = jcmd.setError(STATUS_UNRECOGNIZED_NAME, key);
    }
    return status;
}

Status JsonController::process(Machine &machine, JsonCommand& jcmd) {
    const char *s;
    JsonObject& root = jcmd.requestRoot();
    JsonVariant node;
    node = root;
    Status status = STATUS_OK;

    for (JsonObject::iterator it = root.begin(); it != root.end(); ++it) {
        if (strcmp("dvs", it->key) == 0) {
            status = processStroke(machine, jcmd, root, it->key);
        } else if (strncmp("sys", it->key, 3) == 0) {
            status = processSys(machine, jcmd, root, it->key);
        } else if (strncmp("dpy", it->key, 3) == 0) {
            status = processDisplay(machine, jcmd, root, it->key);
        } else if (strncmp("spo", it->key, 3) == 0) {
            status = processStepperPosition(machine, jcmd, root, it->key);
        } else {
            switch (it->key[0]) {
            case '1':
            case '2':
            case '3':
            case '4':
                status = processMotor(machine, jcmd, root, it->key, it->key[0]);
                break;
            case 'x':
            case 'y':
            case 'z':
            case 'a':
            case 'b':
            case 'c':
                status = processAxis(machine, jcmd, root, it->key, it->key[0]);
                break;
            default:
                status = jcmd.setError(STATUS_UNRECOGNIZED_NAME, it->key);
                break;
            }
        }
    }

    jcmd.setStatus(status);

	if (!isProcessing(status)) {
		jcmd.response().printTo(Serial);
		Serial.println();
	}

	return status;
}


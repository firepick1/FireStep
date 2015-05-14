#include "Arduino.h"
#ifdef CMAKE
#include <cstring>
#include <cstdio>
#endif
#include "version.h"
#include "JsonController.h"

using namespace firestep;

JsonController::JsonController(Machine& machine)
	: machine(machine) {
	lastProcessed = 0;
}

Status JsonController::setup() {
	lastProcessed = threadClock.ticks;
	return STATUS_OK;
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
template Status processField<uint16_t,long>(JsonObject& jobj, const char *key, uint16_t& field);
template Status processField<int32_t,long>(JsonObject& jobj, const char *key, int32_t& field);
template Status processField<uint8_t,long>(JsonObject& jobj, const char *key, uint8_t& field);
template Status processField<float,double>(JsonObject& jobj, const char *key, float& field);
template Status processField<bool,bool>(JsonObject& jobj, const char *key, bool& field);

Status processHomeField(Machine& machine, AxisIndex iAxis, JsonCommand &jcmd, JsonObject &jobj, const char *key) {
    Status status = processField<StepCoord, long>(jobj, key, machine.axis[iAxis].home);
    jobj[key] = machine.axis[iAxis].home;
    machine.axis[iAxis].homing = true;

	return status;
}

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

Status JsonController::processStepperPosition(JsonCommand &jcmd, JsonObject& jobj, const char* key) {
    Status status = STATUS_OK;
    const char *s;
    if (strlen(key) == 3) {
        if ((s = jobj[key]) && *s == 0) {
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
        for (JsonObject::iterator it = kidObj.begin(); it != kidObj.end(); ++it) {
            status = processStepperPosition(jcmd, kidObj, it->key);
            if (status != STATUS_OK) {
                return status;
            }
        }
    } else if (strcmp("x", key) == 0 || strcmp("spox", key) == 0) {
        status = processField<StepCoord, long>(jobj, key, machine.axis[0].position);
    } else if (strcmp("y", key) == 0 || strcmp("spoy", key) == 0) {
        status = processField<StepCoord, long>(jobj, key, machine.axis[1].position);
    } else if (strcmp("z", key) == 0 || strcmp("spoz", key) == 0) {
        status = processField<StepCoord, long>(jobj, key, machine.axis[2].position);
    } else if (strcmp("a", key) == 0 || strcmp("spoa", key) == 0) {
        status = processField<StepCoord, long>(jobj, key, machine.axis[3].position);
    } else if (strcmp("b", key) == 0 || strcmp("spob", key) == 0) {
        status = processField<StepCoord, long>(jobj, key, machine.axis[4].position);
    } else if (strcmp("c", key) == 0 || strcmp("spoc", key) == 0) {
        status = processField<StepCoord, long>(jobj, key, machine.axis[5].position);
    }
    return status;
}

Status JsonController::initializeStrokeArray(JsonCommand &jcmd, 
	JsonObject& stroke, const char *key, MotorIndex iMotor, int16_t &slen) {
    JsonArray &jarr = stroke[key];
    if (!jarr.success()) {
        return jcmd.setError(STATUS_FIELD_ARRAY_ERROR, key);
    }
    for (JsonArray::iterator it2 = jarr.begin(); it2 != jarr.end(); ++it2) {
        if (*it2 < -127 || 127 < *it2) {
            return jcmd.setError(STATUS_RANGE_ERROR, key);
        }
        machine.stroke.seg[slen++].value[iMotor] = (int8_t) (long) * it2;
    }
    stroke[key] = (long) 0;
	return STATUS_BUSY_MOVING;
}

Status JsonController::initializeStroke(JsonCommand &jcmd, JsonObject& stroke) {
	Status status = STATUS_BUSY_MOVING;
    int16_t slen[4] = {0,0,0,0};
    bool us_ok = false;
    for (JsonObject::iterator it = stroke.begin(); it != stroke.end(); ++it) {
        if (strcmp("us", it->key) == 0) {
            status = processField<int32_t, long>(stroke, it->key, machine.stroke.planMicros);
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
            for (int i = 0; i < 4 && jarr[i].success(); i++) {
                machine.stroke.dEndPos.value[i] = jarr[i];
            }
        } else if (strcmp("sc", it->key) == 0) {
            status = processField<StepDV, long>(stroke, it->key, machine.stroke.scale);
            if (status != STATUS_OK) {
                return jcmd.setError(status, it->key);
            }
		} else {
			MotorIndex iMotor = machine.motorOfName(it->key);
            if (iMotor == INDEX_NONE) {
                return jcmd.setError(STATUS_NO_MOTOR, it->key);
            }
			status = initializeStrokeArray(jcmd, stroke, it->key, iMotor, slen[iMotor]);
        }
    }
	if (status != STATUS_BUSY_MOVING) {
		return status;
	}
    if (!us_ok) {
        return jcmd.setError(STATUS_FIELD_REQUIRED, "us");
    }
    if (slen[0] && slen[1] && slen[0] != slen[1]) {
        return STATUS_S1S2LEN_ERROR;
    }
    if (slen[0] && slen[2] && slen[0] != slen[2]) {
        return STATUS_S1S3LEN_ERROR;
    }
    if (slen[0] && slen[3] && slen[0] != slen[3]) {
        return STATUS_S1S4LEN_ERROR;
    }
    machine.stroke.length = slen[0] ? slen[0] : (slen[1] ? slen[1] : (slen[2] ? slen[2] : slen[3]));
    if (machine.stroke.length == 0) {
        return STATUS_STROKE_NULL_ERROR;
    }
    machine.stroke.start(ticks());
    return status;
}

Status JsonController::traverseStroke(JsonCommand &jcmd, JsonObject &stroke) {
    Status status =  machine.stroke.traverse(ticks(), machine);

    Quad<StepCoord> &pos = machine.stroke.position();
    for (JsonObject::iterator it = stroke.begin(); it != stroke.end(); ++it) {
		MotorIndex iMotor = machine.motorOfName(it->key);
		if (iMotor != INDEX_NONE) {
		cout << "key:" << it->key << " iMotor:" << (int) iMotor << endl;
			stroke[it->key] = pos.value[iMotor];
		}
	}

    return status;
}

Status JsonController::processStroke(JsonCommand &jcmd, JsonObject& jobj, const char* key) {
    JsonObject &stroke = jobj[key];
    if (!stroke.success()) {
        return STATUS_JSON_STROKE_ERROR;
    }

    Status status = jcmd.getStatus();
    if (status == STATUS_BUSY_PARSED) {
        status = initializeStroke(jcmd, stroke);
    } else if (status == STATUS_BUSY_MOVING) {
        if (machine.stroke.curSeg < machine.stroke.length) {
            status = traverseStroke(jcmd, stroke);
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
                status = processMotor(jcmd, kidObj, it->key, group);
                if (status != STATUS_OK) {
                    return status;
                }
            }
        }
    } else if (strcmp("ma", key) == 0 || strcmp("ma", key + 1) == 0) {
        AxisIndex iAxis = machine.getMotorAxis(iMotor);
        status = processField<AxisIndex, long>(jobj, key, iAxis);
        machine.setMotorAxis(iMotor, iAxis);
    }
    return status;
}

Status JsonController::processPin(JsonObject& jobj, const char *key, PinType &pin, int16_t mode, int16_t value) {
    PinType newPin = pin;
    Status status = processField<PinType, long>(jobj, key, newPin);
    machine.setPin(pin, newPin, mode, value);
    return status;
}

Status JsonController::processAxis(JsonCommand &jcmd, JsonObject& jobj, const char* key, char group) {
    Status status = STATUS_OK;
    const char *s;
    int iAxis = axisOf(group);
    if (iAxis < 0) {
        return STATUS_AXIS_ERROR;
    }
    Axis &axis = machine.axis[iAxis];
    if (strlen(key) == 1) {
        if ((s = jobj[key]) && *s == 0) {
            JsonObject& node = jobj.createNestedObject(key);
            node["dh"] = "";
            node["en"] = "";
            node["ho"] = "";
            node["is"] = "";
            node["lb"] = "";
            node["lm"] = "";
            node["ln"] = "";
            node["mi"] = "";
            node["pd"] = "";
            node["pe"] = "";
            node["pm"] = "";
            node["pn"] = "";
            node["po"] = "";
            node["ps"] = "";
            node["sa"] = "";
            node["sd"] = "";
            node["tm"] = "";
            node["tn"] = "";
            node["ud"] = "";
            if (!node.at("ud").success()) {
                return jcmd.setError(STATUS_JSON_KEY, "ud");
            }
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
    } else if (strcmp("en", key) == 0 || strcmp("en", key + 1) == 0) {
        bool active = axis.isEnabled();
        status = processField<bool, bool>(jobj, key, active);
        if (status == STATUS_OK) {
            axis.enable(active);
            status = (jobj[key] = axis.isEnabled()).success() ? status : STATUS_FIELD_ERROR;
        }
    } else if (strcmp("dh", key) == 0 || strcmp("dh", key + 1) == 0) {
        status = processField<bool, bool>(jobj, key, axis.dirHIGH);
    } else if (strcmp("ho", key) == 0 || strcmp("ho", key + 1) == 0) {
        status = processField<StepCoord, long>(jobj, key, axis.home);
    } else if (strcmp("is", key) == 0 || strcmp("is", key + 1) == 0) {
        status = processField<DelayMics, long>(jobj, key, axis.idleSnooze);
    } else if (strcmp("lb", key) == 0 || strcmp("lb", key + 1) == 0) {
        status = processField<StepCoord, long>(jobj, key, axis.latchBackoff);
    } else if (strcmp("lm", key) == 0 || strcmp("lm", key + 1) == 0) {
        axis.readAtMax(machine.invertLim);
        status = processField<bool, bool>(jobj, key, axis.atMax);
    } else if (strcmp("ln", key) == 0 || strcmp("ln", key + 1) == 0) {
        axis.readAtMin(machine.invertLim);
        status = processField<bool, bool>(jobj, key, axis.atMin);
    } else if (strcmp("mi", key) == 0 || strcmp("mi", key + 1) == 0) {
        status = processField<uint8_t, long>(jobj, key, axis.microsteps);
        if (axis.microsteps < 1) {
            axis.microsteps = 1;
            return STATUS_JSON_POSITIVE1;
        }
    } else if (strcmp("pd", key) == 0 || strcmp("pd", key + 1) == 0) {
        status = processPin(jobj, key, axis.pinDir, OUTPUT);
    } else if (strcmp("pe", key) == 0 || strcmp("pe", key + 1) == 0) {
        status = processPin(jobj, key, axis.pinEnable, OUTPUT, HIGH);
    } else if (strcmp("pm", key) == 0 || strcmp("pm", key + 1) == 0) {
        status = processPin(jobj, key, axis.pinMax, INPUT);
    } else if (strcmp("pn", key) == 0 || strcmp("pn", key + 1) == 0) {
        status = processPin(jobj, key, axis.pinMin, INPUT);
    } else if (strcmp("po", key) == 0 || strcmp("po", key + 1) == 0) {
        status = processField<StepCoord, long>(jobj, key, axis.position);
    } else if (strcmp("ps", key) == 0 || strcmp("ps", key + 1) == 0) {
        status = processPin(jobj, key, axis.pinStep, OUTPUT);
    } else if (strcmp("sa", key) == 0 || strcmp("sa", key + 1) == 0) {
        status = processField<float, double>(jobj, key, axis.stepAngle);
    } else if (strcmp("sd", key) == 0 || strcmp("sd", key + 1) == 0) {
        status = processField<DelayMics, long>(jobj, key, axis.searchDelay);
    } else if (strcmp("tm", key) == 0 || strcmp("tm", key + 1) == 0) {
        status = processField<StepCoord, long>(jobj, key, axis.travelMax);
    } else if (strcmp("tn", key) == 0 || strcmp("tn", key + 1) == 0) {
        status = processField<StepCoord, long>(jobj, key, axis.travelMin);
    } else if (strcmp("ud", key) == 0 || strcmp("ud", key + 1) == 0) {
        cout << "ud" << endl;
        status = processField<DelayMics, long>(jobj, key, axis.usDelay);
    } else {
        return jcmd.setError(STATUS_UNRECOGNIZED_NAME, key);
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

Status JsonController::processRawSteps(Quad<StepCoord> &steps) {
    while (!steps.isZero()) {
        Quad<StepCoord> motorPos = machine.getMotorPosition();
        Quad<StepCoord> pulse(steps.sgn());
        motorPos -= pulse;
        machine.setMotorPosition(motorPos); // permit infinite travel

        Status status = machine.step(pulse);
        if (status != STATUS_OK) {
            return status;
        }
        steps -= pulse;
    }

    return STATUS_OK;
}

Status JsonController::processTest(JsonCommand& jcmd, JsonObject& jobj, const char* key) {
    Status status = jcmd.getStatus();

    switch (status) {
    case STATUS_BUSY_PARSED:
    case STATUS_BUSY_MOVING:
        if (strcmp("tst", key) == 0) {
            JsonObject& tst = jobj[key];
            if (!tst.success()) {
                return jcmd.setError(STATUS_JSON_OBJECT, key);
            }
            for (JsonObject::iterator it = tst.begin(); it != tst.end(); ++it) {
                status = processTest(jcmd, tst, it->key);
            }
        } else if (strcmp("rv", key) == 0 || strcmp("tstrv", key) == 0) { // revolution steps
            JsonArray &jarr = jobj[key];
            if (!jarr.success()) {
                return jcmd.setError(STATUS_FIELD_ARRAY_ERROR, key);
            }
            Quad<StepCoord> steps;
            for (int i = 0; i < 4; i++) {
                if (jarr[i].success()) {
                    Axis &a = machine.axis[machine.getMotorAxis(i)];
                    int16_t revs = jarr[i];
                    int16_t revSteps = 360 / a.stepAngle;
                    int16_t revMicrosteps = revSteps * a.microsteps;
                    int16_t msRev = (a.usDelay * revMicrosteps) / 1000;
                    steps.value[i] = revs * revMicrosteps;
                }
            }
            Quad<StepCoord> steps1(steps);
            status = processRawSteps(steps1);
            if (status == STATUS_OK) {
                delay(250);
                Quad<StepCoord> steps2(steps.absoluteValue());
                status = processRawSteps(steps2);
                delay(250);
            }
            if (status == STATUS_OK) {
                status = STATUS_BUSY_MOVING;
            }
        } else if (strcmp("sp", key) == 0 || strcmp("tstsp", key) == 0) { // step pulses
            JsonArray &jarr = jobj[key];
            if (!jarr.success()) {
                return jcmd.setError(STATUS_FIELD_ARRAY_ERROR, key);
            }
            Quad<StepCoord> steps;
            for (int i = 0; i < 4; i++) {
                if (jarr[i].success()) {
                    steps.value[i] = jarr[i];
                }
            }
            status = processRawSteps(steps);
        } else {
            return jcmd.setError(STATUS_UNRECOGNIZED_NAME, key);
        }
        break;
    }
    return status;
}

Status JsonController::processSys(JsonCommand& jcmd, JsonObject& jobj, const char* key) {
    Status status = STATUS_OK;
    if (strcmp("sys", key) == 0) {
        const char *s;
        if ((s = jobj[key]) && *s == 0) {
            JsonObject& node = jobj.createNestedObject(key);
            node["fr"] = "";
            node["jp"] = "";
            node["lh"] = "";
            node["tc"] = "";
            node["v"] = "";
        }
        JsonObject& kidObj = jobj[key];
        if (kidObj.success()) {
            for (JsonObject::iterator it = kidObj.begin(); it != kidObj.end(); ++it) {
                status = processSys(jcmd, kidObj, it->key);
                if (status != STATUS_OK) {
                    return status;
                }
            }
        }
    } else if (strcmp("fr", key) == 0 || strcmp("sysfr", key) == 0) {
        jobj[key] = freeRam();
    } else if (strcmp("jp", key) == 0 || strcmp("sysjp", key) == 0) {
        status = processField<bool, bool>(jobj, key, machine.jsonPrettyPrint);
    } else if (strcmp("lh", key) == 0 || strcmp("syslh", key) == 0) {
        status = processField<bool, bool>(jobj, key, machine.invertLim);
    } else if (strcmp("tc", key) == 0 || strcmp("systc", key) == 0) {
        jobj[key] = threadClock.ticks;
    } else if (strcmp("v", key) == 0 || strcmp("sysv", key) == 0) {
        jobj[key] = VERSION_MAJOR * 100 + VERSION_MINOR + VERSION_PATCH / 100.0;
    } else {
        return jcmd.setError(STATUS_UNRECOGNIZED_NAME, key);
    }
    return status;
}

Status JsonController::initializeHome(JsonCommand& jcmd, JsonObject& jobj, const char* key) {
    Status status = STATUS_OK;
    if (strcmp("ho", key) == 0) {
        JsonObject& kidObj = jobj[key];
        if (kidObj.success()) {
            for (JsonObject::iterator it = kidObj.begin(); it != kidObj.end(); ++it) {
                status = initializeHome(jcmd, kidObj, it->key);
                if (status != STATUS_BUSY_MOVING) {
                    return status;
                }
            }
        }
	} else {
		MotorIndex iMotor = machine.motorOfName(key);
		if (iMotor == INDEX_NONE) {
			return jcmd.setError(STATUS_NO_MOTOR, key);
		}
        status = processHomeField(machine, iMotor, jcmd, jobj, key);
    }
    return status == STATUS_OK ? STATUS_BUSY_MOVING : status;
}

Status JsonController::processHome(JsonCommand& jcmd, JsonObject& jobj, const char* key) {
    Status status = jcmd.getStatus();
    if (status == STATUS_BUSY_PARSED) {
        status = initializeHome(jcmd, jobj, key);
    } else if (status == STATUS_BUSY_MOVING) {
        status = machine.home();
    } else {
        return jcmd.setError(STATUS_STATE, key);
    }
    return status;
}

Status JsonController::initializeMove(JsonCommand& jcmd, JsonObject& jobj, const char* key) {
    Status status = STATUS_OK;
    if (strcmp("mov", key) == 0) {
        JsonObject& kidObj = jobj[key];
        if (kidObj.success()) {
            jcmd.move.clear();
            jcmd.stepRate = 0;
            for (JsonObject::iterator it = kidObj.begin(); it != kidObj.end(); ++it) {
                status = initializeMove(jcmd, kidObj, it->key);
                if (status != STATUS_BUSY_MOVING) {
                    return status;
                }
            }
        }
	} else if (strcmp("sr", key) == 0) {
        status = processField<StepCoord, long>(jobj, key, jcmd.stepRate);
	} else {
        MotorIndex iMotor = machine.motorOfName(key);
        if (iMotor == INDEX_NONE) {
            return jcmd.setError(STATUS_NO_MOTOR, key);
        }
        status = processField<StepCoord, long>(jobj, key, jcmd.move.value[iMotor]);
    }
    return status == STATUS_OK ? STATUS_BUSY_MOVING : status;
}

Status JsonController::processMove(JsonCommand& jcmd, JsonObject& jobj, const char* key) {
    Status status = jcmd.getStatus();
    if (status == STATUS_BUSY_PARSED) {
        status = initializeMove(jcmd, jobj, key);
    } else if (status == STATUS_BUSY_MOVING) {
        status = machine.moveTo(jcmd.move, jcmd.stepRate);
    } else {
        return jcmd.setError(STATUS_STATE, key);
    }
    return status;
}

Status JsonController::processDisplay(JsonCommand& jcmd, JsonObject& jobj, const char* key) {
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
                status = processDisplay(jcmd, kidObj, it->key);
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
        bool isAssignment = (!(s = jobj[key]) || *s != 0);
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
        return jcmd.setError(STATUS_UNRECOGNIZED_NAME, key);
    }
    return status;
}

Status JsonController::cancel(JsonCommand& jcmd, Status cause) {
    jcmd.setStatus(cause);
    sendResponse(jcmd);
    return STATUS_WAIT_CANCELLED;
}

void JsonController::sendResponse(JsonCommand &jcmd) {
    if (machine.jsonPrettyPrint) {
        jcmd.response().prettyPrintTo(Serial);
    } else {
        jcmd.response().printTo(Serial);
    }
    Serial.println();
}

Status JsonController::process(JsonCommand& jcmd) {
    const char *s;
    JsonObject& root = jcmd.requestRoot();
    JsonVariant node;
    node = root;
    Status status = STATUS_OK;

    for (JsonObject::iterator it = root.begin(); it != root.end(); ++it) {
        if (strcmp("dvs", it->key) == 0) {
            status = processStroke(jcmd, root, it->key);
        } else if (strcmp("mov", it->key) == 0) {
            status = processMove(jcmd, root, it->key);
        } else if (strncmp("ho", it->key, 2) == 0) {
            status = processHome(jcmd, root, it->key);
        } else if (strncmp("tst", it->key, 3) == 0) {
            status = processTest(jcmd, root, it->key);
        } else if (strncmp("sys", it->key, 3) == 0) {
            status = processSys(jcmd, root, it->key);
        } else if (strncmp("dpy", it->key, 3) == 0) {
            status = processDisplay(jcmd, root, it->key);
        } else if (strncmp("spo", it->key, 3) == 0) {
            status = processStepperPosition(jcmd, root, it->key);
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
                status = jcmd.setError(STATUS_UNRECOGNIZED_NAME, it->key);
                break;
            }
        }
    }

    jcmd.setStatus(status);

    if (!isProcessing(status)) {
        sendResponse(jcmd);
    }
    lastProcessed = threadClock.ticks;

    return status;
}


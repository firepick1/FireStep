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
}

Status JsonController::setup() {
    return STATUS_OK;
}

template<class TF, class TJ>
Status processField(JsonObject& jobj, const char* key, TF& field) {
    Status status = STATUS_OK;
    const char *s;
    if ((s = jobj[key]) && *s == 0) { // query
        status = (jobj[key] = (TJ) field).success() ? status : STATUS_FIELD_ERROR;
    } else {
        float value = (TJ)jobj[key];
        field = (TF)value;
        if ((float) field != value) {
            return STATUS_VALUE_RANGE;
        }
        jobj[key] = (TJ) field;
    }
    return status;
}
template Status processField<int16_t, int32_t>(JsonObject& jobj, const char *key, int16_t& field);
template Status processField<uint16_t, int32_t>(JsonObject& jobj, const char *key, uint16_t& field);
template Status processField<int32_t, int32_t>(JsonObject& jobj, const char *key, int32_t& field);
template Status processField<uint8_t, int32_t>(JsonObject& jobj, const char *key, uint8_t& field);
template Status processField<PH5TYPE, PH5TYPE>(JsonObject& jobj, const char *key, PH5TYPE& field);
template Status processField<bool, bool>(JsonObject& jobj, const char *key, bool& field);

Status processHomeField(Machine& machine, AxisIndex iAxis, JsonCommand &jcmd, JsonObject &jobj, const char *key) {
    Status status = processField<StepCoord, int32_t>(jobj, key, machine.axis[iAxis].home);
    if (machine.axis[iAxis].isEnabled()) {
        jobj[key] = machine.axis[iAxis].home;
        machine.axis[iAxis].homing = true;
    } else {
        jobj[key] = machine.axis[iAxis].position;
        machine.axis[iAxis].homing = false;
    }

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
            node["1"] = "";
            node["2"] = "";
            node["3"] = "";
            node["4"] = "";
            if (!node.at("4").success()) {
                return jcmd.setError(STATUS_JSON_KEY, "4");
            }
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
    } else {
        AxisIndex iAxis = machine.axisOfName(key);
        if (iAxis == INDEX_NONE) {
            if (strlen(key) > 3) {
                iAxis = machine.axisOfName(key + 3);
                if (iAxis == INDEX_NONE) {
                    return jcmd.setError(STATUS_NO_MOTOR, key);
                }
            } else {
                return jcmd.setError(STATUS_NO_MOTOR, key);
            }
        }
        status = processField<StepCoord, int32_t>(jobj, key, machine.axis[iAxis].position);
    }
    return status;
}

inline int8_t hexValue(char c) {
	switch (c) {
	case '0': return 0;
	case '1': return 1;
	case '2': return 2;
	case '3': return 3;
	case '4': return 4;
	case '5': return 5;
	case '6': return 6;
	case '7': return 7;
	case '8': return 8;
	case '9': return 9;
	case 'A': 
	case 'a': return 0xa;
	case 'B': 
	case 'b': return 0xb;
	case 'C': 
	case 'c': return 0xc;
	case 'D': 
	case 'd': return 0xd;
	case 'E': 
	case 'e': return 0xe;
	case 'F': 
	case 'f': return 0xf;
	default: return -1;
	}
}

Status JsonController::initializeStrokeArray(JsonCommand &jcmd,
        JsonObject& stroke, const char *key, MotorIndex iMotor, int16_t &slen) {
	if (stroke.at(key).is<JsonArray&>()) {
		JsonArray &jarr = stroke[key];
		for (JsonArray::iterator it2 = jarr.begin(); it2 != jarr.end(); ++it2) {
			if (*it2 < -127 || 127 < *it2) {
				return STATUS_RANGE_ERROR;
			}
			machine.stroke.seg[slen++].value[iMotor] = (StepDV) (int32_t) * it2;
		}
	} else if (stroke.at(key).is<const char*>()) {
		const char *s = stroke[key];
		while (*s) {
			int8_t high = hexValue(*s++);
			int8_t low = hexValue(*s++);
			if (high < 0 || low < 0) {
				return STATUS_FIELD_HEX_ERROR;
			}
			StepDV dv = ((high<<4) | low);
			//TESTCOUT3("initializeStrokeArray(", key, ") sLen:", (int)slen, " dv:", (int) dv);
			machine.stroke.seg[slen++].value[iMotor] = dv;
		}
	} else {
		return STATUS_FIELD_ARRAY_ERROR;
	}
	stroke[key] = (int32_t) 0;
    return STATUS_OK;
}

Status JsonController::initializeStroke(JsonCommand &jcmd, JsonObject& stroke) {
    Status status = STATUS_OK;
    int16_t slen[4] = {0, 0, 0, 0};
    bool us_ok = false;
    machine.stroke.clear();
    for (JsonObject::iterator it = stroke.begin(); it != stroke.end(); ++it) {
        if (strcmp("us", it->key) == 0) {
            int32_t planMicros;
            status = processField<int32_t, int32_t>(stroke, it->key, planMicros);
            if (status != STATUS_OK) {
                return jcmd.setError(status, it->key);
            }
            float seconds = (float) planMicros / 1000000.0;
            machine.stroke.setTimePlanned(seconds);
            us_ok = true;
        } else if (strcmp("dp", it->key) == 0) {
            JsonArray &jarr = stroke[it->key];
            if (!jarr.success()) {
                return jcmd.setError(STATUS_FIELD_ARRAY_ERROR, it->key);
            }
            if (!jarr[0].success()) {
                return jcmd.setError(STATUS_JSON_ARRAY_LEN, it->key);
            }
            for (MotorIndex i = 0; i < 4 && jarr[i].success(); i++) {
                machine.stroke.dEndPos.value[i] = jarr[i];
            }
        } else if (strcmp("sc", it->key) == 0) {
            status = processField<StepCoord, int32_t>(stroke, it->key, machine.stroke.scale);
            if (status != STATUS_OK) {
                return jcmd.setError(status, it->key);
            }
        } else {
            MotorIndex iMotor = machine.motorOfName(it->key);
            if (iMotor == INDEX_NONE) {
                return jcmd.setError(STATUS_NO_MOTOR, it->key);
            }
            status = initializeStrokeArray(jcmd, stroke, it->key, iMotor, slen[iMotor]);
			if (status != STATUS_OK) {
                return jcmd.setError(status, it->key);
			}
        }
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
    status = machine.stroke.start(ticks());
	if (status != STATUS_OK) {
        return status;
	}
    return STATUS_BUSY_MOVING;
}

Status JsonController::traverseStroke(JsonCommand &jcmd, JsonObject &stroke) {
    Status status =  machine.stroke.traverse(ticks(), machine);

    Quad<StepCoord> &pos = machine.stroke.position();
    for (JsonObject::iterator it = stroke.begin(); it != stroke.end(); ++it) {
        MotorIndex iMotor = machine.motorOfName(it->key + (strlen(it->key) - 1));
        if (iMotor != INDEX_NONE) {
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
        JsonVariant &jv = jobj[key];
        MotorIndex iMotor = group - '1';
        if (iMotor < 0 || MOTOR_COUNT <= iMotor) {
            return STATUS_MOTOR_INDEX;
        }
        AxisIndex iAxis = machine.getAxisIndex(iMotor);
        status = processField<AxisIndex, int32_t>(jobj, key, iAxis);
        machine.setAxisIndex(iMotor, iAxis);
    }
    return status;
}

Status JsonController::processPin(JsonObject& jobj, const char *key, PinType &pin, int16_t mode, int16_t value) {
    PinType newPin = pin;
    Status status = processField<PinType, int32_t>(jobj, key, newPin);
    machine.setPin(pin, newPin, mode, value);
    return status;
}

Status JsonController::processAxis(JsonCommand &jcmd, JsonObject& jobj, const char* key, char group) {
    Status status = STATUS_OK;
    const char *s;
    AxisIndex iAxis = axisOf(group);
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
        status = processField<StepCoord, int32_t>(jobj, key, axis.home);
    } else if (strcmp("is", key) == 0 || strcmp("is", key + 1) == 0) {
        status = processField<DelayMics, int32_t>(jobj, key, axis.idleSnooze);
    } else if (strcmp("lb", key) == 0 || strcmp("lb", key + 1) == 0) {
        status = processField<StepCoord, int32_t>(jobj, key, axis.latchBackoff);
    } else if (strcmp("lm", key) == 0 || strcmp("lm", key + 1) == 0) {
        axis.readAtMax(machine.invertLim);
        status = processField<bool, bool>(jobj, key, axis.atMax);
    } else if (strcmp("ln", key) == 0 || strcmp("ln", key + 1) == 0) {
        axis.readAtMin(machine.invertLim);
        status = processField<bool, bool>(jobj, key, axis.atMin);
    } else if (strcmp("mi", key) == 0 || strcmp("mi", key + 1) == 0) {
        status = processField<uint8_t, int32_t>(jobj, key, axis.microsteps);
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
        status = processField<StepCoord, int32_t>(jobj, key, axis.position);
    } else if (strcmp("ps", key) == 0 || strcmp("ps", key + 1) == 0) {
        status = processPin(jobj, key, axis.pinStep, OUTPUT);
    } else if (strcmp("sa", key) == 0 || strcmp("sa", key + 1) == 0) {
        status = processField<float, double>(jobj, key, axis.stepAngle);
    } else if (strcmp("sd", key) == 0 || strcmp("sd", key + 1) == 0) {
        status = processField<DelayMics, int32_t>(jobj, key, axis.searchDelay);
    } else if (strcmp("tm", key) == 0 || strcmp("tm", key + 1) == 0) {
        status = processField<StepCoord, int32_t>(jobj, key, axis.travelMax);
    } else if (strcmp("tn", key) == 0 || strcmp("tn", key + 1) == 0) {
        status = processField<StepCoord, int32_t>(jobj, key, axis.travelMin);
    } else if (strcmp("ud", key) == 0 || strcmp("ud", key + 1) == 0) {
        status = processField<DelayMics, int32_t>(jobj, key, axis.usDelay);
    } else {
        return jcmd.setError(STATUS_UNRECOGNIZED_NAME, key);
    }
    return status;
}

typedef class PHSelfTest {
    private:
        int32_t nLoops;
        StepCoord pulses;
        int16_t nSegs;
        Machine &machine;

    private:
        Status execute(JsonCommand& jcmd, JsonObject& jobj);

    public:
        PHSelfTest(Machine& machine)
            : nLoops(0), pulses(6400), nSegs(0), machine(machine)
        {}

        Status process(JsonCommand& jcmd, JsonObject& jobj, const char* key);
} PHSelfTest;

Status PHSelfTest::execute(JsonCommand &jcmd, JsonObject& jobj) {
    int16_t minSegs = nSegs ? nSegs : 0; //max(10, min(STROKE_SEGMENTS-1,abs(pulses)/100));
    int16_t maxSegs = nSegs ? nSegs : 0; // STROKE_SEGMENTS-1;
    if (maxSegs >= STROKE_SEGMENTS) {
        return jcmd.setError(STATUS_STROKE_MAXLEN, "sg");
    }
    StrokeBuilder sb(machine.vMax, machine.tvMax, minSegs, maxSegs);
    if (pulses >= 0) {
        machine.setMotorPosition(Quad<StepCoord>());
    } else {
        machine.setMotorPosition(Quad<StepCoord>(
                                     machine.getMotorAxis(0).isEnabled() ? -pulses : 0,
                                     machine.getMotorAxis(1).isEnabled() ? -pulses : 0,
                                     machine.getMotorAxis(2).isEnabled() ? -pulses : 0,
                                     machine.getMotorAxis(3).isEnabled() ? -pulses : 0));
    }
    Status status = sb.buildLine(machine.stroke, Quad<StepCoord>(
                                     machine.getMotorAxis(0).isEnabled() ? pulses : 0,
                                     machine.getMotorAxis(1).isEnabled() ? pulses : 0,
                                     machine.getMotorAxis(2).isEnabled() ? pulses : 0,
                                     machine.getMotorAxis(3).isEnabled() ? pulses : 0));
    if (status != STATUS_OK) {
        return status;
    }
    Ticks tStart = ticks();
    status = machine.stroke.start(tStart);
    switch (status) {
    case STATUS_OK:
        break;
    case STATUS_STROKE_TIME:
        return jcmd.setError(status, "tv");
    default:
        return status;
    }
#ifdef TEST
    cout << "PHSelfTest::execute() pulses:" << pulses
         << " pos:" << machine.getMotorPosition().toString() << endl;
#endif
    do {
        nLoops++;
        status =  machine.stroke.traverse(ticks(), machine);
#ifdef TEST
        if (nLoops % 500 == 0) {
            cout << "PHSelfTest:execute()"
                 << " t:"
                 << (threadClock.ticks - machine.stroke.tStart) /
                 (float) machine.stroke.get_dtTotal()
                 << " pos:"
                 << machine.getMotorPosition().toString() << endl;
        }
#endif
    } while (status == STATUS_BUSY_MOVING);
#ifdef TEST
    cout << "PHSelfTest::execute() pos:" << machine.getMotorPosition().toString()
         << " status:" << status << endl;
#endif
    if (status == STATUS_OK) {
        status = STATUS_BUSY_MOVING; // repeat indefinitely
    }
    Ticks tElapsed = ticks() - tStart;

    float ts = tElapsed / (float) TICKS_PER_SECOND;
    float tp = machine.stroke.getTimePlanned();
    jobj["lp"] = nLoops;
    jobj["pp"].set(machine.stroke.vPeak * (machine.stroke.length / ts), 1);
    jobj["sg"] = machine.stroke.length;
    jobj["ts"].set(ts, 3);
    jobj["tp"].set(tp, 3);

    return status;
}

Status PHSelfTest::process(JsonCommand& jcmd, JsonObject& jobj, const char* key) {
    Status status = STATUS_OK;
    const char *s;

    if (strcmp("tstph", key) == 0) {
        if ((s = jobj[key]) && *s == 0) {
            JsonObject& node = jobj.createNestedObject(key);
            node["lp"] = "";
            node["mv"] = "";
            node["pp"] = "";
            node["pu"] = "";
            node["sg"] = "";
            node["ts"] = "";
            node["tp"] = "";
            node["tv"] = "";
        }
        JsonObject& kidObj = jobj[key];
        if (!kidObj.success()) {
            return jcmd.setError(STATUS_JSON_OBJECT, key);
        }
        for (JsonObject::iterator it = kidObj.begin(); it != kidObj.end(); ++it) {
            status = process(jcmd, kidObj, it->key);
            if (status != STATUS_OK) {
#ifdef TEST
                cout << "PHSelfTest::process() status:" << status << endl;
#endif
                return status;
            }
        }
        status = execute(jcmd, kidObj);
        if (status == STATUS_BUSY_MOVING) {
            pulses = -pulses; //reverse direction
            status = execute(jcmd, kidObj);
        }
    } else if (strcmp("lp", key) == 0) {
        // output variable
    } else if (strcmp("mv", key) == 0) {
        status = processField<int32_t, int32_t>(jobj, key, machine.vMax);
    } else if (strcmp("pp", key) == 0) {
        // output variable
    } else if (strcmp("pu", key) == 0) {
        status = processField<StepCoord, int32_t>(jobj, key, pulses);
    } else if (strcmp("sg", key) == 0) {
        status = processField<int16_t, int32_t>(jobj, key, nSegs);
    } else if (strcmp("ts", key) == 0) {
        // output variable
    } else if (strcmp("tp", key) == 0) {
        // output variable
    } else if (strcmp("tv", key) == 0) {
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, machine.tvMax);
    } else {
        return jcmd.setError(STATUS_UNRECOGNIZED_NAME, key);
    }
    return status;
}

typedef class PHMoveTo {
    private:
        int32_t nLoops;
        Quad<StepCoord> destination;
        int16_t nSegs;
        Machine &machine;

    private:
        Status execute(JsonCommand& jcmd, JsonObject *pjobj);

    public:
        PHMoveTo(Machine& machine)
            : nLoops(0), nSegs(0), machine(machine) {
            destination = machine.getMotorPosition();
        }

        Status process(JsonCommand& jcmd, JsonObject& jobj, const char* key);
} PHMoveTo;

Status PHMoveTo::execute(JsonCommand &jcmd, JsonObject *pjobj) {
    StrokeBuilder sb(machine.vMax, machine.tvMax);
    Quad<StepCoord> curPos = machine.getMotorPosition();
    Quad<StepCoord> dPos = destination - curPos;
    for (QuadIndex i = 0; i < QUAD_ELEMENTS; i++) {
        if (!machine.getMotorAxis(i).isEnabled()) {
            dPos.value[i] = 0;
        }
    }
    Status status = STATUS_OK;
    float tp = 0;
    float ts = 0;
    float pp = 0;
    int16_t sg = 0;
    if (!dPos.isZero()) {
        status = sb.buildLine(machine.stroke, dPos);
        if (status != STATUS_OK) {
            return status;
        }
        Ticks tStrokeStart = ticks();
        status = machine.stroke.start(tStrokeStart);
        switch (status) {
        case STATUS_OK:
            break;
        case STATUS_STROKE_TIME:
            return jcmd.setError(status, "tv");
        default:
            return status;
        }
        do {
            nLoops++;
            status = machine.stroke.traverse(ticks(), machine);
        } while (status == STATUS_BUSY_MOVING);
        tp = machine.stroke.getTimePlanned();
        ts = (ticks() - tStrokeStart) / (float) TICKS_PER_SECOND;
        pp = machine.stroke.vPeak * (machine.stroke.length / ts);
        sg = machine.stroke.length;
    }

    if (pjobj) {
        (*pjobj)["lp"] = nLoops;
        (*pjobj)["pp"].set(pp, 1);
        (*pjobj)["sg"] = sg;
        (*pjobj)["ts"].set(ts, 3);
        (*pjobj)["tp"].set(tp, 3);
    }

    return status;
}

Status PHMoveTo::process(JsonCommand& jcmd, JsonObject& jobj, const char* key) {
    Status status = STATUS_OK;
    const char *s;

    if (strcmp("mov", key) == 0) {
        if ((s = jobj[key]) && *s == 0) {
            JsonObject& node = jobj.createNestedObject(key);
            node["lp"] = "";
            node["mv"] = "";
            node["pp"] = "";
            node["sg"] = "";
            node["ts"] = "";
            node["tp"] = "";
            if (machine.getMotorAxis(0).isEnabled()) {
                node["1"] = "";
            }
            if (machine.getMotorAxis(1).isEnabled()) {
                node["2"] = "";
            }
            if (machine.getMotorAxis(2).isEnabled()) {
                node["3"] = "";
            }
            if (machine.getMotorAxis(3).isEnabled()) {
                node["4"] = "";
            }
        }
        JsonObject& kidObj = jobj[key];
        if (!kidObj.success()) {
            return jcmd.setError(STATUS_JSON_OBJECT, key);
        }
        for (JsonObject::iterator it = kidObj.begin(); it != kidObj.end(); ++it) {
            status = process(jcmd, kidObj, it->key);
            if (status != STATUS_OK) {
                TESTCOUT1("PHMoveTo::process() status:", status);
                return status;
            }
        }
        status = execute(jcmd, &kidObj);
    } else if (strncmp("mov", key, 3) == 0) { // short form
        MotorIndex iMotor = machine.motorOfName(key + strlen(key) - 1);
        if (iMotor == INDEX_NONE) {
            TESTCOUT1("STATUS_NO_MOTOR: ", key);
            return jcmd.setError(STATUS_NO_MOTOR, key);
        }
        status = processField<StepCoord, int32_t>(jobj, key, destination.value[iMotor]);
        if (status == STATUS_OK) {
            status = execute(jcmd, NULL);
        }
    } else if (strcmp("lp", key) == 0) {
        // output variable
    } else if (strcmp("mv", key) == 0) {
        status = processField<int32_t, int32_t>(jobj, key, machine.vMax);
    } else if (strcmp("pp", key) == 0) {
        // output variable
    } else if (strcmp("sg", key) == 0) {
        status = processField<int16_t, int32_t>(jobj, key, nSegs);
    } else if (strcmp("ts", key) == 0) {
        // output variable
    } else if (strcmp("tp", key) == 0) {
        // output variable
    } else if (strcmp("tv", key) == 0) {
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, machine.tvMax);
    } else {
        MotorIndex iMotor = machine.motorOfName(key);
        if (iMotor == INDEX_NONE) {
            TESTCOUT1("STATUS_NO_MOTOR: ", key);
            return jcmd.setError(STATUS_NO_MOTOR, key);
        }
        status = processField<StepCoord, int32_t>(jobj, key, destination.value[iMotor]);
    }
    return status;
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
            for (MotorIndex i = 0; i < 4; i++) {
                if (jarr[i].success()) {
                    Axis &a = machine.getMotorAxis(i);
                    int16_t revs = jarr[i];
                    int16_t revSteps = 360 / a.stepAngle;
                    int16_t revMicrosteps = revSteps * a.microsteps;
                    int16_t msRev = (a.usDelay * revMicrosteps) / 1000;
                    steps.value[i] = revs * revMicrosteps;
                }
            }
            Quad<StepCoord> steps1(steps);
            status = machine.pulse(steps1);
            if (status == STATUS_OK) {
                delay(250);
                Quad<StepCoord> steps2(steps.absoluteValue());
                status = machine.pulse(steps2);
                delay(250);
            }
            if (status == STATUS_OK) {
                status = STATUS_BUSY_MOVING;
            }
        } else if (strcmp("sp", key) == 0 || strcmp("tstsp", key) == 0) {
            // step pulses
            JsonArray &jarr = jobj[key];
            if (!jarr.success()) {
                return jcmd.setError(STATUS_FIELD_ARRAY_ERROR, key);
            }
            Quad<StepCoord> steps;
            for (MotorIndex i = 0; i < 4; i++) {
                if (jarr[i].success()) {
                    steps.value[i] = jarr[i];
                }
            }
            status = machine.pulse(steps);
        } else if (strcmp("ph", key) == 0 || strcmp("tstph", key) == 0) {
            return PHSelfTest(machine).process(jcmd, jobj, key);
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
            node["ee"] = "";
            node["fr"] = "";
            node["jp"] = "";
            node["lh"] = "";
            node["lp"] = "";
            node["pc"] = "";
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
    } else if (strcmp("ee", key) == 0 || strcmp("sysee", key) == 0) {
#ifdef TEST
		const char *s = jobj[key];
		if (!s) {
			return jcmd.setError(STATUS_JSON_STRING, key);
		}
		if (*s == 0) { // query
			uint8_t c = eeprom_read_byte((uint8_t*) 0);
			if (c == '{' || c == '[') {
				char *buf = jcmd.allocate(EEPROM_BYTES);
				if (!buf) {
					return jcmd.setError(STATUS_JSON_MEM, key);
				}
				for (int16_t i=0; i<EEPROM_BYTES; i++) {
					c = eeprom_read_byte((uint8_t*) (size_t) i);
					if (c == 255 || c == 0) {
						buf[i] = 0;
						break;
					} 
					buf[i] = c;
				}
				jobj[key] = buf;
			}
		} else {
			int16_t len = strlen(s) + 1;
			if (len >= EEPROM_BYTES) {
				return jcmd.setError(STATUS_JSON_EEPROM, key);
			}
			for (int16_t i=0; i<len; i++) {
				eeprom_write_byte((uint8_t*) (size_t) i, s[i]);
				TESTCOUT2("EEPROM[", i, "]:", (char) eeprom_read_byte((uint8_t *) (size_t) i));
			}
		}
#endif
    } else if (strcmp("fr", key) == 0 || strcmp("sysfr", key) == 0) {
        leastFreeRam = min(leastFreeRam, freeRam());
        jobj[key] = leastFreeRam;
    } else if (strcmp("jp", key) == 0 || strcmp("sysjp", key) == 0) {
        status = processField<bool, bool>(jobj, key, machine.jsonPrettyPrint);
    } else if (strcmp("pc", key) == 0 || strcmp("syspc", key) == 0) {
        PinConfig pc = machine.getPinConfig();
        status = processField<PinConfig, int32_t>(jobj, key, pc);
        const char *s;
        if ((s = jobj.at(key)) && *s == 0) { // query
            // do nothing
        } else {
            machine.setPinConfig(pc);
        }
    } else if (strcmp("lh", key) == 0 || strcmp("syslh", key) == 0) {
        status = processField<bool, bool>(jobj, key, machine.invertLim);
    } else if (strcmp("lp", key) == 0 || strcmp("syslp", key) == 0) {
        status = processField<int32_t, int32_t>(jobj, key, nLoops);
    } else if (strcmp("tc", key) == 0 || strcmp("systc", key) == 0) {
        jobj[key] = threadClock.ticks;
    } else if (strcmp("v", key) == 0 || strcmp("sysv", key) == 0) {
        jobj[key] = VERSION_MAJOR * 100 + VERSION_MINOR + VERSION_PATCH / 100.0;
    } else {
        return jcmd.setError(STATUS_UNRECOGNIZED_NAME, key);
    }
    return status;
}

Status JsonController::initializeHome(JsonCommand& jcmd, JsonObject& jobj, const char* key, bool clear) {
    Status status = STATUS_OK;
	if (clear) {
		for (QuadIndex i = 0; i < QUAD_ELEMENTS; i++) {
			machine.getMotorAxis(i).homing = false;
		}
	}
    if (strcmp("hom", key) == 0) {
        const char *s;
        if ((s = jobj[key]) && *s == 0) {
            JsonObject& node = jobj.createNestedObject(key);
            node["1"] = "";
            node["2"] = "";
            node["3"] = "";
            node["4"] = "";
        }
        JsonObject& kidObj = jobj[key];
        if (kidObj.success()) {
            for (JsonObject::iterator it = kidObj.begin(); it != kidObj.end(); ++it) {
                status = initializeHome(jcmd, kidObj, it->key, false);
                if (status != STATUS_BUSY_MOVING) {
                    return status;
                }
            }
        }
    } else {
        MotorIndex iMotor = machine.motorOfName(key + (strlen(key) - 1));
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
        status = initializeHome(jcmd, jobj, key, true);
    } else if (status == STATUS_BUSY_MOVING) {
        status = machine.home();
    } else {
        return jcmd.setError(STATUS_STATE, key);
    }
    return status;
}

Status JsonController::processIOPin(JsonCommand& jcmd, JsonObject& jobj, const char* key) {
	const char *pinStr = *key == 'd' || *key == 'a' ? key+1 : key+3;
	long pinLong = strtol(pinStr, NULL, 10);
	if (machine.isCorePin(pinLong)) {
		return jcmd.setError(STATUS_CORE_PIN, key);
	}
	if (pinLong < 0 || MAX_PIN < pinLong) {
		return jcmd.setError(STATUS_NO_SUCH_PIN, key);
	}
	int16_t pin = (int16_t) pinLong;
	const char *s = jobj[key];
	bool isAnalog = *key == 'a' || strncmp("ioa",key,3)==0;
	if (s && *s == 0) { // read
		if (isAnalog) {
			pinMode(pin+A0, INPUT);
			jobj[key] = analogRead(pin+A0);
		} else {
			pinMode(pin, INPUT);
			jobj[key] = (bool) digitalRead(pin);
		}
	} else if (isAnalog) {
		if (jobj[key].is<long>()) { // write
			long value = jobj[key];
			if (value < 0 || 255 < value) {
				return jcmd.setError(STATUS_JSON_255, key);
			}
			pinMode(pin+A0, OUTPUT);
			analogWrite(pin+A0, (int16_t) value);
		} else {
			return jcmd.setError(STATUS_JSON_255, key);
		}
	} else {
		if (jobj[key].is<bool>()) { // write
			bool value = jobj[key];
			pinMode(pin, OUTPUT);
			digitalWrite(pin, value);
		} else if (jobj[key].is<long>()) { // write
			bool value = (bool) (long)jobj[key];
			pinMode(pin, OUTPUT);
			digitalWrite(pin, value);
		} else {
			return jcmd.setError(STATUS_JSON_BOOL, key);
		}
	}
	return STATUS_OK;
}

Status JsonController::processIO(JsonCommand& jcmd, JsonObject& jobj, const char* key) {
	Status status = STATUS_NOT_IMPLEMENTED;
    if (strcmp("io", key) == 0) {
        const char *s;
        if ((s = jobj[key]) && *s == 0) {
            JsonObject& node = jobj.createNestedObject(key);
            node["1"] = "";
            node["2"] = "";
            node["3"] = "";
            node["4"] = "";
        }
        JsonObject& kidObj = jobj[key];
		if (!kidObj.success()) {
			return jcmd.setError(STATUS_IO_OBJ, key);
		}
		for (JsonObject::iterator it = kidObj.begin(); it != kidObj.end(); ++it) {
			status = processIO(jcmd, kidObj, it->key);
		}
    } else if (strncmp("d",key,1)==0 || strncmp("iod",key,3)==0) {
		status = processIOPin(jcmd, jobj, key);
    } else if (strncmp("a",key,1)==0 || strncmp("ioa",key,3)==0) {
		status = processIOPin(jcmd, jobj, key);
	} else {
		return jcmd.setError(STATUS_UNRECOGNIZED_NAME, key);
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
        status = processField<uint8_t, int32_t>(jobj, key, machine.pDisplay->cameraB);
    } else if (strcmp("cg", key) == 0 || strcmp("dpycg", key) == 0) {
        status = processField<uint8_t, int32_t>(jobj, key, machine.pDisplay->cameraG);
    } else if (strcmp("cr", key) == 0 || strcmp("dpycr", key) == 0) {
        status = processField<uint8_t, int32_t>(jobj, key, machine.pDisplay->cameraR);
    } else if (strcmp("dl", key) == 0 || strcmp("dpydl", key) == 0) {
        status = processField<uint8_t, int32_t>(jobj, key, machine.pDisplay->level);
    } else if (strcmp("ds", key) == 0 || strcmp("dpyds", key) == 0) {
        const char *s;
        bool isAssignment = (!(s = jobj[key]) || *s != 0);
        status = processField<uint8_t, int32_t>(jobj, key, machine.pDisplay->status);
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

Status JsonController::processObj(JsonCommand& jcmd, JsonObject&jobj) {
    JsonVariant node;
    node = jobj;
    Status status = STATUS_OK;

    for (JsonObject::iterator it = jobj.begin(); status >= 0 && it != jobj.end(); ++it) {
        if (strcmp("dvs", it->key) == 0) {
            status = processStroke(jcmd, jobj, it->key);
        } else if (strncmp("mov", it->key, 3) == 0) {
            status = PHMoveTo(machine).process(jcmd, jobj, it->key);
        } else if (strncmp("hom", it->key, 3) == 0) {
            status = processHome(jcmd, jobj, it->key);
        } else if (strncmp("tst", it->key, 3) == 0) {
            status = processTest(jcmd, jobj, it->key);
        } else if (strncmp("sys", it->key, 3) == 0) {
            status = processSys(jcmd, jobj, it->key);
        } else if (strncmp("dpy", it->key, 3) == 0) {
            status = processDisplay(jcmd, jobj, it->key);
        } else if (strncmp("mpo", it->key, 3) == 0) {
            status = processStepperPosition(jcmd, jobj, it->key);
        } else if (strncmp("io", it->key, 2) == 0) {
            status = processIO(jcmd, jobj, it->key);
        } else {
            switch (it->key[0]) {
            case '1':
            case '2':
            case '3':
            case '4':
                status = processMotor(jcmd, jobj, it->key, it->key[0]);
                break;
            case 'x':
            case 'y':
            case 'z':
            case 'a':
            case 'b':
            case 'c':
                status = processAxis(jcmd, jobj, it->key, it->key[0]);
                break;
            default:
                status = jcmd.setError(STATUS_UNRECOGNIZED_NAME, it->key);
                break;
            }
        }
    }

    return status;
}

Status JsonController::process(JsonCommand& jcmd) {
	Status status = STATUS_OK;
	JsonVariant &jroot = jcmd.requestRoot();

	if (jroot.is<JsonObject&>()) {
		JsonObject& jobj = jroot;
		status = processObj(jcmd, jobj);
	} else if (jroot.is<JsonArray&>()) {
		JsonArray& jarr = jroot;
		if (jcmd.cmdIndex < jarr.size()) {
			JsonObject& jobj = jarr[jcmd.cmdIndex];
			jcmd.jResponseRoot["r"] = jobj;
			status = processObj(jcmd, jobj);
			TESTCOUT3("JsonController::process(", (int) jcmd.cmdIndex+1, " of ", jarr.size(), ") status:", status);
			if (status == STATUS_OK) {
				status = STATUS_BUSY_OK;
				jcmd.cmdIndex++;
			}
		} else {
			status = STATUS_OK;
		}
	} else {
		status = STATUS_JSON_CMD;
	}

    jcmd.setStatus(status);

    if (!isProcessing(status)) {
        sendResponse(jcmd);
    }

    return status;
}


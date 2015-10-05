#include "Arduino.h"
#ifdef CMAKE
#include <cstring>
#include <cstdio>
#endif
#include "version.h"
#include "JsonController.h"
#include "ProcessField.h"
#include "ProgMem.h"
#include "git_tag.h"

using namespace firestep;

JsonController::JsonController(Machine& machine)
    : machine(machine) {
}

JsonController& JsonController::operator=(JsonController& that) {
    this->machine = that.machine;
    return *this;
}

AxisIndex JsonController::axisOf(char c) {
    switch (c) {
    default:
        return -1;
    case '1':
    case '2':
    case '3':
    case '4':
        return machine.motor[c-'1'];
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

inline int8_t hexValue(char c) {
    switch (c) {
    case '0':
        return 0;
    case '1':
        return 1;
    case '2':
        return 2;
    case '3':
        return 3;
    case '4':
        return 4;
    case '5':
        return 5;
    case '6':
        return 6;
    case '7':
        return 7;
    case '8':
        return 8;
    case '9':
        return 9;
    case 'A':
    case 'a':
        return 0xa;
    case 'B':
    case 'b':
        return 0xb;
    case 'C':
    case 'c':
        return 0xc;
    case 'D':
    case 'd':
        return 0xd;
    case 'E':
    case 'e':
        return 0xe;
    case 'F':
    case 'f':
        return 0xf;
    default:
        return -1;
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
        if (strcmp_PS(OP_us, it->key) == 0) {
            int32_t planMicros;
            status = processField<int32_t, int32_t>(stroke, it->key, planMicros);
            if (status != STATUS_OK) {
                return jcmd.setError(status, it->key);
            }
            float seconds = (float) planMicros / 1000000.0;
            machine.stroke.setTimePlanned(seconds);
            us_ok = true;
        } else if (strcmp_PS(OP_dp, it->key) == 0) {
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
        } else if (strcmp_PS(OP_sc, it->key) == 0) {
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
            jcmd.addQueryAttr(node, OP_ma);
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
    } else if (strcmp_PS(OP_ma, key) == 0 || strcmp_PS(OP_ma, key + 1) == 0) {
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

Status JsonController::process_id(JsonCommand &jcmd, JsonObject& jobj, const char* key) {
    Status status = STATUS_OK;
    const char *s;
    if (strcmp_PS(OP_id, key) == 0) {
        if ((s = jobj[key]) && *s == 0) {
            JsonObject& node = jobj.createNestedObject(key);
            jcmd.addQueryAttr(node, OP_app);
            jcmd.addQueryAttr(node, OP_ch);
            jcmd.addQueryAttr(node, OP_git);
            jcmd.addQueryAttr(node, OP_ver);
        }
        JsonObject& kidObj = jobj[key];
        if (kidObj.success()) {
            for (JsonObject::iterator it = kidObj.begin(); it != kidObj.end(); ++it) {
                status = process_id(jcmd, kidObj, it->key);
                if (status != STATUS_OK) {
                    return status;
                }
            }
        }
    } else if (strcmp_PS(OP_app, key) == 0 || strcmp_PS(OP_app, key + 2) == 0) {
		jobj[key] =  "FireStep";
    } else if (strcmp_PS(OP_ch, key) == 0 || strcmp_PS(OP_ch, key + 2) == 0) {
        int32_t curHash = machine.hash();
        int32_t jsonHash = curHash;
        status = processField<int32_t, int32_t>(jobj, key, jsonHash);
        if (jsonHash != curHash) {
            machine.syncHash = jsonHash;
        }
    } else if (strcmp_PS(OP_git, key) == 0 || strcmp_PS(OP_git, key + 2) == 0) {
		char *buf = jcmd.allocate(41);
		strcpy_P(buf, GIT_TAG);
		jobj[key] = buf;
    } else if (strcmp_PS(OP_ver, key) == 0 || strcmp_PS(OP_ver, key + 2) == 0) {
		jobj[key] = VERSION_MAJOR + VERSION_MINOR/100.0 + VERSION_PATCH/1000.0;
	} else {
        return jcmd.setError(STATUS_UNRECOGNIZED_NAME, key);
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
            jcmd.addQueryAttr(node, OP_dh);
            jcmd.addQueryAttr(node, OP_en);
            jcmd.addQueryAttr(node, OP_ho);
            jcmd.addQueryAttr(node, OP_is);
            jcmd.addQueryAttr(node, OP_lb);
            jcmd.addQueryAttr(node, OP_lm);
            jcmd.addQueryAttr(node, OP_ln);
            jcmd.addQueryAttr(node, OP_mi);
            jcmd.addQueryAttr(node, OP_pd);
            jcmd.addQueryAttr(node, OP_pe);
            jcmd.addQueryAttr(node, OP_pm);
            jcmd.addQueryAttr(node, OP_pn);
            jcmd.addQueryAttr(node, OP_po);
            jcmd.addQueryAttr(node, OP_ps);
            jcmd.addQueryAttr(node, OP_sa);
            jcmd.addQueryAttr(node, OP_tm);
            jcmd.addQueryAttr(node, OP_tn);
            jcmd.addQueryAttr(node, OP_ud);
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
    } else if (strcmp_PS(OP_en, key) == 0 || strcmp_PS(OP_en, key + 1) == 0) {
        bool active = axis.isEnabled();
        status = processField<bool, bool>(jobj, key, active);
        if (status == STATUS_OK) {
            axis.enable(active);
            status = (jobj[key] = axis.isEnabled()).success() ? status : STATUS_FIELD_ERROR;
        }
    } else if (strcmp_PS(OP_dh, key) == 0 || strcmp_PS(OP_dh, key + 1) == 0) {
        status = processField<bool, bool>(jobj, key, axis.dirHIGH);
        if (axis.pinDir != NOPIN && status == STATUS_OK) {	// force setting of direction bit in case meaning changed
            axis.setAdvancing(false);
            axis.setAdvancing(true);
        }
    } else if (strcmp_PS(OP_ho, key) == 0 || strcmp_PS(OP_ho, key + 1) == 0) {
        status = processField<StepCoord, int32_t>(jobj, key, axis.home);
    } else if (strcmp_PS(OP_is, key) == 0 || strcmp_PS(OP_is, key + 1) == 0) {
        status = processField<DelayMics, int32_t>(jobj, key, axis.idleSnooze);
    } else if (strcmp_PS(OP_lb, key) == 0 || strcmp_PS(OP_lb, key + 1) == 0) {
        status = processField<StepCoord, int32_t>(jobj, key, axis.latchBackoff);
    } else if (strcmp_PS(OP_lm, key) == 0 || strcmp_PS(OP_lm, key + 1) == 0) {
        axis.readAtMax(machine.invertLim);
        status = processField<bool, bool>(jobj, key, axis.atMax);
    } else if (strcmp_PS(OP_ln, key) == 0 || strcmp_PS(OP_ln, key + 1) == 0) {
        axis.readAtMin(machine.invertLim);
        status = processField<bool, bool>(jobj, key, axis.atMin);
    } else if (strcmp_PS(OP_mi, key) == 0 || strcmp_PS(OP_mi, key + 1) == 0) {
        status = processField<uint8_t, int32_t>(jobj, key, axis.microsteps);
        if (axis.microsteps < 1) {
            axis.microsteps = 1;
            return STATUS_JSON_POSITIVE1;
        }
    } else if (strcmp_PS(OP_pd, key) == 0 || strcmp_PS(OP_pd, key + 1) == 0) {
        status = processPin(jobj, key, axis.pinDir, OUTPUT);
    } else if (strcmp_PS(OP_pe, key) == 0 || strcmp_PS(OP_pe, key + 1) == 0) {
        status = processPin(jobj, key, axis.pinEnable, OUTPUT,
                            axis.isEnabled() ? PIN_ENABLE : PIN_DISABLE);
    } else if (strcmp_PS(OP_pm, key) == 0 || strcmp_PS(OP_pm, key + 1) == 0) {
        status = processPin(jobj, key, axis.pinMax, INPUT);
    } else if (strcmp_PS(OP_pn, key) == 0 || strcmp_PS(OP_pn, key + 1) == 0) {
        status = processPin(jobj, key, axis.pinMin, INPUT);
    } else if (strcmp_PS(OP_po, key) == 0 || strcmp_PS(OP_po, key + 1) == 0) {
        status = processField<StepCoord, int32_t>(jobj, key, axis.position);
    } else if (strcmp_PS(OP_ps, key) == 0 || strcmp_PS(OP_ps, key + 1) == 0) {
        status = processPin(jobj, key, axis.pinStep, OUTPUT);
    } else if (strcmp_PS(OP_sa, key) == 0 || strcmp_PS(OP_sa, key + 1) == 0) {
        status = processField<float, double>(jobj, key, axis.stepAngle);
    } else if (strcmp_PS(OP_tm, key) == 0 || strcmp_PS(OP_tm, key + 1) == 0) {
        status = processField<StepCoord, int32_t>(jobj, key, axis.travelMax);
    } else if (strcmp_PS(OP_tn, key) == 0 || strcmp_PS(OP_tn, key + 1) == 0) {
        status = processField<StepCoord, int32_t>(jobj, key, axis.travelMin);
    } else if (strcmp_PS(OP_ud, key) == 0 || strcmp_PS(OP_ud, key + 1) == 0) {
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
    int16_t minSegs = nSegs ? nSegs : 0; //maxval(10, minval(STROKE_SEGMENTS-1,absval(pulses)/100));
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
    jobj["tp"].set(tp, 3);
    jobj["ts"].set(ts, 3);

    return status;
}

Status PHSelfTest::process(JsonCommand& jcmd, JsonObject& jobj, const char* key) {
    Status status = STATUS_OK;
    const char *s;

    if (strcmp_PS(OP_tstph, key) == 0) {
        if ((s = jobj[key]) && *s == 0) {
            JsonObject& node = jobj.createNestedObject(key);
            jcmd.addQueryAttr(node, OP_lp);
            jcmd.addQueryAttr(node, OP_mv);
            jcmd.addQueryAttr(node, OP_pp);
            jcmd.addQueryAttr(node, OP_pu);
            jcmd.addQueryAttr(node, OP_sg);
            jcmd.addQueryAttr(node, OP_ts);
            jcmd.addQueryAttr(node, OP_tp);
            jcmd.addQueryAttr(node, OP_tv);
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
    } else if (strcmp_PS(OP_lp, key) == 0) {
        // output variable
    } else if (strcmp_PS(OP_mv, key) == 0) {
        status = processField<int32_t, int32_t>(jobj, key, machine.vMax);
    } else if (strcmp_PS(OP_pp, key) == 0) {
        // output variable
    } else if (strcmp_PS(OP_pu, key) == 0) {
        status = processField<StepCoord, int32_t>(jobj, key, pulses);
    } else if (strcmp_PS(OP_sg, key) == 0) {
        status = processField<int16_t, int32_t>(jobj, key, nSegs);
    } else if (strcmp_PS(OP_ts, key) == 0) {
        // output variable
    } else if (strcmp_PS(OP_tp, key) == 0) {
        // output variable
    } else if (strcmp_PS(OP_tv, key) == 0) {
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, machine.tvMax);
    } else {
        return jcmd.setError(STATUS_UNRECOGNIZED_NAME, key);
    }
    return status;
}

Status JsonController::processTest(JsonCommand& jcmd, JsonObject& jobj, const char* key) {
    Status status = jcmd.getStatus();

    switch (status) {
    case STATUS_BUSY_PARSED:
    case STATUS_BUSY_MOVING:
        if (strcmp_PS(OP_tst, key) == 0) {
            JsonObject& tst = jobj[key];
            if (!tst.success()) {
                return jcmd.setError(STATUS_JSON_OBJECT, key);
            }
            for (JsonObject::iterator it = tst.begin(); it != tst.end(); ++it) {
                status = processTest(jcmd, tst, it->key);
            }
        } else if (strcmp_PS(OP_rv, key) == 0 || strcmp_PS(OP_tstrv, key) == 0) { // revolution steps
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
        } else if (strcmp_PS(OP_sp, key) == 0 || strcmp_PS(OP_tstsp, key) == 0) {
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
        } else if (strcmp_PS(OP_ph, key) == 0 || strcmp_PS(OP_tstph, key) == 0) {
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
    if (strcmp_PS(OP_sys, key) == 0) {
        const char *s;
        if ((s = jobj[key]) && *s == 0) {
            JsonObject& node = jobj.createNestedObject(key);
            jcmd.addQueryAttr(node, OP_ah);
            jcmd.addQueryAttr(node, OP_as);
            //jcmd.addQueryAttr(node, OP_ch); // Deprecated
            jcmd.addQueryAttr(node, OP_eu);
            jcmd.addQueryAttr(node, OP_hp);
            jcmd.addQueryAttr(node, OP_jp);
            jcmd.addQueryAttr(node, OP_lh);
            jcmd.addQueryAttr(node, OP_mv);
            jcmd.addQueryAttr(node, OP_om);
            jcmd.addQueryAttr(node, OP_pb);
            jcmd.addQueryAttr(node, OP_pc);
            jcmd.addQueryAttr(node, OP_pi);
            jcmd.addQueryAttr(node, OP_sd);
            jcmd.addQueryAttr(node, OP_to);
            jcmd.addQueryAttr(node, OP_tv);
            jcmd.addQueryAttr(node, OP_v);
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
    } else if (strcmp_PS(OP_ah, key) == 0 || strcmp_PS(OP_sysah, key) == 0) {
        status = processField<bool, bool>(jobj, key, machine.autoHome);
    } else if (strcmp_PS(OP_as, key) == 0 || strcmp_PS(OP_sysas, key) == 0) {
        status = processField<bool, bool>(jobj, key, machine.autoSync);
    } else if (strcmp_PS(OP_ch, key) == 0 || strcmp_PS(OP_sysch, key) == 0) {
        int32_t curHash = machine.hash();
        int32_t jsonHash = curHash;
        status = processField<int32_t, int32_t>(jobj, key, jsonHash);
        if (jsonHash != curHash) {
            machine.syncHash = jsonHash;
        }
    } else if (strcmp_PS(OP_eu, key) == 0 || strcmp_PS(OP_syseu, key) == 0) {
        bool euExisting = machine.isEEUserEnabled();
        bool euNew = euExisting;
        status = processField<bool, bool>(jobj, key, euNew);
        if (euNew != euExisting) {
            machine.enableEEUser(euNew);
        }
    } else if (strcmp_PS(OP_db, key) == 0 || strcmp_PS(OP_sysdb, key) == 0) {
        status = processField<uint8_t, long>(jobj, key, machine.debounce);
    } else if (strcmp_PS(OP_fr, key) == 0 || strcmp_PS(OP_sysfr, key) == 0) {
        leastFreeRam = minval(leastFreeRam, freeRam());
        jobj[key] = leastFreeRam;
    } else if (strcmp_PS(OP_hp, key) == 0 || strcmp_PS(OP_syshp, key) == 0) {
        status = processField<int16_t, long>(jobj, key, machine.fastSearchPulses);
    } else if (strcmp_PS(OP_jp, key) == 0 || strcmp_PS(OP_sysjp, key) == 0) {
        status = processField<bool, bool>(jobj, key, machine.jsonPrettyPrint);
    } else if (strcmp_PS(OP_lb, key) == 0 || strcmp_PS(OP_lb, key + 1) == 0) {
        StepCoord latchBackoff = 0;
        for (AxisIndex i=0; i<AXIS_COUNT; i++) {
            latchBackoff = maxval(latchBackoff, machine.axis[i].latchBackoff);
        }
        status = processField<StepCoord, int32_t>(jobj, key, latchBackoff);
        for (AxisIndex i=0; i<AXIS_COUNT; i++) {
            machine.axis[i].latchBackoff = latchBackoff;
        }
    } else if (strcmp_PS(OP_lh, key) == 0 || strcmp_PS(OP_syslh, key) == 0) {
        status = processField<bool, bool>(jobj, key, machine.invertLim);
    } else if (strcmp_PS(OP_lp, key) == 0 || strcmp_PS(OP_syslp, key) == 0) {
        status = processField<int32_t, int32_t>(jobj, key, nLoops);
    } else if (strcmp_PS(OP_mv, key) == 0 || strcmp_PS(OP_sysmv, key) == 0) {
        status = processField<int32_t, int32_t>(jobj, key, machine.vMax);
    } else if (strcmp_PS(OP_om, key) == 0 || strcmp_PS(OP_sysom, key) == 0) {
        status = processField<OutputMode, int32_t>(jobj, key, machine.outputMode);
    } else if (strcmp_PS(OP_pc, key) == 0 || strcmp_PS(OP_syspc, key) == 0) {
        PinConfig pc = machine.getPinConfig();
        status = processField<PinConfig, int32_t>(jobj, key, pc);
        if (pc != machine.getPinConfig()) {
            machine.setPinConfig(pc);
        }
    } else if (strcmp_PS(OP_pb, key) == 0 || strcmp_PS(OP_syspb, key) == 0) {
        status = processPin(jobj, key, machine.op.probe.pinProbe, INPUT);
    } else if (strcmp_PS(OP_pi, key) == 0 || strcmp_PS(OP_syspi, key) == 0) {
        PinType pinStatus = machine.pinStatus;
        status = processField<PinType, int32_t>(jobj, key, pinStatus);
        if (pinStatus != machine.pinStatus) {
            machine.pinStatus = pinStatus;
            machine.pDisplay->setup(pinStatus);
        }
    } else if (strcmp_PS(OP_sd, key) == 0 || strcmp_PS(OP_syssd, key) == 0) {
        status = processField<DelayMics, int32_t>(jobj, key, machine.searchDelay);
    } else if (strcmp_PS(OP_to, key) == 0 || strcmp_PS(OP_systo, key) == 0) {
        status = processField<Topology, int32_t>(jobj, key, machine.topology);
    } else if (strcmp_PS(OP_tc, key) == 0 || strcmp_PS(OP_systc, key) == 0) {
        jobj[key] = threadClock.ticks;
    } else if (strcmp_PS(OP_tv, key) == 0 || strcmp_PS(OP_systv, key) == 0) {
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, machine.tvMax);
    } else if (strcmp_PS(OP_v, key) == 0 || strcmp_PS(OP_sysv, key) == 0) {
        jobj.at(key).set(VERSION_MAJOR + VERSION_MINOR/100.0 + VERSION_PATCH/1000.0,3);
    } else {
        return jcmd.setError(STATUS_UNRECOGNIZED_NAME, key);
    }
    return status;
}

Status JsonController::processDebug(JsonCommand& jcmd, JsonObject& jobj, const char* key) {
    Status status = STATUS_OK;
    if (strcmp_PS(OP_dbg, key) == 0) {
        const char *s;
        if ((s = jobj[key]) && *s == 0) {
            JsonObject& node = jobj.createNestedObject(key);
            jcmd.addQueryAttr(node, OP_fr);
            jcmd.addQueryAttr(node, OP_lp);
            jcmd.addQueryAttr(node, OP_tc);
        }
        JsonObject& kidObj = jobj[key];
        if (kidObj.success()) {
            for (JsonObject::iterator it = kidObj.begin(); it != kidObj.end(); ++it) {
                status = processDebug(jcmd, kidObj, it->key);
                if (status != STATUS_OK) {
                    return status;
                }
            }
        }
    } else if (strcmp_PS(OP_fr, key) == 0 || strcmp_PS(OP_dbgfr, key) == 0) {
        leastFreeRam = minval(leastFreeRam, freeRam());
        jobj[key] = leastFreeRam;
    } else if (strcmp_PS(OP_lp, key) == 0 || strcmp_PS(OP_dbglp, key) == 0) {
        status = processField<int32_t, int32_t>(jobj, key, nLoops);
    } else if (strcmp_PS(OP_tc, key) == 0 || strcmp_PS(OP_dbgtc, key) == 0) {
        jobj[key] = threadClock.ticks;
    } else {
        return jcmd.setError(STATUS_UNRECOGNIZED_NAME, key);
    }
    return status;
}


Status JsonController::processEEPROMValue(JsonCommand& jcmd, JsonObject& jobj, const char* key, const char*addr) {
    Status status = STATUS_OK;
    JsonVariant &jvalue = jobj[key];
    if (addr && *addr == '!') {
        status = processObj(jcmd, jvalue);
        if (status != STATUS_OK) {
            return jcmd.setError(status, key);
        }
        TESTCOUT1("processEEPROMValue!:", addr);
        addr++;
    }
    if (!addr || *addr<'0' || '9'<*addr) {
        return STATUS_JSON_DIGIT;
    }
    long addrLong = strtol(addr, NULL, 10);
    if (addrLong<0 || EEPROM_END <= addrLong) {
        return STATUS_EEPROM_ADDR;
    }
    char buf[EEPROM_BYTES];
    buf[0] = 0;
    if (jvalue.is<JsonArray&>()) {
        JsonArray &jeep = jvalue;
        jeep.printTo(buf, EEPROM_BYTES);
    } else if (jvalue.is<JsonObject&>()) {
        JsonObject &jeep = jvalue;
        jeep.printTo(buf, EEPROM_BYTES);
    } else if (jvalue.is<const char *>()) {
        const char *s = jvalue;
        snprintf(buf, sizeof(buf), "%s", s);
    }
    if (!buf) {
        return STATUS_JSON_STRING;
    }
    if (buf[0] == 0) { // query
        uint8_t c = eeprom_read_byte((uint8_t*) addrLong);
        if (c && c != 255) {
            char *buf = jcmd.allocate(EEPROM_BYTES);
            if (!buf) {
                return jcmd.setError(STATUS_JSON_MEM3, key);
            }
            for (int16_t i=0; i<EEPROM_BYTES; i++) {
                c = eeprom_read_byte((uint8_t*) addrLong+i);
                if (c == 255 || c == 0) {
                    buf[i] = 0;
                    break;
                }
                buf[i] = c;
            }
            jobj[key] = buf;
        }
    } else {
        int16_t len = strlen(buf) + 1;
        if (len >= EEPROM_BYTES) {
            return jcmd.setError(STATUS_JSON_EEPROM, key);
        }
        for (int16_t i=0; i<len; i++) {
            eeprom_write_byte((uint8_t*)addrLong+i, buf[i]);
            TESTCOUT3("EEPROM[", ((int)addrLong+i), "]:",
                      (char) eeprom_read_byte((uint8_t *) addrLong+i),
                      " ",
                      (int) eeprom_read_byte((uint8_t *) addrLong+i)
                     );
        }
    }
    return status;
}

Status JsonController::processEEPROM(JsonCommand& jcmd, JsonObject& jobj, const char* key) {
    Status status = STATUS_OK;
    if (strcmp_PS(OP_eep, key) == 0) {
        JsonObject& kidObj = jobj[key];
        if (!kidObj.success()) {
            return jcmd.setError(STATUS_JSON_OBJECT, key);
        }
        for (JsonObject::iterator it = kidObj.begin(); status>=0 && it != kidObj.end(); ++it) {
            status = processEEPROMValue(jcmd, kidObj, it->key, it->key);
        }
    } else if (strncmp("eep",key,3) == 0) {
        status = processEEPROMValue(jcmd, jobj, key, key+3);
    } else {
        return jcmd.setError(status, key);
    }
    if (status < 0) {
        return jcmd.setError(status, key);
    }
    return status;
}

Status JsonController::processIOPin(JsonCommand& jcmd, JsonObject& jobj, const char* key, bool pullUp) {
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
            pinMode(pin+A0, pullUp ? INPUT_PULLUP : INPUT); // maybe INPUT_PULLUP should be error
            jobj[key] = analogRead(pin+A0);
        } else {
            pinMode(pin, pullUp ? INPUT_PULLUP : INPUT);
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

Status JsonController::processProgram(JsonCommand& jcmd, JsonObject& jobj, const char* key) {
    Status status = STATUS_NOT_IMPLEMENTED;
    if (strcmp_PS(OP_pgm, key) == 0) {
        JsonObject& kidObj = jobj[key];
        if (!kidObj.success()) {
            status = prog_dump("help");
        }
        for (JsonObject::iterator it = kidObj.begin(); it != kidObj.end(); ++it) {
            status = processProgram(jcmd, kidObj, it->key);
        }
    } else if (strcmp_PS(OP_d,key)==0 || strcmp_PS(OP_pgmd,key)==0) {
        const char * name = jobj[key];
        status = prog_dump(name);
		if (status < 0) {
			return jcmd.setError(status, name);
		}
    } else if (strcmp_PS(OP_x,key)==0 || strcmp_PS(OP_pgmx,key)==0) {
        const char * name = jobj[key];
        status = prog_load_cmd(name, jcmd);
		if (status < 0) {
			return jcmd.setError(status, name);
		}
    } else {
        return jcmd.setError(STATUS_UNRECOGNIZED_NAME, key);
    }
    return status;
}

Status JsonController::processIO(JsonCommand& jcmd, JsonObject& jobj, const char* key, bool pullUp) {
    Status status = STATUS_OK;
    if (strcmp_PS(OP_io, key) == 0) {
        JsonObject& kidObj = jobj[key];
        if (!kidObj.success()) {
            return jcmd.setError(STATUS_JSON_OBJECT, key);
        }
		char pu[10];
		strcpy_P(pu, OP_pu);
		if (kidObj.at(pu).success()) {
			pullUp = kidObj[pu];
			TESTCOUT1("iopu:", pullUp);
		}
        for (JsonObject::iterator it = kidObj.begin(); it != kidObj.end(); ++it) {
            status = processIO(jcmd, kidObj, it->key, pullUp);
			if (status < 0) {
				return status;
			}
        }
    } else if (strcmp_PS(OP_pu,key)==0) {
		// handled above
    } else if (strncmp("d",key,1)==0 || strncmp("iod",key,3)==0) {
        status = processIOPin(jcmd, jobj, key, pullUp);
    } else if (strncmp("a",key,1)==0 || strncmp("ioa",key,3)==0) {
        status = processIOPin(jcmd, jobj, key, pullUp);
    } else {
        return jcmd.setError(STATUS_UNRECOGNIZED_NAME, key);
    }
    return status;
}

Status JsonController::processCalibrate(JsonCommand& jcmd, JsonObject& jobj, const char* key) {
    return jcmd.setError(STATUS_TOPOLOGY_NAME, key);
}

Status JsonController::processProbeData(JsonCommand& jcmd, JsonObject& jobj, const char* key) {
    Status status = STATUS_OK;
    const char *s;
    if ((s = jobj[key]) && *s == 0) {
        JsonArray &jarr = jobj.createNestedArray(key);
        for (int16_t i=0; i<PROBE_DATA; i++) {
            jarr.add(machine.op.probe.probeData[i]);
        }
    } else if (jobj.at(key).is<JsonArray&>()) {
        JsonArray &jarr = jobj[key];
		for (int16_t i=0; i<PROBE_DATA; i++) {
			if (jarr[i].success()) {
				machine.op.probe.probeData[i] = jarr[i];
			}
		}
    } else {
        status = jcmd.setError(STATUS_OUTPUT_FIELD, key);
    }
    return status;
}

Status JsonController::processMark(JsonCommand& jcmd, JsonObject& jobj, const char* key) {
    Status status = STATUS_OK;
    size_t keyLen = strlen(key);
    if (strcmp_PS(OP_mrk, key) == 0) {
        const char *s;
        if ((s = jobj[key]) && *s == 0) {
            JsonObject& node = jobj.createNestedObject(key);
            jcmd.addQueryAttr(node, OP_m1);
            jcmd.addQueryAttr(node, OP_m2);
            jcmd.addQueryAttr(node, OP_m3);
            jcmd.addQueryAttr(node, OP_m4);
            jcmd.addQueryAttr(node, OP_m5);
            jcmd.addQueryAttr(node, OP_m6);
            jcmd.addQueryAttr(node, OP_m7);
            jcmd.addQueryAttr(node, OP_m8);
            jcmd.addQueryAttr(node, OP_m9);
        }
        JsonObject& kidObj = jobj[key];
        if (kidObj.success()) {
            for (JsonObject::iterator it = kidObj.begin(); it != kidObj.end(); ++it) {
                status = processMark(jcmd, kidObj, it->key);
                if (status != STATUS_OK) {
                    return status;
                }
            }
        }
    } else if (strcmp_PS(OP_m1, key) == 0 || strcmp_PS(OP_mrkm1, key) == 0) {
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, machine.marks[0]);
    } else if (strcmp_PS(OP_m2, key) == 0 || strcmp_PS(OP_mrkm2, key) == 0) {
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, machine.marks[1]);
    } else if (strcmp_PS(OP_m3, key) == 0 || strcmp_PS(OP_mrkm3, key) == 0) {
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, machine.marks[2]);
    } else if (strcmp_PS(OP_m4, key) == 0 || strcmp_PS(OP_mrkm4, key) == 0) {
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, machine.marks[3]);
    } else if (strcmp_PS(OP_m5, key) == 0 || strcmp_PS(OP_mrkm5, key) == 0) {
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, machine.marks[4]);
    } else if (strcmp_PS(OP_m6, key) == 0 || strcmp_PS(OP_mrkm6, key) == 0) {
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, machine.marks[5]);
    } else if (strcmp_PS(OP_m7, key) == 0 || strcmp_PS(OP_mrkm7, key) == 0) {
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, machine.marks[6]);
    } else if (strcmp_PS(OP_m8, key) == 0 || strcmp_PS(OP_mrkm8, key) == 0) {
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, machine.marks[7]);
    } else if (strcmp_PS(OP_m9, key) == 0 || strcmp_PS(OP_mrkm9, key) == 0) {
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, machine.marks[8]);
    } else if (keyLen >= 2 && (strncmp("a",key,1) == 0 || strncmp("mrka",key,4) == 0)) {
        AxisIndex iAxis = axisOf(key[keyLen-1]);
        if (iAxis < 0) {
            TESTCOUT1("axisOf:", key[keyLen-1]);
            return jcmd.setError(STATUS_MARK_AXIS, key);
        }
        int16_t iMark = ((int16_t)jobj[key]) - 1;
        if (iMark < 0 || MARK_COUNT <= iMark) {
            TESTCOUT1("mark index:", iMark);
            return jcmd.setError(STATUS_MARK_INDEX, key);
        }
        machine.marks[iMark] = machine.axis[iAxis].position;
    } else {
        return jcmd.setError(STATUS_UNRECOGNIZED_NAME, key);
    }
    return status;
}

Status JsonController::processDisplay(JsonCommand& jcmd, JsonObject& jobj, const char* key) {
    Status status = STATUS_OK;
    if (strcmp_PS(OP_dpy, key) == 0) {
        const char *s;
        if ((s = jobj[key]) && *s == 0) {
            JsonObject& node = jobj.createNestedObject(key);
            jcmd.addQueryAttr(node, OP_cb);
            jcmd.addQueryAttr(node, OP_cg);
            jcmd.addQueryAttr(node, OP_cr);
            jcmd.addQueryAttr(node, OP_dl);
            jcmd.addQueryAttr(node, OP_ds);
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
    } else if (strcmp_PS(OP_cb, key) == 0 || strcmp_PS(OP_dpycb, key) == 0) {
        status = processField<uint8_t, int32_t>(jobj, key, machine.pDisplay->cameraB);
    } else if (strcmp_PS(OP_cg, key) == 0 || strcmp_PS(OP_dpycg, key) == 0) {
        status = processField<uint8_t, int32_t>(jobj, key, machine.pDisplay->cameraG);
    } else if (strcmp_PS(OP_cr, key) == 0 || strcmp_PS(OP_dpycr, key) == 0) {
        status = processField<uint8_t, int32_t>(jobj, key, machine.pDisplay->cameraR);
    } else if (strcmp_PS(OP_dl, key) == 0 || strcmp_PS(OP_dpydl, key) == 0) {
        status = processField<uint8_t, int32_t>(jobj, key, machine.pDisplay->level);
    } else if (strcmp_PS(OP_ds, key) == 0 || strcmp_PS(OP_dpyds, key) == 0) {
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
    sendResponse(jcmd, cause);
    return STATUS_WAIT_CANCELLED;
}

void JsonController::sendResponse(JsonCommand &jcmd, Status status, bool final) {
    jcmd.setStatus(status);
    if (status >= 0) {
        if (jcmd.responseAvailable() < 1) {
            TESTCOUT2("response available:", jcmd.responseAvailable(), " capacity:", jcmd.responseCapacity());
            jcmd.setStatus(STATUS_JSON_MEM1);
        } else if (jcmd.requestAvailable() < 1) {
            TESTCOUT2("request available:", jcmd.requestAvailable(), " capacity:", jcmd.requestCapacity());
            jcmd.setStatus(STATUS_JSON_MEM2);
        }
    }
    if (machine.jsonPrettyPrint) {
        jcmd.response().prettyPrintTo(Serial);
    } else {
        jcmd.response().printTo(Serial);
    }
    jcmd.responseClear();
    if (final) {
        Serial.println(" "); // }-SPACE-LF marks final output before return to STATUS_WAIT_IDLE
    } else {
        Serial.println();
    }
}

Status JsonController::processObj(JsonCommand& jcmd, JsonObject&jobj) {
    JsonVariant node;
    node = jobj;
    Status status = STATUS_OK;
    //TESTCOUT1("processObj:", "in");

    for (JsonObject::iterator it = jobj.begin(); status >= 0 && it != jobj.end(); ++it) {
        //TESTCOUT1("processObj key:", it->key);
        if (strcmp_PS(OP_dvs, it->key) == 0) {
            status = processStroke(jcmd, jobj, it->key);
        } else if (strncmp("mov", it->key, 3) == 0) {
            status = processMove(jcmd, jobj, it->key);
        } else if (strncmp("hom", it->key, 3) == 0) {
            status = processHome(jcmd, jobj, it->key);
        } else if (strncmp("tst", it->key, 3) == 0) {
            status = processTest(jcmd, jobj, it->key);
        } else if (strncmp("sys", it->key, 3) == 0) {
            status = processSys(jcmd, jobj, it->key);
        } else if (strncmp("dbg", it->key, 3) == 0) {
            status = processDebug(jcmd, jobj, it->key);
        } else if (strncmp("dpy", it->key, 3) == 0) {
            status = processDisplay(jcmd, jobj, it->key);
        } else if (strncmp("mpo", it->key, 3) == 0) {
            status = processPosition(jcmd, jobj, it->key);
        } else if (strncmp("io", it->key, 2) == 0) {
            status = processIO(jcmd, jobj, it->key);
        } else if (strncmp("eep", it->key, 3) == 0) {
            status = processEEPROM(jcmd, jobj, it->key);
        } else if (strncmp("dim", it->key, 3) == 0) {
            status = processDimension(jcmd, jobj, it->key);
        } else if (strncmp("cal", it->key, 3) == 0) {
            status = processCalibrate(jcmd, jobj, it->key);
        } else if (strncmp("mrk", it->key, 3) == 0) {
            status = processMark(jcmd, jobj, it->key);
        } else if (strcmp_PS(OP_prbd, it->key) == 0) {
            status = processProbeData(jcmd, jobj, it->key);
        } else if (strncmp("prb", it->key, 3) == 0) {
            status = processProbe(jcmd, jobj, it->key);
        } else if (strncmp("prb", it->key, 3) == 0) {
            status = processProbe(jcmd, jobj, it->key);
        } else if (strcmp_PS(OP_idl, it->key) == 0) {
            int16_t ms = it->value;
            delay(ms);
        } else if (strcmp_PS(OP_cmt, it->key) == 0) {
            if (OUTPUT_CMT==(machine.outputMode&OUTPUT_CMT)) {
                const char *s = it->value;
                Serial.println(s);
            }
            status = STATUS_OK;
        } else if (strncmp("id", it->key, 2) == 0) {
            status = process_id(jcmd, jobj, it->key);
		} else if (strcmp_PS(OP_msg, it->key) == 0) {
            const char *s = it->value;
            Serial.println(s);
            status = STATUS_OK;
            TESTCOUT1("msg:", s);
        } else if (strncmp("pgm", it->key, 3) == 0) {
            status = processProgram(jcmd, jobj, it->key);
            break; // This critically interrupts processing	to allow new program to run
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

    //TESTCOUT1("processObj:", "out");
    return status;
}


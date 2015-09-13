#ifdef CMAKE
#include <cstring>
#include <cstdio>
#endif
#include "version.h"
#include "RawController.h"
#include "ProcessField.h"
#include "ProgMem.h"

using namespace firestep;

typedef class MTO_RAWMoveTo {
private:
    int32_t nLoops;
    Quad<PH5TYPE> destination;
    int16_t nSegs;
    Machine &machine;

private:
    Status execute(JsonCommand& jcmd, JsonObject *pjobj);

public:
    MTO_RAWMoveTo(Machine& machine);
    Status process(JsonCommand& jcmd, JsonObject& jobj, const char* key);
} MTO_RAWMoveTo;

MTO_RAWMoveTo::MTO_RAWMoveTo(Machine& machine)
    : nLoops(0), nSegs(0), machine(machine)
{
    Quad<StepCoord> curPos = machine.getMotorPosition();
    for (QuadIndex i=0; i<QUAD_ELEMENTS; i++) {
        destination.value[i] = curPos.value[i];
    }
}

Status MTO_RAWMoveTo::execute(JsonCommand &jcmd, JsonObject *pjobj) {
    StrokeBuilder sb(machine.vMax, machine.tvMax);
    Quad<StepCoord> curPos = machine.getMotorPosition();
    Quad<StepCoord> dPos;
    for (QuadIndex i=0; i<QUAD_ELEMENTS; i++) {
        dPos.value[i] = destination.value[i] - curPos.value[i];
    }
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
        status = sb.buildLine(machine.stroke, dPos, machine.pDuino);
        if (status != STATUS_OK) {
            return status;
        }
        Ticks tStrokeStart = machine.pDuino->ticks();
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
            status = machine.stroke.traverse(machine.pDuino->ticks(), machine);
        } while (status == STATUS_BUSY_MOVING);
        tp = machine.stroke.getTimePlanned();
        ts = (machine.pDuino->ticks() - tStrokeStart) / (float) TICKS_PER_SECOND;
        pp = machine.stroke.vPeak * (machine.stroke.length / ts);
        sg = machine.stroke.length;
    }

    if (pjobj) {
        if (pjobj->at("lp").success()) {
            (*pjobj)["lp"] = nLoops;
        }
        if (pjobj->at("pp").success()) {
            (*pjobj)["pp"].set(pp, 1);
        }
        if (pjobj->at("sg").success()) {
            (*pjobj)["sg"] = sg;
        }
        if (pjobj->at("tp").success()) {
            (*pjobj)["tp"].set(tp, 3);
        }
        if (pjobj->at("ts").success()) {
            (*pjobj)["ts"].set(ts, 3);
        }
    }

    return status;
}

Status MTO_RAWMoveTo::process(JsonCommand& jcmd, JsonObject& jobj, const char* key) {
    Status status = STATUS_OK;
    const char *s;

    if (machine.pDuino->PM_strcmp(OP_mov, key) == 0) {
        if ((s = jobj[key]) && *s == 0) {
            JsonObject& node = jobj.createNestedObject(key);
            jcmd.addQueryAttr(node, OP_lp, machine.pDuino);
            jcmd.addQueryAttr(node, OP_mv, machine.pDuino);
            jcmd.addQueryAttr(node, OP_pp, machine.pDuino);
            jcmd.addQueryAttr(node, OP_sg, machine.pDuino);
            jcmd.addQueryAttr(node, OP_tp, machine.pDuino);
            jcmd.addQueryAttr(node, OP_ts, machine.pDuino);
            if (machine.getMotorAxis(0).isEnabled()) {
                jcmd.addQueryAttr(node, OP_1, machine.pDuino);
            }
            if (machine.getMotorAxis(1).isEnabled()) {
                jcmd.addQueryAttr(node, OP_2, machine.pDuino);
            }
            if (machine.getMotorAxis(2).isEnabled()) {
                jcmd.addQueryAttr(node, OP_3, machine.pDuino);
            }
            if (machine.getMotorAxis(3).isEnabled()) {
                jcmd.addQueryAttr(node, OP_4, machine.pDuino);
            }
        }
        JsonObject& kidObj = jobj[key];
        if (!kidObj.success()) {
            return jcmd.setError(STATUS_JSON_OBJECT, key);
        }
        for (JsonObject::iterator it = kidObj.begin(); it != kidObj.end(); ++it) {
            status = process(jcmd, kidObj, it->key);
            if (status != STATUS_OK) {
                TESTCOUT1("MTO_RAWMoveTo::process() status:", status);
                return status;
            }
        }
        status = execute(jcmd, &kidObj);
    } else if (machine.pDuino->PM_strcmp(OP_movrx,key) == 0 || machine.pDuino->PM_strcmp(OP_rx,key) == 0) {
        return jcmd.setError(STATUS_MTO_FIELD, key);
    } else if (machine.pDuino->PM_strcmp(OP_movry,key) == 0 || machine.pDuino->PM_strcmp(OP_ry,key) == 0) {
        return jcmd.setError(STATUS_MTO_FIELD, key);
    } else if (machine.pDuino->PM_strcmp(OP_movrz,key) == 0 || machine.pDuino->PM_strcmp(OP_rz,key) == 0) {
        return jcmd.setError(STATUS_MTO_FIELD, key);
    } else if (strncmp("mov", key, 3) == 0) { // short form
        // TODO: clean up mov implementation
        MotorIndex iMotor = machine.motorOfName(key + strlen(key) - 1);
        if (iMotor == INDEX_NONE) {
            TESTCOUT1("STATUS_NO_MOTOR: ", key);
            return jcmd.setError(STATUS_NO_MOTOR, key);
        }
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, destination.value[iMotor]);
        if (status == STATUS_OK) {
            status = execute(jcmd, NULL);
        }
    } else if (machine.pDuino->PM_strcmp(OP_d, key) == 0) {
        if (!jobj.at("a").success()) {
            return jcmd.setError(STATUS_FIELD_REQUIRED,"a");
        }
    } else if (machine.pDuino->PM_strcmp(OP_a, key) == 0) {
        // polar CCW from X-axis around X0Y0
        if (!jobj.at("d").success()) {
            return jcmd.setError(STATUS_FIELD_REQUIRED,"d");
        }
        PH5TYPE d = jobj["d"];
        PH5TYPE a = jobj["a"];
        PH5TYPE pi = 3.14159265359;
        PH5TYPE radians = a * pi / 180.0;
        PH5TYPE y = d * sin(radians);
        PH5TYPE x = d * cos(radians);
        TESTCOUT2("x:", x, " y:", y);
        destination.value[0] = x;
        destination.value[1] = y;
    } else if (machine.pDuino->PM_strcmp(OP_lp, key) == 0) {
        // output variable
    } else if (machine.pDuino->PM_strcmp(OP_mv, key) == 0) {
        status = processField<int32_t, int32_t>(jobj, key, machine.vMax);
    } else if (machine.pDuino->PM_strcmp(OP_pp, key) == 0) {
        // output variable
    } else if (machine.pDuino->PM_strcmp(OP_sg, key) == 0) {
        status = processField<int16_t, int32_t>(jobj, key, nSegs);
    } else if (machine.pDuino->PM_strcmp(OP_ts, key) == 0) {
        // output variable
    } else if (machine.pDuino->PM_strcmp(OP_tp, key) == 0) {
        // output variable
    } else if (machine.pDuino->PM_strcmp(OP_tv, key) == 0) {
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, machine.tvMax);
    } else {
        MotorIndex iMotor = machine.motorOfName(key);
        if (iMotor == INDEX_NONE) {
            TESTCOUT1("STATUS_NO_MOTOR: ", key);
            return jcmd.setError(STATUS_NO_MOTOR, key);
        }
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, destination.value[iMotor]);
    }
    return status;
}


//////////////////// RawController //////////////////

RawController::RawController(Machine& machine)
    : JsonController(machine) {
}


const char * RawController::name() {
    return "MTO_RAW";
}

Status RawController::processPosition(JsonCommand &jcmd, JsonObject& jobj, const char* key) {
    Status status = STATUS_OK;
    const char *s;
    if (strlen(key) == 3) {
        if ((s = jobj[key]) && *s == 0) {
            JsonObject& node = jobj.createNestedObject(key);
            jcmd.addQueryAttr(node, OP_1, machine.pDuino);
            jcmd.addQueryAttr(node, OP_2, machine.pDuino);
            jcmd.addQueryAttr(node, OP_3, machine.pDuino);
            jcmd.addQueryAttr(node, OP_4, machine.pDuino);
            if (!node.at("4").success()) {
                return jcmd.setError(STATUS_JSON_KEY, "4");
            }
        }
        JsonObject& kidObj = jobj[key];
        if (!kidObj.success()) {
            return STATUS_POSITION_ERROR;
        }
        for (JsonObject::iterator it = kidObj.begin(); it != kidObj.end(); ++it) {
            status = processPosition(jcmd, kidObj, it->key);
            if (status != STATUS_OK) {
                return status;
            }
        }
    } else {
        const char* axisStr = key;
        AxisIndex iAxis = machine.axisOfName(axisStr);
        if (iAxis == INDEX_NONE) {
            if (strlen(key) > 3) {
                axisStr += 3;
                iAxis = machine.axisOfName(axisStr);
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

Status RawController::initializeProbe(JsonCommand& jcmd, JsonObject& jobj,
                                      const char* key, bool clear)
{
    Status status = STATUS_OK;
    if (clear) {
        machine.probe.setup(machine.getMotorPosition(), machine.pDuino);
    }
    if (machine.pDuino->PM_strcmp(OP_prb, key) == 0) {
        const char *s;
        if ((s = jobj[key]) && *s == 0) {
            JsonObject& node = jobj.createNestedObject(key);
            jcmd.addQueryAttr(node, OP_1, machine.pDuino);
            jcmd.addQueryAttr(node, OP_2, machine.pDuino);
            jcmd.addQueryAttr(node, OP_3, machine.pDuino);
            jcmd.addQueryAttr(node, OP_4, machine.pDuino);
            jcmd.addQueryAttr(node, OP_ip, machine.pDuino);
            jcmd.addQueryAttr(node, OP_pn, machine.pDuino);
            jcmd.addQueryAttr(node, OP_sd, machine.pDuino);
        }
        JsonObject& kidObj = jobj[key];
        if (kidObj.success()) {
            for (JsonObject::iterator it = kidObj.begin(); it != kidObj.end(); ++it) {
                status = initializeProbe(jcmd, kidObj, it->key, false);
                if (status < 0) {
                    return jcmd.setError(status, it->key);
                }
            }
            if (status == STATUS_BUSY_CALIBRATING && machine.probe.pinProbe==NOPIN) {
                return jcmd.setError(STATUS_FIELD_REQUIRED, "pn");
            }
        }
    } else if (machine.pDuino->PM_strcmp(OP_prbip, key) == 0 || machine.pDuino->PM_strcmp(OP_ip, key) == 0) {
        status = processField<bool, bool>(jobj, key, machine.probe.invertProbe);
    } else if (machine.pDuino->PM_strcmp(OP_prbpn, key) == 0 || machine.pDuino->PM_strcmp(OP_pn, key) == 0) {
        status = processField<PinType, int32_t>(jobj, key, machine.probe.pinProbe);
    } else if (machine.pDuino->PM_strcmp(OP_prbsd, key) == 0 || machine.pDuino->PM_strcmp(OP_sd, key) == 0) {
        status = processField<DelayMics, int32_t>(jobj, key, machine.searchDelay);
    } else {
        MotorIndex iMotor = machine.motorOfName(key + (strlen(key) - 1));
        if (iMotor == INDEX_NONE) {
            return jcmd.setError(STATUS_NO_MOTOR, key);
        }
        status = processProbeField(machine, iMotor, jcmd, jobj, key);
    }
    return status == STATUS_OK ? STATUS_BUSY_CALIBRATING : status;
}

Status RawController::processProbe(JsonCommand& jcmd, JsonObject& jobj, const char* key) {
    Status status = jcmd.getStatus();
    switch (status) {
    case STATUS_BUSY_PARSED:
        status = initializeProbe(jcmd, jobj, key, true);
        break;
    case STATUS_BUSY_OK:
    case STATUS_BUSY_CALIBRATING:
        status = machine.probeNow(status);
        if (status == STATUS_OK) {
            JsonObject& kidObj = jobj[key];
            for (JsonObject::iterator it = kidObj.begin(); it != kidObj.end(); ++it) {
                MotorIndex iMotor = machine.motorOfName(it->key + (strlen(it->key) - 1));
                if (iMotor != INDEX_NONE) {
                    kidObj[it->key] = machine.getMotorAxis(iMotor).position;
                }
            }
        }
        break;
    default:
        ASSERT(false);
        return jcmd.setError(STATUS_STATE, key);
    }
    return status;
}


Status RawController::processDimension(JsonCommand& jcmd, JsonObject& jobj, const char* key) {
    return jcmd.setError(STATUS_TOPOLOGY_NAME, key);
}

Status RawController::processMove(JsonCommand& jcmd, JsonObject& jobj, const char* key) {
    return MTO_RAWMoveTo(machine).process(jcmd, jobj, key);
}

Status RawController::initializeHome(JsonCommand& jcmd, JsonObject& jobj,
                                     const char* key, bool clear)
{
    Status status = STATUS_OK;
    if (clear) {
        for (QuadIndex i = 0; i < QUAD_ELEMENTS; i++) {
            machine.getMotorAxis(i).homing = false;
        }
    }
    if (machine.pDuino->PM_strcmp(OP_hom, key) == 0) {
        const char *s;
        if ((s = jobj[key]) && *s == 0) {
            JsonObject& node = jobj.createNestedObject(key);
            node["1"] = ""; // do not use jcmd.addQueryAttr()
            node["2"] = ""; // do not use jcmd.addQueryAttr()
            node["3"] = ""; // do not use jcmd.addQueryAttr()
            node["4"] = ""; // do not use jcmd.addQueryAttr()
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

Status RawController::processHome(JsonCommand& jcmd, JsonObject& jobj, const char* key) {
    Status status = jcmd.getStatus();
    switch (status) {
    case STATUS_BUSY_PARSED:
        status = initializeHome(jcmd, jobj, key, true);
        break;
    case STATUS_BUSY_MOVING:
    case STATUS_BUSY_OK:
    case STATUS_BUSY_CALIBRATING:
        status = machine.home(status);
        break;
    default:
        TESTCOUT1("status:", status);
        ASSERT(false);
        return jcmd.setError(STATUS_STATE, key);
    }
    return status;
}


#include "Arduino.h"
#ifdef CMAKE
#include <cstring>
#include <cstdio>
#endif
#include "version.h"
#include "FPDController.h"
#include "ProcessField.h"

using namespace firestep;

FPDController::FPDController(Machine& machine)
    : JsonController(machine) {
}

const char * FPDController::name() {
    return "MTO_FPD";
}

Status FPDController::processPosition(JsonCommand &jcmd, JsonObject& jobj, const char* key) {
    Status status = STATUS_OK;
    const char *axisStr = key + strlen(key) - 1;
    const char *s;
    if (strlen(key) == 3) {
        if ((s = jobj[key]) && *s == 0) {
            JsonObject& node = jobj.createNestedObject(key);
            node["1"] = "";
            node["2"] = "";
            node["3"] = "";
            node["4"] = "";
            node["x"] = "";
            node["y"] = "";
            node["z"] = "";
            if (!node.at("4").success()) {
                return jcmd.setError(STATUS_JSON_KEY, "4");
            }
        }
        JsonObject& kidObj = jobj[key];
        if (!kidObj.success()) {
            return jcmd.setError(STATUS_POSITION_ERROR, key);
        }
        for (JsonObject::iterator it = kidObj.begin(); it != kidObj.end(); ++it) {
            status = processPosition(jcmd, kidObj, it->key);
            if (status != STATUS_OK) {
                return status;
            }
        }
    } else if (strcmp("1", axisStr) == 0) {
        status = processField<StepCoord, int32_t>(jobj, key, machine.axis[0].position);
    } else if (strcmp("2", axisStr) == 0) {
        status = processField<StepCoord, int32_t>(jobj, key, machine.axis[1].position);
    } else if (strcmp("3", axisStr) == 0) {
        status = processField<StepCoord, int32_t>(jobj, key, machine.axis[2].position);
    } else if (strcmp("4", axisStr) == 0) {
        status = processField<StepCoord, int32_t>(jobj, key, machine.axis[3].position);
    } else if (strcmp("x", axisStr) == 0) {
        XYZ3D xyz(machine.getXYZ3D());
        if (!xyz.isValid()) {
            return jcmd.setError(STATUS_KINEMATIC_XYZ, key);
        }
        PH5TYPE value = xyz.x;
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, value);
        if (value != xyz.x) {
            status = jcmd.setError(STATUS_OUTPUT_FIELD, key);
        }
    } else if (strcmp("y", axisStr) == 0) {
        XYZ3D xyz(machine.getXYZ3D());
        if (!xyz.isValid()) {
            return jcmd.setError(STATUS_KINEMATIC_XYZ, key);
        }
        PH5TYPE value = xyz.y;
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, value);
        if (value != xyz.y) {
            status = jcmd.setError(STATUS_OUTPUT_FIELD, key);
        }
    } else if (strcmp("z", axisStr) == 0) {
        XYZ3D xyz(machine.getXYZ3D());
        if (!xyz.isValid()) {
            return jcmd.setError(STATUS_KINEMATIC_XYZ, key);
        }
        PH5TYPE value = xyz.z;
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, value);
        if (value != xyz.z) {
            status = jcmd.setError(STATUS_OUTPUT_FIELD, key);
        }
    } else {
        return jcmd.setError(STATUS_UNRECOGNIZED_NAME, key);
    }
    return status;
}

Status FPDController::initializeProbe_MTO_FPD(JsonCommand& jcmd, JsonObject& jobj,
        const char* key, bool clear)
{
    Status status = STATUS_OK;
    OpProbe &probe = machine.op.probe;

    if (clear) {
        Quad<StepCoord> curPos = machine.getMotorPosition();
        probe.setup(curPos);
    }
    Step3D probeEnd(probe.end.value[0], probe.end.value[1], probe.end.value[2]);
    XYZ3D xyzEnd = machine.delta.calcXYZ(probeEnd);
    const char *s;
    if (strcmp("prb", key) == 0) {
        if ((s = jobj[key]) && *s == 0) {
            JsonObject& node = jobj.createNestedObject(key);
            xyzEnd.z = machine.delta.getMinZ(xyzEnd.z, xyzEnd.y);
            node["1"] = "";
            node["2"] = "";
            node["3"] = "";
            node["4"] = "";
            node["ip"] = "";
            node["pn"] = machine.op.probe.pinProbe;
            node["sd"] = "";
            node["x"] = xyzEnd.x;
            node["y"] = xyzEnd.y;
            node["z"] = xyzEnd.z;
        }
        JsonObject& kidObj = jobj[key];
        if (kidObj.success()) {
            for (JsonObject::iterator it = kidObj.begin(); it != kidObj.end(); ++it) {
                status = initializeProbe_MTO_FPD(jcmd, kidObj, it->key, false);
                if (status < 0) {
                    return jcmd.setError(status, it->key);
                }
            }
            if (status == STATUS_BUSY_CALIBRATING && machine.op.probe.pinProbe==NOPIN) {
                return jcmd.setError(STATUS_FIELD_REQUIRED, "pn");
            }
        }
    } else if (strcmp("prbip", key) == 0 || strcmp("ip", key) == 0) {
        status = processField<bool, bool>(jobj, key, machine.op.probe.invertProbe);
    } else if (strcmp("prbpn", key) == 0 || strcmp("pn", key) == 0) {
        status = processField<PinType, int32_t>(jobj, key, machine.op.probe.pinProbe);
    } else if (strcmp("prbsd", key) == 0 || strcmp("sd", key) == 0) {
        status = processField<DelayMics, int32_t>(jobj, key, machine.searchDelay);
    } else if (strcmp("prbx", key) == 0 || strcmp("x", key) == 0) {
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, xyzEnd.x);
    } else if (strcmp("prby", key) == 0 || strcmp("y", key) == 0) {
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, xyzEnd.y);
    } else if (strcmp("prbz", key) == 0 || strcmp("z", key) == 0) {
        machine.op.probe.dataSource = PDS_Z;
        xyzEnd.z = machine.delta.getMinZ(xyzEnd.x, xyzEnd.y);
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, xyzEnd.z);
    } else {
        MotorIndex iMotor = machine.motorOfName(key + (strlen(key) - 1));
        if (iMotor != INDEX_NONE) {
            if ((s = jobj[key]) && *s == 0) {
                // query is fine
            } else {
                return jcmd.setError(STATUS_OUTPUT_FIELD, key);
            }
        } else {
            return jcmd.setError(STATUS_UNRECOGNIZED_NAME, key);
        }
    }

    // This code only works for probes along a single cartesian axis
    Step3D pEnd = machine.delta.calcPulses(xyzEnd);
    machine.op.probe.end.value[0] = pEnd.p1;
    machine.op.probe.end.value[1] = pEnd.p2;
    machine.op.probe.end.value[2] = pEnd.p3;
    TESTCOUT3("pEnd:", pEnd.p1, ", ", pEnd.p2, ", ", pEnd.p3);
    machine.op.probe.maxDelta = 0;
    for (MotorIndex iMotor=0; iMotor<3; iMotor++) {
        StepCoord delta = machine.op.probe.end.value[iMotor] - machine.op.probe.start.value[iMotor];
        machine.op.probe.maxDelta = max((StepCoord)abs(machine.op.probe.maxDelta), delta);
    }

    return status == STATUS_OK ? STATUS_BUSY_CALIBRATING : status;
}

Status FPDController::finalizeProbe_MTO_FPD(JsonCommand& jcmd, JsonObject& jobj, const char* key) {
    XYZ3D xyz = machine.getXYZ3D();
    if (!xyz.isValid()) {
        return jcmd.setError(STATUS_KINEMATIC_XYZ, key);
    }
    if (strcmp("prbx",key) == 0 || strcmp("x",key) == 0) {
        jobj[key] = xyz.x;
    } else if (strcmp("prby",key) == 0 || strcmp("y",key) == 0) {
        jobj[key] = xyz.y;
    } else if (strcmp("prbz",key) == 0 || strcmp("z",key) == 0) {
        jobj[key] = xyz.z;
    } else if (strcmp("1",key) == 0) {
        jobj[key] = machine.getMotorAxis(0).position;
    } else if (strcmp("2",key) == 0) {
        jobj[key] = machine.getMotorAxis(1).position;
    } else if (strcmp("3",key) == 0) {
        jobj[key] = machine.getMotorAxis(2).position;
    } else if (strcmp("4",key) == 0) {
        jobj[key] = machine.getMotorAxis(3).position;
    }
    return STATUS_OK;
}

Status FPDController::processProbe_MTO_FPD(JsonCommand& jcmd, JsonObject& jobj, const char* key) {
    Status status = jcmd.getStatus();
    switch (status) {
    case STATUS_BUSY_PARSED:
        status = initializeProbe_MTO_FPD(jcmd, jobj, key, true);
        break;
    case STATUS_BUSY_OK:
    case STATUS_BUSY_CALIBRATING:
        status = machine.probe(status);
        if (status == STATUS_OK) {
            if (jobj[key].is<JsonObject&>()) {
                JsonObject &kidObj = jobj[key];
                for (JsonObject::iterator it = kidObj.begin(); status == STATUS_OK && it != kidObj.end(); ++it) {
                    status = finalizeProbe_MTO_FPD(jcmd, kidObj, it->key);
                }
            } else {
                status = finalizeProbe_MTO_FPD(jcmd, jobj, key);
            }
        }
        break;
    default:
        ASSERT(false);
        return jcmd.setError(STATUS_STATE, key);
    }
    return status;
}

Status FPDController::processDimension_MTO_FPD(JsonCommand& jcmd, JsonObject& jobj, const char* key) {
    Status status = STATUS_OK;
    if (strcmp("dim", key) == 0) {
        const char *s;
        if ((s = jobj[key]) && *s == 0) {
            JsonObject& node = jobj.createNestedObject(key);
            node["e"] = "";
            node["f"] = "";
            node["gr"] = "";
            node["ha1"] = "";
            node["ha2"] = "";
            node["ha3"] = "";
            node["mi"] = "";
            node["pd"] = "";
            node["re"] = "";
            node["rf"] = "";
            node["st"] = "";
        }
        JsonObject& kidObj = jobj[key];
        if (kidObj.success()) {
            for (JsonObject::iterator it = kidObj.begin(); it != kidObj.end(); ++it) {
                status = processDimension_MTO_FPD(jcmd, kidObj, it->key);
                if (status != STATUS_OK) {
                    return status;
                }
            }
        }
    } else if (strcmp("e", key) == 0 || strcmp("dime", key) == 0) {
        PH5TYPE value = machine.delta.getEffectorLength();
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, value);
        machine.delta.setEffectorLength(value);
    } else if (strcmp("f", key) == 0 || strcmp("dimf", key) == 0) {
        PH5TYPE value = machine.delta.getBaseArmLength();
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, value);
        machine.delta.setBaseArmLength(value);
    } else if (strcmp("gr", key) == 0 || strcmp("dimgr", key) == 0) {
        PH5TYPE value = machine.delta.getGearRatio();
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, value);
        machine.delta.setGearRatio(value);
    } else if (strcmp("ha1", key) == 0 || strcmp("dimha1", key) == 0) {
        Angle3D homeAngles = machine.delta.getHomeAngles();
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, homeAngles.theta1);
        machine.delta.setHomeAngles(homeAngles);
    } else if (strcmp("ha2", key) == 0 || strcmp("dimha2", key) == 0) {
        Angle3D homeAngles = machine.delta.getHomeAngles();
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, homeAngles.theta2);
        machine.delta.setHomeAngles(homeAngles);
    } else if (strcmp("ha3", key) == 0 || strcmp("dimha3", key) == 0) {
        Angle3D homeAngles = machine.delta.getHomeAngles();
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, homeAngles.theta3);
        machine.delta.setHomeAngles(homeAngles);
    } else if (strcmp("mi", key) == 0 || strcmp("dimmi", key) == 0) {
        int16_t value = machine.delta.getMicrosteps();
        status = processField<int16_t, int16_t>(jobj, key, value);
        machine.delta.setMicrosteps(value);
    } else if (strcmp("pd", key) == 0 || strcmp("dimpd", key) == 0) {
        const char *s;
        if ((s = jobj[key]) && *s == 0) {
            JsonArray &jarr = jobj.createNestedArray(key);
            for (int16_t i=0; i<PROBE_DATA; i++) {
                jarr.add(machine.op.probe.probeData[i]);
            }
        } else {
            status = jcmd.setError(STATUS_OUTPUT_FIELD, key);
        }
    } else if (strcmp("re", key) == 0 || strcmp("dimre", key) == 0) {
        PH5TYPE value = machine.delta.getEffectorTriangleSide();
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, value);
        machine.delta.setEffectorTriangleSide(value);
    } else if (strcmp("rf", key) == 0 || strcmp("dimrf", key) == 0) {
        PH5TYPE value = machine.delta.getBaseTriangleSide();
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, value);
        machine.delta.setBaseTriangleSide(value);
    } else if (strcmp("st", key) == 0 || strcmp("dimst", key) == 0) {
        int16_t value = machine.delta.getSteps360();
        status = processField<int16_t, int16_t>(jobj, key, value);
        machine.delta.setSteps360(value);
    } else {
        return jcmd.setError(STATUS_UNRECOGNIZED_NAME, key);
    }
    return status;
}


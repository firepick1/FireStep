#include "Arduino.h"
#ifdef CMAKE
#include <cstring>
#include <cstdio>
#endif
#include "version.h"
#include "FPDController.h"
#include "ProcessField.h"

using namespace firestep;

//////////////// FPDCalibrateHome ///////////////

FPDCalibrateHome::FPDCalibrateHome(Machine& machine) 
	: machine(machine), status(STATUS_BUSY_CALIBRATING) {
}

Status FPDCalibrateHome::calibrate() {
	if (status != STATUS_BUSY_CALIBRATING) {
		return status;
	}
	OpProbe& probe = machine.op.probe;
	zCenter = (probe.probeData[0]+probe.probeData[7])/2;
	PH5TYPE zRim0 = (probe.probeData[1]+probe.probeData[4])/2;;
	PH5TYPE zRim60 = (probe.probeData[2]+probe.probeData[5])/2;;
	PH5TYPE zRim120 = (probe.probeData[3]+probe.probeData[6])/2;;
	zRim = (zRim0+zRim60+zRim120)/3;
	if (abs(zRim-zCenter) > 2) {
		status = STATUS_CAL_HOME1;
		return status;
	}
	PH5TYPE radius = 50;
	eTheta = machine.delta.calcZBowlETheta(zCenter, zRim, radius);
	homeAngle = machine.delta.getHomeAngle() + eTheta;
	TESTCOUT4("CalibrateHome zCenter:", zCenter, " zRim:", zRim,
			  " eTheta:", eTheta, " homeAngle:", homeAngle);
	status = STATUS_OK;
	return status;
}

Status FPDCalibrateHome::save() {
	Status status = calibrate();
	if (status == STATUS_OK) {
		machine.delta.setHomeAngle(homeAngle);
		StepCoord pulses = machine.delta.getHomePulses();
		machine.axis[0].home = pulses;
		machine.axis[1].home = pulses;
		machine.axis[2].home = pulses;
	}
	return status;
}

/////////////////////////// FPDMoveTo ///////////////
typedef class FPDMoveTo {
private:
    int32_t nLoops;
    Quad<PH5TYPE> destination;
    int16_t nSegs;
    Machine &machine;
    FPDController &controller;

private:
    Status execute(JsonCommand& jcmd, JsonObject *pjobj);

public:
    FPDMoveTo(FPDController &controller, Machine& machine);
    Status process(JsonCommand& jcmd, JsonObject& jobj, const char* key);
} FPDMoveTo;

FPDMoveTo::FPDMoveTo(FPDController &controller, Machine& machine)
    : nLoops(0), nSegs(0), machine(machine), controller(controller)
{
	machine.loadDeltaCalculator();
	StepCoord pulses = machine.delta.getHomePulses();
	ASSERTEQUAL(machine.axis[0].home, pulses);
	ASSERTEQUAL(machine.axis[1].home, pulses);
	ASSERTEQUAL(machine.axis[2].home, pulses);

    Quad<StepCoord> curPos = machine.getMotorPosition();
    for (QuadIndex i=0; i<QUAD_ELEMENTS; i++) {
        destination.value[i] = curPos.value[i];
    }

    XYZ3D xyz(controller.getXYZ3D());
    destination.value[0] = xyz.x;
    destination.value[1] = xyz.y;
    destination.value[2] = xyz.z;
}

Status FPDMoveTo::execute(JsonCommand &jcmd, JsonObject *pjobj) {
    StrokeBuilder sb(machine.vMax, machine.tvMax);
    Quad<StepCoord> curPos = machine.getMotorPosition();
    Quad<StepCoord> dPos;
    XYZ3D xyz(destination.value[0], destination.value[1], destination.value[2]);
    Step3D pulses(machine.delta.calcPulses(xyz));
	if (!pulses.isValid()) {
		TESTCOUT3("FPDMoveTo STATUS_KINEMATIC_ERROR x:", xyz.x, " y:", xyz.y, " z:", xyz.z);
		return STATUS_KINEMATIC_XYZ;
	}
    dPos.value[0] = pulses.p1 - curPos.value[0];
    dPos.value[1] = pulses.p2 - curPos.value[1];
    dPos.value[2] = pulses.p3 - curPos.value[2];
    dPos.value[3] = destination.value[3] - curPos.value[3];
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

Status FPDMoveTo::process(JsonCommand& jcmd, JsonObject& jobj, const char* key) {
    Status status = STATUS_OK;
    const char *s;

    if (strcmp("mov", key) == 0) {
        if ((s = jobj[key]) && *s == 0) {
            JsonObject& node = jobj.createNestedObject(key);
            node["lp"] = "";
            node["mv"] = "";
            node["pp"] = "";
            node["sg"] = "";
            node["tp"] = "";
            node["ts"] = "";
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
                TESTCOUT1("FPDMoveTo::process() status:", status);
                return status;
            }
        }
        status = execute(jcmd, &kidObj);
    } else if (strcmp("movrx",key) == 0 || strcmp("rx",key) == 0) {
        // TODO: clean up mov implementation
        switch (machine.topology) {
        case MTO_FPD: {
            XYZ3D xyz = controller.getXYZ3D();
            PH5TYPE x = 0;
            status = processField<PH5TYPE, PH5TYPE>(jobj, key, x);
            if (status == STATUS_OK) {
                destination.value[0] = xyz.x + x;
                if (strcmp("movrx",key) == 0) {
                    status = execute(jcmd, NULL);
                }
            }
            break;
        }
        default:
            return jcmd.setError(STATUS_MTO_FIELD, key);
        }
    } else if (strcmp("movry",key) == 0 || strcmp("ry",key) == 0) {
        // TODO: clean up mov implementation
        switch (machine.topology) {
        case MTO_FPD: {
            XYZ3D xyz = controller.getXYZ3D();
            PH5TYPE y = 0;
            status = processField<PH5TYPE, PH5TYPE>(jobj, key, y);
            if (status == STATUS_OK) {
                destination.value[1] = xyz.y + y;
                if (strcmp("movry",key) == 0) {
                    status = execute(jcmd, NULL);
                }
            }
            break;
        }
        default:
            return jcmd.setError(STATUS_MTO_FIELD, key);
        }
    } else if (strcmp("movrz",key) == 0 || strcmp("rz",key) == 0) {
        // TODO: clean up mov implementation
        switch (machine.topology) {
        case MTO_FPD: {
            XYZ3D xyz = controller.getXYZ3D();
            PH5TYPE z = 0;
            status = processField<PH5TYPE, PH5TYPE>(jobj, key, z);
            if (status == STATUS_OK) {
                destination.value[2] = xyz.z + z;
                if (strcmp("movrz",key) == 0) {
                    status = execute(jcmd, NULL);
                }
            }
            break;
        }
        default:
            return jcmd.setError(STATUS_MTO_FIELD, key);
        }
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
    } else if (strcmp("d", key) == 0) {
        if (!jobj.at("a").success()) {
            return jcmd.setError(STATUS_FIELD_REQUIRED,"a");
        }
    } else if (strcmp("a", key) == 0) {
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
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, destination.value[iMotor]);
    }
    return status;
}

////////////////////// FPDController ///////////////

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
        XYZ3D xyz(getXYZ3D());
        if (!xyz.isValid()) {
            return jcmd.setError(STATUS_KINEMATIC_XYZ, key);
        }
        PH5TYPE value = xyz.x;
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, value);
        if (value != xyz.x) {
            status = jcmd.setError(STATUS_OUTPUT_FIELD, key);
        }
    } else if (strcmp("y", axisStr) == 0) {
        XYZ3D xyz(getXYZ3D());
        if (!xyz.isValid()) {
            return jcmd.setError(STATUS_KINEMATIC_XYZ, key);
        }
        PH5TYPE value = xyz.y;
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, value);
        if (value != xyz.y) {
            status = jcmd.setError(STATUS_OUTPUT_FIELD, key);
        }
    } else if (strcmp("z", axisStr) == 0) {
        XYZ3D xyz(getXYZ3D());
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
    XYZ3D xyz = getXYZ3D();
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

Status FPDController::processProbe(JsonCommand& jcmd, JsonObject& jobj, const char* key) {
    Status status = jcmd.getStatus();
    switch (status) {
    case STATUS_BUSY_PARSED:
        status = initializeProbe_MTO_FPD(jcmd, jobj, key, true);
        break;
    case STATUS_BUSY_OK:
    case STATUS_BUSY_CALIBRATING:
        status = machine.probe(status);
        if (status == STATUS_OK) {
            if (machine.op.probe.dataSource == PDS_Z) {
                XYZ3D xyz = getXYZ3D();
                machine.op.probe.archiveData(xyz.z);
            }
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

Status FPDController::processDimension(JsonCommand& jcmd, JsonObject& jobj, const char* key) {
    Status status = STATUS_OK;
    if (strcmp("dim", key) == 0) {
        const char *s;
        if ((s = jobj[key]) && *s == 0) {
            JsonObject& node = jobj.createNestedObject(key);
            node["e"] = "";
            node["f"] = "";
            node["gr"] = "";
            node["ha"] = "";
            //node["ha1"] = "";
            //node["ha2"] = "";
            //node["ha3"] = "";
            node["mi"] = "";
            node["pd"] = "";
            node["re"] = "";
            node["rf"] = "";
            node["st"] = "";
        }
        JsonObject& kidObj = jobj[key];
        if (kidObj.success()) {
            for (JsonObject::iterator it = kidObj.begin(); it != kidObj.end(); ++it) {
                status = processDimension(jcmd, kidObj, it->key);
                if (status != STATUS_OK) {
                    return status;
                }
            }
        }
    } else if (strcmp("e", key) == 0 || strcmp("dime", key) == 0) {
        PH5TYPE value = machine.delta.getEffectorTriangleSide();
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, value);
        machine.delta.setEffectorTriangleSide(value);
    } else if (strcmp("f", key) == 0 || strcmp("dimf", key) == 0) {
        PH5TYPE value = machine.delta.getBaseTriangleSide();
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, value);
        machine.delta.setBaseTriangleSide(value);
    } else if (strcmp("gr", key) == 0 || strcmp("dimgr", key) == 0) {
        PH5TYPE value = machine.delta.getGearRatio();
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, value);
        machine.delta.setGearRatio(value);
    } else if (strcmp("ha", key) == 0 || strcmp("dimha", key) == 0) {
        PH5TYPE homeAngle = machine.delta.getHomeAngle();
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, homeAngle);
        machine.delta.setHomeAngle(homeAngle);
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
        PH5TYPE value = machine.delta.getEffectorLength();
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, value);
        machine.delta.setEffectorLength(value);
    } else if (strcmp("rf", key) == 0 || strcmp("dimrf", key) == 0) {
        PH5TYPE value = machine.delta.getBaseArmLength();
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, value);
        machine.delta.setBaseArmLength(value);
    } else if (strcmp("st", key) == 0 || strcmp("dimst", key) == 0) {
        int16_t value = machine.delta.getSteps360();
        status = processField<int16_t, int16_t>(jobj, key, value);
        machine.delta.setSteps360(value);
    } else {
        return jcmd.setError(STATUS_UNRECOGNIZED_NAME, key);
    }
    return status;
}

Status FPDController::processMove(JsonCommand& jcmd, JsonObject& jobj, const char* key) {
    return FPDMoveTo(*this, machine).process(jcmd, jobj, key);
}

Status FPDController::initializeHome(JsonCommand& jcmd, JsonObject& jobj,
                                      const char* key, bool clear)
{
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

Status FPDController::finalizeHome() {
    Status status = STATUS_OK;
	
	// calculate distance to post-home destination
	machine.loadDeltaCalculator();
    Quad<StepCoord> limit = machine.getMotorPosition();
	XYZ3D xyzPostHome(0,0,0); // post-home destination
    Step3D oPulses = machine.delta.calcPulses(xyzPostHome);
	TESTCOUT3("finalizeHome x home:", machine.delta.getHomePulses(), " pos:", machine.axis[0].position, " dst:", oPulses.p1);
    machine.op.probe.setup(limit, Quad<StepCoord>(
                               oPulses.p1,
                               oPulses.p2,
                               oPulses.p3,
                               limit.value[3]
                           ));
	
	// move to post-home destination using probe
    status = STATUS_BUSY_CALIBRATING;
    do {
        // fast probe because we don't expect to hit anything
        status = machine.probe(status, 0);
        //TESTCOUT2("finalizeHome status:", (int) status, " 1:", axis[0].position);
    } while (status == STATUS_BUSY_CALIBRATING);

    if (status == STATUS_PROBE_FAILED) {
        // we didn't hit anything and that is good
        status = STATUS_OK;
    } else if (status == STATUS_OK) {
        // we hit something and that's not good
        status = STATUS_LIMIT_MAX;
    }

    return status;
}

Status FPDController::processHome(JsonCommand& jcmd, JsonObject& jobj, const char* key) {
    Status status = jcmd.getStatus();
    switch (status) {
    case STATUS_BUSY_PARSED:
		machine.loadDeltaCalculator();
        status = initializeHome(jcmd, jobj, key, true);
		machine.loadDeltaCalculator();
        break;
    case STATUS_BUSY_MOVING:
    case STATUS_BUSY_OK:
        status = machine.home(status);
        break;
    case STATUS_BUSY_CALIBRATING:
		TESTCOUT4("processHome home 1:", machine.axis[0].home,
			" 2:", machine.axis[1].home,
			" 3:", machine.axis[2].home,
			" 4:", machine.axis[3].home);
        status = machine.home(status);
		TESTCOUT4("processHome position 1:", machine.axis[0].position,
			" 2:", machine.axis[1].position,
			" 3:", machine.axis[2].position,
			" 4:", machine.axis[3].position);
        if (status == STATUS_OK) {
            status = finalizeHome();
			TESTCOUT4("processHome finalize 1:", machine.axis[0].position,
				" 2:", machine.axis[1].position,
				" 3:", machine.axis[2].position,
				" 4:", machine.axis[3].position);
        }
        break;
    default:
        TESTCOUT1("status:", status);
        ASSERT(false);
        return jcmd.setError(STATUS_STATE, key);
    }
    return status;
}

Status FPDController::processCalibrate(JsonCommand &jcmd, JsonObject& jobj, const char* key) {
	FPDCalibrateHome cal(machine);
	Status status = cal.calibrate();
	if (status != STATUS_OK) {
		return status;
	}
	return processCalibrateCore(jcmd, jobj, key, cal);
}
Status FPDController::processCalibrateCore(JsonCommand &jcmd, JsonObject& jobj, const char* key, FPDCalibrateHome &cal) {
    Status status = jcmd.getStatus();
    const char *s;
    if (strcmp("cal", key) == 0) {
        if ((s = jobj[key]) && *s == 0) {
            JsonObject& node = jobj.createNestedObject(key);
            node["ha"] = "";
            node["he"] = "";
            node["sv"] = "";
            node["zc"] = "";
            node["zr"] = "";
        }
        JsonObject& kidObj = jobj[key];
        if (!kidObj.success()) {
            return jcmd.setError(STATUS_JSON_OBJECT, key);
        }
        for (JsonObject::iterator it = kidObj.begin(); it != kidObj.end(); ++it) {
            status = processCalibrateCore(jcmd, kidObj, it->key, cal);
            if (status != STATUS_OK) {
                TESTCOUT1("processCalibrate status:", status);
                return status;
            }
        }
    } else if (strcmp("calha",key) == 0 || strcmp("ha",key) == 0) {
		PH5TYPE value = cal.homeAngle;
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, value);
		if (value != cal.homeAngle) {
			return jcmd.setError(STATUS_OUTPUT_FIELD, key);
		}
    } else if (strcmp("calhe",key) == 0 || strcmp("he",key) == 0) {
		PH5TYPE value = cal.eTheta;
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, value);
		if (value != cal.eTheta) {
			return jcmd.setError(STATUS_OUTPUT_FIELD, key);
		}
    } else if (strcmp("calzc",key) == 0 || strcmp("zc",key) == 0) {
		PH5TYPE value = cal.zCenter;
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, value);
		if (value != cal.zCenter) {
			return jcmd.setError(STATUS_OUTPUT_FIELD, key);
		}
    } else if (strcmp("calzr",key) == 0 || strcmp("zr",key) == 0) {
		PH5TYPE value = cal.zRim;
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, value);
		if (value != cal.zRim) {
			return jcmd.setError(STATUS_OUTPUT_FIELD, key);
		}
    } else if (strcmp("calsv",key) == 0 || strcmp("sv",key) == 0) {
		bool value = false;
        status = processField<bool, bool>(jobj, key, value);
		if (value) {
			cal.save();
		}
    } else {
        return jcmd.setError(STATUS_UNRECOGNIZED_NAME, key);
    }
    return status;
}

void FPDController::onTopologyChanged() {
    machine.delta.setup();
    if (machine.axis[0].home >= 0 &&
            machine.axis[1].home >= 0 &&
            machine.axis[2].home >= 0) {
        // Delta always has negative home limit switch
        StepCoord home = machine.delta.getHomePulses();
        machine.axis[0].position += home-machine.axis[0].home;
        machine.axis[1].position += home-machine.axis[1].home;
        machine.axis[2].position += home-machine.axis[2].home;
        machine.axis[0].home = home;
        machine.axis[1].home = home;
        machine.axis[2].home = home;
    }
}

XYZ3D FPDController::getXYZ3D() {
    return machine.delta.calcXYZ(Step3D(
                                     machine.motorAxis[0]->position,
                                     machine.motorAxis[1]->position,
                                     machine.motorAxis[2]->position
                                 ));
}


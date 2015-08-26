#include "Arduino.h"
#ifdef CMAKE
#include <cstring>
#include <cstdio>
#endif
#include "version.h"
#include "FPDController.h"
#include "ProcessField.h"
#include "ProgMem.h"

using namespace firestep;

//////////////// FPDCalibrateHome ///////////////

#define PI ((PH5TYPE) 3.14159265359)

FPDCalibrateHome::FPDCalibrateHome(Machine& machine)
    : machine(machine), mode(CAL_NONE), saveWeight(1) {
}

#define MAX_GEAR_ZBOWL_ERROR 0.3

Status FPDCalibrateHome::calibrate() {
    machine.loadDeltaCalculator();
    OpProbe& probe = machine.op.probe;
    zCenter = (probe.probeData[0]+probe.probeData[7])/2;
    PH5TYPE zRim0 = (probe.probeData[1]+probe.probeData[4])/2;;
    PH5TYPE zRim60 = (probe.probeData[2]+probe.probeData[5])/2;;
    PH5TYPE zRim120 = (probe.probeData[3]+probe.probeData[6])/2;;
    zRim = (zRim0+zRim60+zRim120)/3;
    if (abs(zRim-zCenter) > 2) {
        return STATUS_CAL_HOME1;
    }
    PH5TYPE radius = 50;
    PH5TYPE zError = zRim - zCenter;
    PH5TYPE zRim_gearRatio = zRim; // assume all error is due to gear ratio
    PH5TYPE zRim_homeAngle = zRim; // assume all error is due to home angle

    // convert probe data to stepper coordinates
    Step3D pd[6];
    for (int16_t i=0; i<6; i++) {
        PH5TYPE a = i*60;
        PH5TYPE radians = a * PI / 180.0;
        pd[i] = machine.delta.calcPulses(XYZ3D(radius*cos(radians), radius*sin(radians), probe.probeData[1+i]));
    };

    if ((mode&CAL_HOME) && (mode&CAL_GEAR)) {
        PH5TYPE hgr = 4; // SWAG at ratio of homeAngleError / gearRatioError
        PH5TYPE g = sqrt(zError*zError /(1+hgr*hgr));
        PH5TYPE h = hgr * g;
        zRim_gearRatio = zCenter + (zError > 0 ? g : -g); // gear ratio error split
        zRim_homeAngle = zCenter + (zError > 0 ? h : -h); // homeAngle error split
    }
    TESTCOUT1("saveWeight:", saveWeight);
    if (mode&CAL_HOME) {
        eTheta = machine.delta.calcZBowlETheta(zCenter, zRim_homeAngle, radius);
        PH5TYPE homeAngleCur = machine.delta.getHomeAngle();
        homeAngle = saveWeight*(homeAngleCur+eTheta) + (1-saveWeight)*machine.getHomeAngle();
        TESTCOUT4("CalibrateHome CAL_HOME zCenter:", zCenter, " zRim_homeAngle:", zRim_homeAngle,
                  " eTheta:", eTheta, " homeAngle:", homeAngle);
        machine.setHomeAngle(homeAngle);
    }
    if (mode&CAL_GEAR) {
        if (MAX_GEAR_ZBOWL_ERROR < abs(zRim_gearRatio - zCenter)) {
            return STATUS_ZBOWL_GEAR;
        }
        gearRatio = machine.delta.calcZBowlGearRatio(zCenter, zRim_gearRatio, radius);
        PH5TYPE curGearRatio = machine.delta.getGearRatio();
        eGear = gearRatio - curGearRatio;
        gearRatio = saveWeight * gearRatio + (1-saveWeight) * curGearRatio;
        machine.delta.setGearRatio(gearRatio);
        TESTCOUT4("CalibrateHome CAL_GEAR zCenter:", zCenter, " zRim_gearRatio:", zRim_gearRatio,
                  " eGear:", eGear, " gearRatio:", gearRatio);
    }

    // Calculate bed ZPlane
    ZPlane zpl0;
    if (!zpl0.initialize(
                machine.delta.calcXYZ(pd[0]),
                machine.delta.calcXYZ(pd[2]),
                machine.delta.calcXYZ(pd[4])
            )) {
        return STATUS_CAL_BED;
    }
    TESTCOUT3("zpl0 a:", zpl0.a, " b:", zpl0.b, " c:", zpl0.c);
    ZPlane zpl60;
    if (!zpl60.initialize(
                machine.delta.calcXYZ(pd[1]),
                machine.delta.calcXYZ(pd[3]),
                machine.delta.calcXYZ(pd[5])
            )) {
        return STATUS_CAL_BED;
    }
    TESTCOUT3("zpl60 a:", zpl60.a, " b:", zpl60.b, " c:", zpl60.c);
    bed.a = (zpl0.a + zpl60.b) / 2;
    bed.b = (zpl0.b + zpl60.b) / 2;
    bed.c = (zpl0.c + zpl60.c) / 2;
    TESTCOUT3("bed a:", bed.a, " b:", bed.b, " c:", bed.c);
    machine.bed.a = bed.a = saveWeight*bed.a + (1-saveWeight) * machine.bed.a;
    machine.bed.b = bed.b = saveWeight*bed.b + (1-saveWeight) * machine.bed.b;
    machine.bed.c = bed.c = saveWeight*bed.c + (1-saveWeight) * machine.bed.c;
    TESTCOUT3("machine.bed a:", machine.bed.a, " b:", machine.bed.b, " c:", machine.bed.c);

    return STATUS_OK;
}

/////////////////////////// FPDMoveTo ///////////////
typedef class FPDMoveTo {
private:
    int32_t nLoops;
    Quad<PH5TYPE> destination;
    int16_t nSegs;
    bool isZBed;
    bool isRaw; // MTO_RAW movement requested
    Machine &machine;
    FPDController &controller;

private:
    Status execute(JsonCommand& jcmd, JsonObject *pjobj);
    Status movePulleyArmToAngle(DeltaAxis axis, PH5TYPE degrees);

public:
    FPDMoveTo(FPDController &controller, Machine& machine);
    Status process(JsonCommand& jcmd, JsonObject& jobj, const char* key);
} FPDMoveTo;

FPDMoveTo::FPDMoveTo(FPDController &controller, Machine& machine)
    : nLoops(0), nSegs(0), machine(machine), controller(controller), isZBed(false), isRaw(false)
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
    if (isRaw) {
        TESTCOUT1("execute:", "isRaw");
        return STATUS_OK;
    }
    PH5TYPE x = destination.value[0];
    PH5TYPE y = destination.value[1];
    PH5TYPE z = destination.value[2];
    if (isZBed) {
        PH5TYPE zb = machine.bed.calcZ(x,y);
        z += zb;
    }
    StrokeBuilder sb(machine.vMax, machine.tvMax);
    Quad<StepCoord> curPos = machine.getMotorPosition();
    Quad<StepCoord> dPos;
    XYZ3D xyz(x, y, z);
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
    if (dPos.isZero()) {
        TESTCOUT1("execute:","dPos.isZero()");
    } else {
        status = sb.buildLine(machine.stroke, dPos);
        if (status != STATUS_OK) {
            return status;
        }
        Ticks tStrokeStart = ticks()-1; // ensure that we always move
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

Status FPDMoveTo::movePulleyArmToAngle(DeltaAxis axis, PH5TYPE degrees) {
    Quad<StepCoord> posStart = machine.getMotorPosition();
    Quad<StepCoord> posEnd = posStart;
    PH5TYPE dpp = machine.delta.getDegreesPerPulse(axis);
    if (axis == DELTA_AXIS_ALL) {
        return STATUS_AXIS_ERROR;
    }
    posEnd.value[axis] = machine.delta.roundStep(degrees / dpp);
    PinType pinProbe = machine.op.probe.pinProbe; // save syspb
    machine.op.probe.pinProbe = machine.axis[axis].pinMin;
    machine.op.probe.setup(posStart, posEnd);
    Status status = STATUS_BUSY_CALIBRATING;
    int32_t stopTicks = ticks() + MS_TICKS(20000); // stop after 20 seconds

    while (ticks() < stopTicks && status == STATUS_BUSY_CALIBRATING) {
        status = machine.probe(status, 0);
    }
    machine.op.probe.pinProbe = pinProbe; // restore syspb
    if (status == STATUS_PROBE_FAILED) {
        isRaw = true;
        return STATUS_OK; // we're not really probing, so this is good
    } else if (status == STATUS_OK) {
        return STATUS_LIMIT_MIN; // uh oh
    }
    return status;
}

Status FPDMoveTo::process(JsonCommand& jcmd, JsonObject& jobj, const char* key) {
    Status status = STATUS_OK;
    size_t keyLen = strlen(key);
    const char *s;

    if (strcmp_PS(OP_mov, key) == 0) {
        if ((s = jobj[key]) && *s == 0) {
            JsonObject& node = jobj.createNestedObject(key);
            jcmd.addQueryAttr(node, OP_lp);
            jcmd.addQueryAttr(node, OP_mv);
            jcmd.addQueryAttr(node, OP_pp);
            jcmd.addQueryAttr(node, OP_sg);
            jcmd.addQueryAttr(node, OP_tp);
            jcmd.addQueryAttr(node, OP_ts);
            if (machine.getMotorAxis(0).isEnabled()) {
                jcmd.addQueryAttr(node, OP_1);
            }
            if (machine.getMotorAxis(1).isEnabled()) {
                jcmd.addQueryAttr(node, OP_2);
            }
            if (machine.getMotorAxis(2).isEnabled()) {
                jcmd.addQueryAttr(node, OP_3);
            }
            if (machine.getMotorAxis(3).isEnabled()) {
                jcmd.addQueryAttr(node, OP_4);
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
    } else if (strcmp_PS(OP_movwp,key) == 0 || strcmp_PS(OP_wp,key) == 0) {
        int16_t iMark = ((int16_t)jobj[key]) - 1;
        if (iMark < 0 || MARK_COUNT <= iMark) {
            return jcmd.setError(STATUS_MARK_INDEX, key);
        }
        destination.value[0] = machine.marks[iMark%MARK_COUNT];
        iMark++;
        destination.value[1] = machine.marks[iMark%MARK_COUNT];
        iMark++;
        destination.value[2] = machine.marks[iMark%MARK_COUNT];
        iMark++;
        if (keyLen > 2) {
            status = execute(jcmd, NULL);
        }
    } else if (strcmp_PS(OP_mova1,key) == 0 || strcmp_PS(OP_a1,key) == 0) {
        return movePulleyArmToAngle(DELTA_AXIS_1, (PH5TYPE) jobj[key]);
    } else if (strcmp_PS(OP_mova2,key) == 0 || strcmp_PS(OP_a2,key) == 0) {
        return movePulleyArmToAngle(DELTA_AXIS_2, (PH5TYPE) jobj[key]);
    } else if (strcmp_PS(OP_mova3,key) == 0 || strcmp_PS(OP_a3,key) == 0) {
        return movePulleyArmToAngle(DELTA_AXIS_3, (PH5TYPE) jobj[key]);
    } else if (strcmp_PS(OP_movxm,key) == 0 || strcmp_PS(OP_xm,key) == 0) {
        int16_t iMark = ((int16_t)jobj[key]) - 1;
        if (iMark < 0 || MARK_COUNT <= iMark) {
            return jcmd.setError(STATUS_MARK_INDEX, key);
        }
        destination.value[0] = machine.marks[iMark];
        if (keyLen > 2) {
            status = execute(jcmd, NULL);
        }
    } else if (strcmp_PS(OP_movym,key) == 0 || strcmp_PS(OP_ym,key) == 0) {
        int16_t iMark = ((int16_t)jobj[key]) - 1;
        if (iMark < 0 || MARK_COUNT <= iMark) {
            return jcmd.setError(STATUS_MARK_INDEX, key);
        }
        destination.value[1] = machine.marks[iMark];
        if (keyLen > 2) {
            status = execute(jcmd, NULL);
        }
    } else if (strcmp_PS(OP_movzm,key) == 0 || strcmp_PS(OP_zm,key) == 0) {
        int16_t iMark = ((int16_t)jobj[key]) - 1;
        if (iMark < 0 || MARK_COUNT <= iMark) {
            return jcmd.setError(STATUS_MARK_INDEX, key);
        }
        TESTCOUT2("z:", destination.value[2], " mark:", machine.marks[iMark]);
        destination.value[2] = machine.marks[iMark];
        if (keyLen > 2) {
            status = execute(jcmd, NULL);
        }
    } else if (strcmp_PS(OP_movzb,key) == 0) {
        XYZ3D xyz = controller.getXYZ3D();
        PH5TYPE value = xyz.z - machine.bed.c;
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, value);
        if (status == STATUS_OK) {
            PH5TYPE zBed = machine.bed.calcZ(xyz.x,xyz.y);
            destination.value[2] = zBed + value;
            TESTCOUT3("movzb z:", destination.value[2], " zBed:", zBed, " value:", value);
            status = execute(jcmd, NULL);
        }
    } else if (strcmp_PS(OP_zb,key) == 0) {
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, destination.value[2]);
        if (status == STATUS_OK) {
            isZBed = true;
            TESTCOUT1("mov zb:", destination.value[2]);
        }
    } else if (strcmp_PS(OP_movxr,key) == 0 || strcmp_PS(OP_xr,key) == 0) {
        XYZ3D xyz = controller.getXYZ3D();
        PH5TYPE x = 0;
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, x);
        if (status == STATUS_OK) {
            destination.value[0] = xyz.x + x;
            if (keyLen > 2) {
                status = execute(jcmd, NULL);
            }
        }
    } else if (strcmp_PS(OP_movyr,key) == 0 || strcmp_PS(OP_yr,key) == 0) {
        XYZ3D xyz = controller.getXYZ3D();
        PH5TYPE y = 0;
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, y);
        if (status == STATUS_OK) {
            destination.value[1] = xyz.y + y;
            if (keyLen > 2) {
                status = execute(jcmd, NULL);
            }
        }
    } else if (strcmp_PS(OP_movzr,key) == 0 || strcmp_PS(OP_zr,key) == 0) {
        XYZ3D xyz = controller.getXYZ3D();
        PH5TYPE z = 0;
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, z);
        if (status == STATUS_OK) {
            destination.value[2] = xyz.z + z;
            if (keyLen > 2) {
                status = execute(jcmd, NULL);
            }
        }
    } else if (strlen(key)==4 && strncmp("mov", key, 3) == 0) { // short form
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
    } else if (strcmp_PS(OP_d, key) == 0) {
        char keyword[10];
        strcpy_P(keyword, OP_angle);
        if (!jobj.at(keyword).success()) {
            return jcmd.setError(STATUS_FIELD_REQUIRED,"a");
        }
    } else if (strcmp_PS(OP_angle, key) == 0) {
        // polar CCW from X-axis around X0Y0
        if (!jobj.at("d").success()) {
            return jcmd.setError(STATUS_FIELD_REQUIRED,"d");
        }
        PH5TYPE d = jobj["d"];
        char keyword[10];
        strcpy_P(keyword, OP_angle);
        PH5TYPE a = jobj[keyword];
        PH5TYPE radians = a * PI / 180.0;
        PH5TYPE y = d * sin(radians);
        PH5TYPE x = d * cos(radians);
        TESTCOUT2("x:", x, " y:", y);
        destination.value[0] = x;
        destination.value[1] = y;
    } else if (strcmp_PS(OP_lp, key) == 0) {
        // output variable
    } else if (strcmp_PS(OP_mv, key) == 0) {
        status = processField<int32_t, int32_t>(jobj, key, machine.vMax);
    } else if (strcmp_PS(OP_pp, key) == 0) {
        // output variable
    } else if (strcmp_PS(OP_sg, key) == 0) {
        status = processField<int16_t, int32_t>(jobj, key, nSegs);
    } else if (strcmp_PS(OP_ts, key) == 0) {
        // output variable
    } else if (strcmp_PS(OP_tp, key) == 0) {
        // output variable
    } else if (strcmp_PS(OP_tv, key) == 0) {
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
            jcmd.addQueryAttr(node, OP_1);
            jcmd.addQueryAttr(node, OP_2);
            jcmd.addQueryAttr(node, OP_3);
            jcmd.addQueryAttr(node, OP_4);
            jcmd.addQueryAttr(node, OP_x);
            jcmd.addQueryAttr(node, OP_y);
            jcmd.addQueryAttr(node, OP_z);
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
    } else if (strcmp_PS(OP_1, axisStr) == 0) {
        status = processField<StepCoord, int32_t>(jobj, key, machine.axis[0].position);
    } else if (strcmp_PS(OP_2, axisStr) == 0) {
        status = processField<StepCoord, int32_t>(jobj, key, machine.axis[1].position);
    } else if (strcmp_PS(OP_3, axisStr) == 0) {
        status = processField<StepCoord, int32_t>(jobj, key, machine.axis[2].position);
    } else if (strcmp_PS(OP_4, axisStr) == 0) {
        status = processField<StepCoord, int32_t>(jobj, key, machine.axis[3].position);
    } else if (strcmp_PS(OP_x, axisStr) == 0) {
        XYZ3D xyz(getXYZ3D());
        if (!xyz.isValid()) {
            return jcmd.setError(STATUS_KINEMATIC_XYZ, key);
        }
        PH5TYPE value = xyz.x;
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, value);
        if (value != xyz.x) {
            status = jcmd.setError(STATUS_OUTPUT_FIELD, key);
        }
    } else if (strcmp_PS(OP_y, axisStr) == 0) {
        XYZ3D xyz(getXYZ3D());
        if (!xyz.isValid()) {
            return jcmd.setError(STATUS_KINEMATIC_XYZ, key);
        }
        PH5TYPE value = xyz.y;
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, value);
        if (value != xyz.y) {
            status = jcmd.setError(STATUS_OUTPUT_FIELD, key);
        }
    } else if (strcmp_PS(OP_z, axisStr) == 0) {
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
    if (strcmp_PS(OP_prb, key) == 0) {
        if ((s = jobj[key]) && *s == 0) {
            JsonObject& node = jobj.createNestedObject(key);
            xyzEnd.z = machine.delta.getMinZ(xyzEnd.z, xyzEnd.y);
            jcmd.addQueryAttr(node, OP_1);
            jcmd.addQueryAttr(node, OP_2);
            jcmd.addQueryAttr(node, OP_3);
            jcmd.addQueryAttr(node, OP_4);
            jcmd.addQueryAttr(node, OP_ip);
            node["pb"] = machine.op.probe.pinProbe;
            jcmd.addQueryAttr(node, OP_sd);
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
                return jcmd.setError(STATUS_FIELD_REQUIRED, "pb");
            }
        }
    } else if (strcmp_PS(OP_prbip, key) == 0 || strcmp_PS(OP_ip, key) == 0) {
        status = processField<bool, bool>(jobj, key, machine.op.probe.invertProbe);
    } else if (strcmp_PS(OP_prbpb, key) == 0 || strcmp_PS(OP_pb, key) == 0) {
        status = processField<PinType, int32_t>(jobj, key, machine.op.probe.pinProbe);
    } else if (strcmp_PS(OP_prbpn, key) == 0 || strcmp_PS(OP_pn, key) == 0) { // legacy
        status = processField<PinType, int32_t>(jobj, key, machine.op.probe.pinProbe); // legacy
    } else if (strcmp_PS(OP_prbsd, key) == 0 || strcmp_PS(OP_sd, key) == 0) {
        status = processField<DelayMics, int32_t>(jobj, key, machine.searchDelay);
    } else if (strcmp_PS(OP_prbx, key) == 0 || strcmp_PS(OP_x, key) == 0) {
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, xyzEnd.x);
    } else if (strcmp_PS(OP_prby, key) == 0 || strcmp_PS(OP_y, key) == 0) {
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, xyzEnd.y);
    } else if (strcmp_PS(OP_prbz, key) == 0 || strcmp_PS(OP_z, key) == 0) {
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
    if (strcmp_PS(OP_prx,key) == 0 || strcmp_PS(OP_x,key) == 0) {
        jobj[key] = xyz.x;
    } else if (strcmp_PS(OP_prby,key) == 0 || strcmp_PS(OP_y,key) == 0) {
        jobj[key] = xyz.y;
    } else if (strcmp_PS(OP_prbz,key) == 0 || strcmp_PS(OP_z,key) == 0) {
        jobj[key] = xyz.z;
    } else if (strcmp_PS(OP_1,key) == 0) {
        jobj[key] = machine.getMotorAxis(0).position;
    } else if (strcmp_PS(OP_2,key) == 0) {
        jobj[key] = machine.getMotorAxis(1).position;
    } else if (strcmp_PS(OP_3,key) == 0) {
        jobj[key] = machine.getMotorAxis(2).position;
    } else if (strcmp_PS(OP_4,key) == 0) {
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
    if (strcmp_PS(OP_dim, key) == 0) {
        const char *s;
        if ((s = jobj[key]) && *s == 0) {
            JsonObject& node = jobj.createNestedObject(key);
            jcmd.addQueryAttr(node, OP_bx);
            jcmd.addQueryAttr(node, OP_by);
            jcmd.addQueryAttr(node, OP_bz);
            jcmd.addQueryAttr(node, OP_e);
            jcmd.addQueryAttr(node, OP_f);
            jcmd.addQueryAttr(node, OP_gr);
            jcmd.addQueryAttr(node, OP_ha);
            jcmd.addQueryAttr(node, OP_hp);
            jcmd.addQueryAttr(node, OP_mi);
            jcmd.addQueryAttr(node, OP_re);
            jcmd.addQueryAttr(node, OP_rf);
            jcmd.addQueryAttr(node, OP_spa);
            jcmd.addQueryAttr(node, OP_sps);
            jcmd.addQueryAttr(node, OP_st);
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
    } else if (strcmp_PS(OP_bx, key) == 0 || strcmp_PS(OP_dimbx, key) == 0) {
        PH5TYPE value = machine.bed.a;
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, value);
        machine.bed.a = value;
        jobj[key].set(value,4);
    } else if (strcmp_PS(OP_by, key) == 0 || strcmp_PS(OP_dimby, key) == 0) {
        PH5TYPE value = machine.bed.b;
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, value);
        machine.bed.b = value;
        jobj[key].set(value,4);
    } else if (strcmp_PS(OP_bz, key) == 0 || strcmp_PS(OP_dimbz, key) == 0) {
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, machine.bed.c);
    } else if (strcmp_PS(OP_e, key) == 0 || strcmp_PS(OP_dime, key) == 0) {
        PH5TYPE value = machine.delta.getEffectorTriangleSide();
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, value);
        machine.delta.setEffectorTriangleSide(value);
    } else if (strcmp_PS(OP_f, key) == 0 || strcmp_PS(OP_dimf, key) == 0) {
        PH5TYPE value = machine.delta.getBaseTriangleSide();
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, value);
        machine.delta.setBaseTriangleSide(value);
    } else if (strcmp_PS(OP_gr, key) == 0 || strcmp_PS(OP_dimgr, key) == 0) {
        PH5TYPE value = machine.delta.getGearRatio();
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, value);
        machine.delta.setGearRatio(value);
    } else if (strcmp_PS(OP_gr1, key) == 0 || strcmp_PS(OP_dimgr1, key) == 0) {
        PH5TYPE value = machine.delta.getGearRatio(DELTA_AXIS_1);
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, value);
        machine.delta.setGearRatio(value, DELTA_AXIS_1);
    } else if (strcmp_PS(OP_gr2, key) == 0 || strcmp_PS(OP_dimgr2, key) == 0) {
        PH5TYPE value = machine.delta.getGearRatio(DELTA_AXIS_2);
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, value);
        machine.delta.setGearRatio(value, DELTA_AXIS_2);
    } else if (strcmp_PS(OP_gr3, key) == 0 || strcmp_PS(OP_dimgr3, key) == 0) {
        PH5TYPE value = machine.delta.getGearRatio(DELTA_AXIS_3);
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, value);
        machine.delta.setGearRatio(value, DELTA_AXIS_3);
    } else if (strcmp_PS(OP_ha, key) == 0 || strcmp_PS(OP_dimha, key) == 0) {
        PH5TYPE homeAngle = machine.getHomeAngle();
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, homeAngle);
        machine.setHomeAngle(homeAngle);
    } else if (strcmp_PS(OP_hp, key) == 0 || strcmp_PS(OP_dimhp, key) == 0) {
        StepCoord homePulses = machine.getHomePulses();
        status = processField<StepCoord, StepCoord>(jobj, key, homePulses);
        machine.setHomePulses(homePulses);
    } else if (strcmp_PS(OP_mi, key) == 0 || strcmp_PS(OP_dimmi, key) == 0) {
        int16_t value = machine.delta.getMicrosteps();
        status = processField<int16_t, int16_t>(jobj, key, value);
        machine.delta.setMicrosteps(value);
    } else if (strcmp_PS(OP_pd, key) == 0 || strcmp_PS(OP_dimpd, key) == 0) {
        status = processProbeData(jcmd, jobj, key);
    } else if (strcmp_PS(OP_re, key) == 0 || strcmp_PS(OP_dimre, key) == 0) {
        PH5TYPE value = machine.delta.getEffectorLength();
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, value);
        machine.delta.setEffectorLength(value);
    } else if (strcmp_PS(OP_rf, key) == 0 || strcmp_PS(OP_dimrf, key) == 0) {
        PH5TYPE value = machine.delta.getBaseArmLength();
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, value);
        machine.delta.setBaseArmLength(value);
    } else if (strcmp_PS(OP_spa, key) == 0 || strcmp_PS(OP_dimspa, key) == 0) {
        PH5TYPE value = machine.delta.getSPAngle();
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, value);
        machine.delta.setSPAngle(value);
    } else if (strcmp_PS(OP_sps, key) == 0 || strcmp_PS(OP_dimsps, key) == 0) {
        PH5TYPE value = machine.delta.getSPSlope();
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, value);
        machine.delta.setSPSlope(value);
    } else if (strcmp_PS(OP_st, key) == 0 || strcmp_PS(OP_dimst, key) == 0) {
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
    if (strcmp_PS(OP_hom, key) == 0) {
        const char *s;
        if ((s = jobj[key]) && *s == 0) {
            JsonObject& node = jobj.createNestedObject(key);
			node["1"] = 
			node["2"] =
			node["3"] =
			node["4"] = ""; // DO NOT USE jcmd.addQueryAttr(...)
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

Status FPDController::finalizeHome(JsonCommand& jcmd, JsonObject& jobj, const char * key) {
    Status status = STATUS_OK;

    if (strcmp_PS(OP_hom, key) != 0) {
        return status; // single axis home does not reposition to post-home destination
    }

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
        TESTCOUT1("finalizeHome:", "COLLISION");
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
            status = finalizeHome(jcmd, jobj, key);
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
    Status status = processCalibrateCore(jcmd, jobj, key, cal, false); // get saveWeight
    if (status == STATUS_OK) {
        status = cal.calibrate();
    }
    if (status == STATUS_OK) {
        status = processCalibrateCore(jcmd, jobj, key, cal, true); // save results
    }
    return status;
}

Status FPDController::processCalibrateCore(JsonCommand &jcmd, JsonObject& jobj, const char* key,
        FPDCalibrateHome &cal, bool output)
{
    Status status = STATUS_OK;
    const char *s;
    if (strcmp_PS(OP_cal, key) == 0) {
        if ((s = jobj[key]) && *s == 0) {
            JsonObject& node = jobj.createNestedObject(key);
            jcmd.addQueryAttr(node, OP_bx);
            jcmd.addQueryAttr(node, OP_by);
            jcmd.addQueryAttr(node, OP_bz);
            jcmd.addQueryAttr(node, OP_gr);
            jcmd.addQueryAttr(node, OP_ge);
            jcmd.addQueryAttr(node, OP_ha);
            jcmd.addQueryAttr(node, OP_he);
            jcmd.addQueryAttr(node, OP_sv);
            jcmd.addQueryAttr(node, OP_zc);
            jcmd.addQueryAttr(node, OP_zr);
        }
        JsonObject& kidObj = jobj[key];
        if (!kidObj.success()) {
            return jcmd.setError(STATUS_JSON_OBJECT, key);
        }
        for (JsonObject::iterator it = kidObj.begin(); it != kidObj.end(); ++it) {
            status = processCalibrateCore(jcmd, kidObj, it->key, cal, output);
            if (status != STATUS_OK) {
                TESTCOUT1("processCalibrate status:", status);
                return status;
            }
        }
    } else if (strcmp_PS(OP_calbx,key) == 0 || strcmp_PS(OP_bx,key) == 0) {
        if (output) {
            PH5TYPE value = machine.bed.a;
            status = processField<PH5TYPE, PH5TYPE>(jobj, key, value);
            if (value != cal.bed.a) {
                return jcmd.setError(STATUS_OUTPUT_FIELD, key);
            }
            jobj[key].set(value, 4);
        }
    } else if (strcmp_PS(OP_calby,key) == 0 || strcmp_PS(OP_by,key) == 0) {
        if (output) {
            PH5TYPE value = machine.bed.b;
            status = processField<PH5TYPE, PH5TYPE>(jobj, key, value);
            if (value != cal.bed.b) {
                return jcmd.setError(STATUS_OUTPUT_FIELD, key);
            }
            jobj[key].set(value, 4);
        }
    } else if (strcmp_PS(OP_calbz,key) == 0 || strcmp_PS(OP_bz,key) == 0) {
        if (output) {
            PH5TYPE value = machine.bed.c;
            status = processField<PH5TYPE, PH5TYPE>(jobj, key, value);
            if (value != cal.bed.c) {
                return jcmd.setError(STATUS_OUTPUT_FIELD, key);
            }
        }
    } else if (strcmp_PS(OP_calgr,key) == 0 || strcmp_PS(OP_gr,key) == 0) {
        if (output) {
            PH5TYPE value = machine.delta.getGearRatio();
            status = processField<PH5TYPE, PH5TYPE>(jobj, key, value);
            if (value != machine.delta.getGearRatio()) {
                return jcmd.setError(STATUS_OUTPUT_FIELD, key);
            }
        }
        cal.mode = (CalibrateMode)((cal.mode&CAL_HOME) | CAL_GEAR);
    } else if (strcmp_PS(OP_calgr1,key) == 0 || strcmp_PS(OP_gr1,key) == 0) {
        PH5TYPE degrees = 0;
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, degrees);
        if (!degrees) {
            return jcmd.setError(STATUS_CAL_DEGREES, key);
        }
        if (machine.axis[0].position == 0) {
            return jcmd.setError(STATUS_CAL_POSITION_0, key);
        }
        PH5TYPE dpp = abs(degrees / machine.axis[0].position);
        machine.delta.setDegreesPerPulse(dpp, DELTA_AXIS_1);
        cal.mode = CAL_GEAR1;
    } else if (strcmp_PS(OP_calgr2,key) == 0 || strcmp_PS(OP_gr2,key) == 0) {
        PH5TYPE degrees = 0;
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, degrees);
        if (!degrees) {
            return jcmd.setError(STATUS_CAL_DEGREES, key);
        }
        if (machine.axis[1].position == 0) {
            return jcmd.setError(STATUS_CAL_POSITION_0, key);
        }
        PH5TYPE dpp = abs(degrees / machine.axis[1].position);
        machine.delta.setDegreesPerPulse(dpp, DELTA_AXIS_2);
        cal.mode = CAL_GEAR2;
    } else if (strcmp_PS(OP_calgr3,key) == 0 || strcmp_PS(OP_gr3,key) == 0) {
        PH5TYPE degrees = 0;
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, degrees);
        if (!degrees) {
            return jcmd.setError(STATUS_CAL_DEGREES, key);
        }
        if (machine.axis[2].position == 0) {
            return jcmd.setError(STATUS_CAL_POSITION_0, key);
        }
        PH5TYPE dpp = abs(degrees / machine.axis[2].position);
        machine.delta.setDegreesPerPulse(dpp, DELTA_AXIS_3);
        cal.mode = CAL_GEAR3;
    } else if (strcmp_PS(OP_calge,key) == 0 || strcmp_PS(OP_ge,key) == 0) {
        if (output) {
            PH5TYPE value = cal.eGear;
            status = processField<PH5TYPE, PH5TYPE>(jobj, key, value);
            if (value != cal.eGear) {
                return jcmd.setError(STATUS_OUTPUT_FIELD, key);
            }
        }
        cal.mode = (CalibrateMode)((cal.mode&CAL_HOME) | CAL_GEAR);
    } else if (strcmp_PS(OP_calha,key) == 0 || strcmp_PS(OP_ha,key) == 0) {
        if (output) {
            PH5TYPE value = machine.getHomeAngle();
            status = processField<PH5TYPE, PH5TYPE>(jobj, key, value);
            if (value != machine.getHomeAngle()) {
                return jcmd.setError(STATUS_OUTPUT_FIELD, key);
            }
        }
        cal.mode = (CalibrateMode)((cal.mode&CAL_GEAR) | CAL_HOME);
    } else if (strcmp_PS(OP_calhe,key) == 0 || strcmp_PS(OP_he,key) == 0) {
        if (output) {
            PH5TYPE value = cal.eTheta;
            status = processField<PH5TYPE, PH5TYPE>(jobj, key, value);
            if (value != cal.eTheta) {
                return jcmd.setError(STATUS_OUTPUT_FIELD, key);
            }
        }
        cal.mode = (CalibrateMode)((cal.mode&CAL_GEAR) | CAL_HOME);
    } else if (strcmp_PS(OP_calzc,key) == 0 || strcmp_PS(OP_zc,key) == 0) {
        if (output) {
            PH5TYPE value = cal.zCenter;
            status = processField<PH5TYPE, PH5TYPE>(jobj, key, value);
            if (value != cal.zCenter) {
                return jcmd.setError(STATUS_OUTPUT_FIELD, key);
            }
        }
    } else if (strcmp_PS(OP_calzr,key) == 0 || strcmp_PS(OP_zr,key) == 0) {
        if (output) {
            PH5TYPE value = cal.zRim;
            status = processField<PH5TYPE, PH5TYPE>(jobj, key, value);
            if (value != cal.zRim) {
                return jcmd.setError(STATUS_OUTPUT_FIELD, key);
            }
        }
    } else if (strcmp_PS(OP_calsv,key) == 0 || strcmp_PS(OP_sv,key) == 0) {
        if (jobj.at(key).is<bool>()) {
            bool save = true;
            status = processField<bool, bool>(jobj, key, save);
            cal.saveWeight = save ? 0.3 : 0;
        } else {
            status = processField<PH5TYPE, PH5TYPE>(jobj, key, cal.saveWeight);
        }
    } else {
        return jcmd.setError(STATUS_UNRECOGNIZED_NAME, key);
    }
    return status;
}

void FPDController::onTopologyChanged() {
    machine.setHomeAngle(machine.getHomeAngle()); // sync axes to homeAngle
    machine.delta.useEffectorOrigin();
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

Status FPDController::processMark(JsonCommand& jcmd, JsonObject& jobj, const char* key) {
    Status status = STATUS_OK;
    if (strcmp_PS(OP_wp, key) == 0 || strcmp_PS(OP_mrkwp, key) == 0) {
        int16_t iMark = ((int16_t)jobj[key]) - 1;
        if (iMark < 0 || MARK_COUNT <= iMark) {
            TESTCOUT1("mark index:", iMark);
            return jcmd.setError(STATUS_MARK_INDEX, key);
        }
        machine.loadDeltaCalculator();
        XYZ3D xyz = getXYZ3D();
        if (!xyz.isValid()) {
            return jcmd.setError(STATUS_KINEMATIC_XYZ, key);
        }
        TESTCOUT3("processMark x:", xyz.x, " y:", xyz.y, " z:", xyz.z);
        machine.marks[iMark%MARK_COUNT] = xyz.x;
        iMark++;
        machine.marks[iMark%MARK_COUNT] = xyz.y;
        iMark++;
        machine.marks[iMark%MARK_COUNT] = xyz.z;
        iMark++;
    } else if (strcmp_PS(OP_a1, key) == 0 || strcmp_PS(OP_mrka1, key) == 0) {
        int16_t iMark = ((int16_t)jobj[key]) - 1;
        if (iMark < 0 || MARK_COUNT <= iMark) {
            TESTCOUT1("mark index:", iMark);
            return jcmd.setError(STATUS_MARK_INDEX, key);
        }
        machine.marks[iMark] = machine.axis[0].position;
    } else if (strcmp_PS(OP_a2, key) == 0 || strcmp_PS(OP_mrka2, key) == 0) {
        int16_t iMark = ((int16_t)jobj[key]) - 1;
        if (iMark < 0 || MARK_COUNT <= iMark) {
            TESTCOUT1("mark index:", iMark);
            return jcmd.setError(STATUS_MARK_INDEX, key);
        }
        machine.marks[iMark] = machine.axis[1].position;
    } else if (strcmp_PS(OP_a3, key) == 0 || strcmp_PS(OP_mrka3, key) == 0) {
        int16_t iMark = ((int16_t)jobj[key]) - 1;
        if (iMark < 0 || MARK_COUNT <= iMark) {
            TESTCOUT1("mark index:", iMark);
            return jcmd.setError(STATUS_MARK_INDEX, key);
        }
        machine.marks[iMark] = machine.axis[2].position;
    } else if (strcmp_PS(OP_ax, key) == 0 || strcmp_PS(OP_mrkax, key) == 0) {
        int16_t iMark = ((int16_t)jobj[key]) - 1;
        if (iMark < 0 || MARK_COUNT <= iMark) {
            TESTCOUT1("mark index:", iMark);
            return jcmd.setError(STATUS_MARK_INDEX, key);
        }
        machine.loadDeltaCalculator();
        XYZ3D xyz = getXYZ3D();
        if (!xyz.isValid()) {
            return jcmd.setError(STATUS_KINEMATIC_XYZ, key);
        }
        TESTCOUT1("processMark x:", xyz.x);
        machine.marks[iMark] = xyz.x;
    } else if (strcmp_PS(OP_ay, key) == 0 || strcmp_PS(OP_mrkay, key) == 0) {
        int16_t iMark = ((int16_t)jobj[key]) - 1;
        if (iMark < 0 || MARK_COUNT <= iMark) {
            TESTCOUT1("mark index:", iMark);
            return jcmd.setError(STATUS_MARK_INDEX, key);
        }
        machine.loadDeltaCalculator();
        XYZ3D xyz = getXYZ3D();
        if (!xyz.isValid()) {
            return jcmd.setError(STATUS_KINEMATIC_XYZ, key);
        }
        machine.marks[iMark] = xyz.y;
    } else if (strcmp_PS(OP_az, key) == 0 || strcmp_PS(OP_mrkaz, key) == 0) {
        int16_t iMark = ((int16_t)jobj[key]) - 1;
        if (iMark < 0 || MARK_COUNT <= iMark) {
            TESTCOUT1("mark index:", iMark);
            return jcmd.setError(STATUS_MARK_INDEX, key);
        }
        machine.loadDeltaCalculator();
        XYZ3D xyz = getXYZ3D();
        if (!xyz.isValid()) {
            return jcmd.setError(STATUS_KINEMATIC_XYZ, key);
        }
        machine.marks[iMark] = xyz.z;
    } else {
        return JsonController::processMark(jcmd, jobj, key);
    }
    return status;
}


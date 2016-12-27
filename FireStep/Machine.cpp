#include <Arduino.h>
#ifdef CMAKE
#include <cstring>
#include <cmath>
#endif
#include "FireUtils.h"
#include "Machine.h"
#include "version.h"
#include "ProgMem.h"

using namespace firestep;

template class Quad<int16_t>;
template class Quad<int32_t>;

TESTDECL(int32_t, firestep::delayMicsTotal = 0);

char * firestep::saveConfigValue(const char *key, const char *value, char *out) {
    sprintf(out, "\"%s\":%s,", key, value);
	//TESTCOUT2("saveConfigValue char* key:", key, " value:", value);
    return out+strlen(out);
}

char * firestep::saveConfigValue(const char *key, bool value, char *out) {
    return saveConfigValue(key, value ? "1":"0", out);
}

char * firestep::saveConfigValue(const char *key, uint8_t value, char *out) {
    sprintf(out, "\"%s\":%d,", key, value);
	//TESTCOUT2("saveConfigValue int16 key:", key, " value:", value);
    return out+strlen(out);
}

char * firestep::saveConfigValue(const char *key, int16_t value, char *out) {
    sprintf(out, "\"%s\":%d,", key, value);
	//TESTCOUT2("saveConfigValue int16 key:", key, " value:", value);
    return out+strlen(out);
}

char * firestep::saveConfigValue(const char *key, int32_t value, char *out) {
    sprintf(out, "\"%s\":%ld,", key, (long) value);
	//TESTCOUT2("saveConfigValue int32 key:", key, " value:", value);
    return out+strlen(out);
}

char * firestep::saveConfigValue(const char *key, PH5TYPE value, char *out, uint8_t places) {
    bool minus = (value < 0);
    if (minus) {
        value = -value;
    }
    int32_t ivalue = value;
    //TESTCOUT3("key:", key, " value:", value, " ivalue:", ivalue);
    if (minus) {
        sprintf(out, "\"%s\":-%ld,", key, (long) ivalue);
    } else {
        sprintf(out, "\"%s\":%ld,", key, (long) ivalue);
    }
    out += strlen(out);
    out--;
    *out++ = '.';
    value -= ivalue;
    switch (places) {
    case 5:
        ivalue = value * 100000 + 0.5;
        out[4] = '0' + (ivalue % 10);
        ivalue /= 10;
        out[3] = '0' + (ivalue % 10);
        ivalue /= 10;
        out[2] = '0' + (ivalue % 10);
        ivalue /= 10;
        out[1] = '0' + (ivalue % 10);
        ivalue /= 10;
        out[0] = '0' + (ivalue % 10);
        out += 5;
        break;
    case 4:
        ivalue = value * 10000 + 0.5;
        out[3] = '0' + (ivalue % 10);
        ivalue /= 10;
        out[2] = '0' + (ivalue % 10);
        ivalue /= 10;
        out[1] = '0' + (ivalue % 10);
        ivalue /= 10;
        out[0] = '0' + (ivalue % 10);
        out += 4;
        break;
    case 3:
        ivalue = value * 1000 + 0.5;
        out[2] = '0' + (ivalue % 10);
        ivalue /= 10;
        out[1] = '0' + (ivalue % 10);
        ivalue /= 10;
        out[0] = '0' + (ivalue % 10);
        out += 3;
        break;
    case 2:
    default:
        ivalue = value * 100 + 0.5;
        out[1] = '0' + (ivalue % 10);
        ivalue /= 10;
        out[0] = '0' + (ivalue % 10);
        out += 2;
        break;
    case 1:
        ivalue = value * 10 + 0.5;
        out[0] = '0' + (ivalue % 10);
        out += 1;
        break;
    }
    *out++ = ',';
    *out = 0;

    return out;
}

/////////////////////////// Axis ///////////////////////

Status Axis::enable(bool active) {
    if (pinEnable == NOPIN || pinStep == NOPIN || pinDir == NOPIN) {
        //TESTCOUT2("Axis::enable(", active, ") IGNORED axis:", id);
        return STATUS_NOPIN;
    }
    //TESTCOUT2("Axis::enable(", active, ") axis:", id);
    fireduino::digitalWrite(pinEnable, active ? PIN_ENABLE : PIN_DISABLE);
    setAdvancing(true);
    enabled = active;
    return STATUS_OK;
}

#define BIT_HASH ((int32_t)0x3)
int32_t Axis::hash() {
    int32_t result = 0
                     ^ (dirHIGH ? (BIT_HASH << 0) : 0)
                     ^ (enabled ? (BIT_HASH << 1) : 0)
                     ^ ((int32_t) home << 0)
                     ^ ((int32_t) idleSnooze << 1)
                     ^ ((int32_t) microsteps << 2)
                     ^ ((int32_t) travelMin << 3)
                     ^ ((int32_t) travelMax << 4)
                     ^ ((int32_t) usDelay << 5)
                     ^ ((int32_t) latchBackoff << 6)
                     ;

    return result;
}

char * Axis::saveConfig(char *out, size_t maxLen) {
    *out++ = '{';
    if (enabled) {
        out = saveConfigValue("dh", dirHIGH, out);
        out = saveConfigValue("en", enabled, out);
        out = saveConfigValue("ho", home, out);
        out = saveConfigValue("is", idleSnooze, out);
        out = saveConfigValue("lb", latchBackoff, out);
        out = saveConfigValue("mi", microsteps, out);
        out = saveConfigValue("mp", mstepPulses, out);
        out = saveConfigValue("sa", stepAngle, out, 1);
        out = saveConfigValue("tm", travelMax, out);
        out = saveConfigValue("tn", travelMin, out);
        out = saveConfigValue("ud", usDelay, out);
    } else {
        out = saveConfigValue("en", enabled, out);
    }
    out--;
    *out++ = '}';
    *out = 0;
    return out;
}

////////////////////// ZPlane /////////////////////////

bool ZPlane::initialize(XYZ3D p1, XYZ3D p2, XYZ3D p3) {
    //z1 = a*x1 + b*y1 + c
    //z2 = a*x2 + b*y2 + c
    //z3 = a*x3 + b*y3 + c

    //z1-z2 = a*(x1-x2) + b*(y1-y2);
    //z1-z3 = a*(x1-x3) + b*(y1-y3);

    //[(z1-z2) - b*(y1-y2)] / (x1-x2) = a
    //[(z1-z3) - b*(y1-y3)] / (x1-x3) = a

    //(x1-x3)*[(z1-z2) - b*(y1-y2)]  = (x1-x2)*[(z1-z3) - b*(y1-y3)]

    //(x1-x3)(z1-z2) - b*(y1-y2)*(x1-x3)  = (x1-x2)*(z1-z3) - b*(y1-y3)*(x1-x2)

    //(x1-x3)(z1-z2) - b*(y1-y2)*(x1-x3)  = (x1-x2)*(z1-z3) - b*(y1-y3)*(x1-x2)

    PH5TYPE x12 = p1.x-p2.x;
    PH5TYPE x13 = p1.x-p3.x;
    PH5TYPE y12 = p1.y-p2.y;
    PH5TYPE y13 = p1.y-p3.y;
    PH5TYPE z12 = p1.z-p2.z;
    PH5TYPE z13 = p1.z-p3.z;

    PH5TYPE det23 = y13*x12-y12*x13;
    if (det23 == 0) {
        TESTCOUT4("det23 y13:", y13, " x12:", x12, " y12:", y12, " x13:", x13);
        return false;
    }
    //x13*z12 - b*y12*x13  = x12*z13 - b*y13*x12
    //b*y13*x12 - b*y12*x13  = x12*z13 - x13*z12
    b  = (x12*z13 - x13*z12) / det23;

    if (x12) {
        a = (z12-b*y12)/x12;
        c = p1.z - a*p1.x - b*p1.y;
    } else if (x13) {
        a = (z13-b*y13)/x13;
        c = p1.z - a*p1.x - b*p1.y;
    } else {
        TESTCOUT3("p1.x:", p1.x, " p2.x:", p2.x, " p3.x:", p3.x);
        return false;
    }

    return true;
}

////////////////////// Machine /////////////////////////

Machine::Machine() :
	autoHome(false),
	invertLim(false),
	jsonPrettyPrint(false),
	autoSync(false),
	pullups(0),
	debounce(0),
	vMax(12800),
	tvMax(0.7),
	fastSearchPulses(FPD_FAST_SEARCH_PULSES),
	searchDelay(FPD_SEARCH_DELAY),
	pinStatus(NOPIN),
	topology(MTO_RAW),
	outputMode(OUTPUT_ARRAY1),
	homeZ(0),
	syncHash(0),
	pDisplay(&nullDisplay)
{
    pinEnableHigh = false;
    for (QuadIndex i = 0; i < QUAD_ELEMENTS; i++) {
        setAxisIndex((MotorIndex)i, (AxisIndex)i);
    }

    for (MotorIndex i = 0; i < MOTOR_COUNT; i++) {
        motor[i] = i;
    }

    for (int16_t i=0; i<PROBE_DATA; i++) {
        op.probe.probeData[i] = 0;
    }

    for (int16_t i=0; i<MARK_COUNT; i++) {
        marks[i] = 0;
    }

    axis[0].id = 'x';
    axis[1].id = 'y';
    axis[2].id = 'z';
    axis[3].id = 'a';
    axis[4].id = 'b';
    axis[5].id = 'c';
    homeAngle = delta.getHomeAngle();
    setPinConfig(PC0_NOPIN);
}

/**
 * Set the home angle and pulses
 */
void Machine::setHomeAngle(PH5TYPE degrees) {
    if (topology == MTO_FPD) {
        delta.setHomeAngle(degrees);
        StepCoord newHomePulses = delta.getHomePulses();
        homeAngle = degrees;
		TESTCOUT4("newHomePulses:", newHomePulses, 
			" axis[0]:", axis[0].home,
			" axis[1]:", axis[1].home,
			" axis[2]:", axis[2].home);
		axis[0].position += newHomePulses - axis[0].home;
		axis[1].position += newHomePulses - axis[1].home;
		axis[2].position += newHomePulses - axis[2].home;
        axis[0].home = newHomePulses;
        axis[1].home = newHomePulses;
        axis[2].home = newHomePulses;
        TESTCOUT3("setHomeAngle degrees:", degrees, " newHomePulses:", newHomePulses, " dHome:", newHomePulses - delta.getHomePulses());
    }
}

/**
 * Set the home pulses and angle.
 */
void Machine::setHomeAngleFromPulses(StepCoord pulseCount) {
    delta.setHomePulses(pulseCount);
    setHomeAngle(delta.getHomeAngle());
}

void Machine::setup(PinConfig cfg) {
    setPinConfig(cfg);
    for (AxisIndex i=0; i<AXIS_COUNT; i++) {
        axis[i].enable(false); // toggle
        axis[i].enable(true);
    }
}

int32_t hashOf(PH5TYPE &value) {
    const uint8_t *pRawBytes = (const uint8_t *)(void *) &value;

    int32_t result = 0;
    for (size_t i=sizeof(PH5TYPE); i-- > 0; ) {
        result = result << 8;
        result |= pRawBytes[i];
    }
    return result;
}

int32_t Machine::hash() {
    PH5TYPE gr1 = delta.getGearRatio(DELTA_AXIS_1);
    PH5TYPE gr2 = delta.getGearRatio(DELTA_AXIS_2);
    PH5TYPE gr3 = delta.getGearRatio(DELTA_AXIS_3);
    PH5TYPE spa = delta.getSPEAngle();
    PH5TYPE sps = delta.getSPERatio();
    int32_t result = 0;
    result = result ^ ((int32_t) outputMode << 8);
    result = result ^ ((int32_t) topology << 9);
    result = result ^ ((int32_t) pinConfig << 10);
    result = result ^ ((int32_t) op.probe.pinProbe << 11);
    result = result ^ (invertLim ? (BIT_HASH << 16) : 0);
    result = result ^ (pinEnableHigh ? (BIT_HASH << 17) : 0);
    // result = result  ^ (autoSync ? (BIT_HASH << 18) : 0);
    result = result ^ (jsonPrettyPrint ? (BIT_HASH << 19) : 0);
    result = result ^ (autoHome ? (BIT_HASH << 20) : 0);
    result = result ^ delta.hash();
	result = result ^ (pullups);
    result = result ^ (vMax);
    result = result ^ hashOf(tvMax);
    result = result ^ (debounce);
    result = result ^ (fastSearchPulses);
    result = result ^ (searchDelay);
    result = result ^ (pinStatus);
    result = result ^ (delta.getSteps360());
    result = result ^ hashOf(homeZ);
    result = result ^ hashOf(bed.a);
    result = result ^ hashOf(bed.b);
    result = result ^ hashOf(bed.c);
    result = result ^ hashOf(gr1);
    result = result ^ hashOf(gr2);
    result = result ^ hashOf(gr3);
    result = result ^ hashOf(spa);
    result = result ^ hashOf(sps);
    //^ (eeUser);
    for (AxisIndex i=0; i<AXIS_COUNT; i++) {
        result ^= axis[i].hash() << i;
    }

    return result;
}

bool Machine::isCorePin(int16_t pin) {
    for (AxisIndex i=0; i<AXIS_COUNT; i++) {
        if (axis[i].pinStep == pin ||
                axis[i].pinDir == pin ||
                axis[i].pinMin == pin ||
                axis[i].pinMax == pin ||
                axis[i].pinEnable == pin) {
            return true;
        }
    }
    return false;
}

Status Machine::setPinConfig_EMC02() {
    Status status = STATUS_OK;

    pinStatus = PC1_STATUS_PIN;
    op.probe.pinProbe = PC1_PROBE_PIN;
    setPin(axis[0].pinStep, PC1_X_STEP_PIN, OUTPUT);
    setPin(axis[0].pinDir, PC1_X_DIR_PIN, OUTPUT);
    setPin(axis[0].pinMin, PC1_X_MIN_PIN, pullupMode(PULLUP_LIMIT_MIN));
    setPin(axis[0].pinEnable, PC1_X_ENABLE_PIN, OUTPUT, HIGH);
    setPin(axis[1].pinStep, PC1_Y_STEP_PIN, OUTPUT);
    setPin(axis[1].pinDir, PC1_Y_DIR_PIN, OUTPUT);
    setPin(axis[1].pinMin, PC1_Y_MIN_PIN, pullupMode(PULLUP_LIMIT_MIN));
    setPin(axis[1].pinEnable, PC1_Y_ENABLE_PIN, OUTPUT, HIGH);
    setPin(axis[2].pinStep, PC1_Z_STEP_PIN, OUTPUT);
    setPin(axis[2].pinDir, PC1_Z_DIR_PIN, OUTPUT);
    setPin(axis[2].pinMin, PC1_Z_MIN_PIN, pullupMode(PULLUP_LIMIT_MIN));
    setPin(axis[2].pinEnable, PC1_Z_ENABLE_PIN, OUTPUT, HIGH);
    setPin(axis[3].pinStep, PC1_TOOL_STEP_PIN, OUTPUT);
    setPin(axis[3].pinDir, PC1_TOOL_DIR_PIN, OUTPUT);
    setPin(axis[3].pinMin, NOPIN, pullupMode(PULLUP_LIMIT_MIN));
    setPin(axis[3].pinEnable, PC1_TOOL1_ENABLE_PIN, OUTPUT, HIGH);
    setPin(axis[4].pinStep, PC1_TOOL_STEP_PIN, OUTPUT);
    setPin(axis[4].pinDir, PC1_TOOL_DIR_PIN, OUTPUT);
    setPin(axis[4].pinMin, NOPIN, pullupMode(PULLUP_LIMIT_MIN));
    setPin(axis[4].pinEnable, PC1_TOOL2_ENABLE_PIN, OUTPUT, HIGH);
    setPin(axis[5].pinStep, PC1_TOOL_STEP_PIN, OUTPUT);
    setPin(axis[5].pinDir, PC1_TOOL_DIR_PIN, OUTPUT);
    setPin(axis[5].pinMin, NOPIN, pullupMode(PULLUP_LIMIT_MIN));
    setPin(axis[5].pinEnable, PC1_TOOL3_ENABLE_PIN, OUTPUT, HIGH);

    //FirePick Delta specific stuff
    fireduino::pinMode(PC1_TOOL1_ENABLE_PIN,OUTPUT);
    fireduino::pinMode(PC1_TOOL2_ENABLE_PIN,OUTPUT);
    fireduino::pinMode(PC1_TOOL3_ENABLE_PIN,OUTPUT);
    fireduino::pinMode(PC1_TOOL4_ENABLE_PIN,OUTPUT);
    fireduino::pinMode(PC1_PWR_SUPPLY_PIN,OUTPUT);
    fireduino::pinMode(PC1_TOOL1_DOUT,OUTPUT);
    fireduino::pinMode(PC1_TOOL2_DOUT,OUTPUT);
    fireduino::pinMode(PC1_TOOL3_DOUT,OUTPUT);
    fireduino::pinMode(PC1_TOOL4_DOUT,OUTPUT);
    fireduino::pinMode(PC1_SERVO1,OUTPUT);
    fireduino::pinMode(PC1_SERVO2,OUTPUT);
    fireduino::pinMode(PC1_SERVO3,OUTPUT);
    fireduino::pinMode(PC1_SERVO4,OUTPUT);
    fireduino::digitalWrite(PC1_TOOL1_ENABLE_PIN,HIGH);
    fireduino::digitalWrite(PC1_TOOL2_ENABLE_PIN,HIGH);
    fireduino::digitalWrite(PC1_TOOL3_ENABLE_PIN,HIGH);
    fireduino::digitalWrite(PC1_TOOL4_ENABLE_PIN,HIGH);
    fireduino::digitalWrite(PC1_PWR_SUPPLY_PIN,LOW);
    fireduino::digitalWrite(PC1_TOOL1_DOUT,LOW);
    fireduino::digitalWrite(PC1_TOOL2_DOUT,LOW);
    fireduino::digitalWrite(PC1_TOOL3_DOUT,LOW);
    fireduino::digitalWrite(PC1_TOOL4_DOUT,LOW);
    fireduino::digitalWrite(PC1_SERVO1,LOW);
    fireduino::digitalWrite(PC1_SERVO2,LOW);
    fireduino::digitalWrite(PC1_SERVO3,LOW);
    fireduino::digitalWrite(PC1_SERVO4,LOW);

    return status;
}

Status Machine::setPinConfig_RAMPS1_4() {
    Status status = STATUS_OK;
    pinStatus = PC2_STATUS_PIN;
    op.probe.pinProbe = PC2_PROBE_PIN;
    setPin(axis[0].pinStep, PC2_X_STEP_PIN, OUTPUT);
    setPin(axis[0].pinDir, PC2_X_DIR_PIN, OUTPUT);
    setPin(axis[0].pinMin, PC2_X_MIN_PIN, pullupMode(PULLUP_LIMIT_MIN));
    setPin(axis[0].pinEnable, PC2_X_ENABLE_PIN, OUTPUT, HIGH);
    setPin(axis[1].pinStep, PC2_Y_STEP_PIN, OUTPUT);
    setPin(axis[1].pinDir, PC2_Y_DIR_PIN, OUTPUT);
    setPin(axis[1].pinMin, PC2_Y_MIN_PIN, pullupMode(PULLUP_LIMIT_MIN));
    setPin(axis[1].pinEnable, PC2_Y_ENABLE_PIN, OUTPUT, HIGH);
    setPin(axis[2].pinStep, PC2_Z_STEP_PIN, OUTPUT);
    setPin(axis[2].pinDir, PC2_Z_DIR_PIN, OUTPUT);
    setPin(axis[2].pinMin, PC2_Z_MIN_PIN, pullupMode(PULLUP_LIMIT_MIN));
    setPin(axis[2].pinEnable, PC2_Z_ENABLE_PIN, OUTPUT, HIGH);
    setPin(axis[3].pinStep, PC2_E0_STEP_PIN, OUTPUT);
    setPin(axis[3].pinDir, PC2_E0_DIR_PIN, OUTPUT);
    setPin(axis[3].pinEnable, PC2_E0_ENABLE_PIN, OUTPUT, HIGH);
    setPin(axis[4].pinStep, PC2_E1_STEP_PIN, OUTPUT);
    setPin(axis[4].pinDir, PC2_E1_DIR_PIN, OUTPUT);
    setPin(axis[4].pinEnable, PC2_E1_ENABLE_PIN, OUTPUT, HIGH);
    return status;
}

Status Machine::setPinConfig_FireMC() {
    Status status = STATUS_OK;
    pinStatus = PC3_STATUS_PIN;
    op.probe.pinProbe = PC3_PROBE_PIN;
    setPin(axis[0].pinStep, PC3_X_STEP_PIN, OUTPUT);
    setPin(axis[0].pinDir, PC3_X_DIR_PIN, OUTPUT);
    setPin(axis[0].pinMin, PC3_X_MIN_PIN, pullupMode(PULLUP_LIMIT_MIN));
    setPin(axis[0].pinEnable, PC3_X_ENABLE_PIN, OUTPUT, HIGH);
    setPin(axis[1].pinStep, PC3_Y_STEP_PIN, OUTPUT);
    setPin(axis[1].pinDir, PC3_Y_DIR_PIN, OUTPUT);
    setPin(axis[1].pinMin, PC3_Y_MIN_PIN, pullupMode(PULLUP_LIMIT_MIN));
    setPin(axis[1].pinEnable, PC3_Y_ENABLE_PIN, OUTPUT, HIGH);
    setPin(axis[2].pinStep, PC3_Z_STEP_PIN, OUTPUT);
    setPin(axis[2].pinDir, PC3_Z_DIR_PIN, OUTPUT);
    setPin(axis[2].pinMin, PC3_Z_MIN_PIN, pullupMode(PULLUP_LIMIT_MIN));
    setPin(axis[2].pinEnable, PC3_Z_ENABLE_PIN, OUTPUT, HIGH);
    setPin(axis[3].pinStep, PC3_E0_STEP_PIN, OUTPUT);
    setPin(axis[3].pinDir, PC3_E0_DIR_PIN, OUTPUT);
    setPin(axis[3].pinEnable, PC3_E0_ENABLE_PIN, OUTPUT, HIGH);
    setPin(axis[4].pinStep, PC3_E1_STEP_PIN, OUTPUT);
    setPin(axis[4].pinDir, PC3_E1_DIR_PIN, OUTPUT);
    setPin(axis[4].pinEnable, PC3_E1_ENABLE_PIN, OUTPUT, HIGH);
    setPin(axis[4].pinStep, PC3_E2_STEP_PIN, OUTPUT);
    setPin(axis[4].pinDir, PC3_E2_DIR_PIN, OUTPUT);
    setPin(axis[4].pinEnable, PC3_E2_ENABLE_PIN, OUTPUT, HIGH);
    return status;
}

Status Machine::setPinConfig(PinConfig pc) {
    Status status = STATUS_OK;
    bool enabled[AXIS_COUNT];
    for (AxisIndex i=0; i<AXIS_COUNT; i++) {
        enabled[i] = axis[i].isEnabled();
        axis[i].enable(false);
    }
    switch (pc) {
    default:
        return STATUS_PIN_CONFIG;
    case PC0_NOPIN:
        pinStatus = NOPIN;
        for (AxisIndex iAxis=0; iAxis<AXIS_COUNT; iAxis++) {
            axis[iAxis].pinStep = NOPIN;
            axis[iAxis].pinDir = NOPIN;
            axis[iAxis].pinMin = NOPIN;
            axis[iAxis].pinEnable = NOPIN;
        }
        break;
    case PC1_EMC02:
        status = setPinConfig_EMC02();
        break;
    case PC2_RAMPS_1_4:
        status = setPinConfig_RAMPS1_4();
        break;
    case PC3_FIREMC:
        status = setPinConfig_FireMC();
        break;
    }

    pinConfig = pc;
    for (AxisIndex i=0; i<AXIS_COUNT; i++) {
        axis[i].enable(enabled[i]);
    }
    pDisplay->setup(pinStatus);

    return status;
}

Status Machine::setAxisIndex(MotorIndex iMotor, AxisIndex iAxis) {
    if (iMotor < 0 || MOTOR_COUNT <= iMotor) {
        return STATUS_MOTOR_ERROR;
    }
    if (iAxis < 0 || AXIS_COUNT <= iAxis) {
        return STATUS_AXIS_ERROR;
    }
    motor[iMotor] = iAxis;
    motorAxis[iMotor] = &axis[iAxis];
    return STATUS_OK;
}

MotorIndex Machine::motorOfName(const char *name) {
    // Motor reference
    if (strcmp_PS(OP_1, name) == 0) {
        return 0;
    } else if (strcmp_PS(OP_2, name) == 0) {
        return 1;
    } else if (strcmp_PS(OP_3, name) == 0) {
        return 2;
    } else if (strcmp_PS(OP_4, name) == 0) {
        return 3;
    }

    // Axis reference
    AxisIndex iAxis = axisOfName(name);
    if (iAxis != INDEX_NONE) {
        for (MotorIndex iMotor = 0; iMotor < MOTOR_COUNT; iMotor++) {
            if (motor[iMotor] == iAxis) {
                return iMotor;
            }
        }
    }
    return INDEX_NONE;
}

AxisIndex Machine::axisOfName(const char *name) {
    if (strcmp_PS(OP_x, name) == 0) {
        return X_AXIS;
    } else if (strcmp_PS(OP_y, name) == 0) {
        return Y_AXIS;
    } else if (strcmp_PS(OP_z, name) == 0) {
        return Z_AXIS;
    } else if (strcmp_PS(OP_a, name) == 0) {
        return A_AXIS;
    } else if (strcmp_PS(OP_b, name) == 0) {
        return B_AXIS;
    } else if (strcmp_PS(OP_c, name) == 0) {
        return C_AXIS;
    }

    // Motor reference
    if (strcmp_PS(OP_1, name) == 0) {
        return motor[0];
    } else if (strcmp_PS(OP_2, name) == 0) {
        return motor[1];
    } else if (strcmp_PS(OP_3, name) == 0) {
        return motor[2];
    } else if (strcmp_PS(OP_4, name) == 0) {
        return motor[3];
    }

    return INDEX_NONE;
}

void Machine::setPin(PinType &pinDst, PinType pinSrc, int16_t mode, int16_t value) {
    pinDst = pinSrc;
    if (pinDst != NOPIN) {
        fireduino::pinMode(pinDst, mode);
        if (mode == OUTPUT) {
            fireduino::digitalWrite(pinDst, value);
        }
    }
}

/**
 * The step() method is the "stepper inner loop" that creates the
 * stepper pulse train to QUAD_ELEMENTS steppers. The pulse parameter
 * must have a value of -1, 0, or 1 for each stepper.
 *
 * Steppers can't be driven too quickly--they stall. Each
 * axis has a usDelay field that specifies a minimum delay
 * between pulses.
 */
Status Machine::step(const Quad<StepDV> &pulse) {
    int16_t usDelay = 0;
    for (uint8_t i = 0; i < QUAD_ELEMENTS; i++) { // Pulse leading edges
        Axis &a(*motorAxis[i]);
        switch (pulse.value[i]) {
        case 1:
            if (!a.enabled) {
                TESTCOUT1("step(1): STATUS_AXIS_DISABLED:", (int) i);
                return STATUS_AXIS_DISABLED;
            }
            if (a.position >= a.travelMax) {
                return STATUS_TRAVEL_MAX;
            }
            a.setAdvancing(true);
            fireduino::digitalWrite(a.pinStep, HIGH);
            break;
        case 0:
            break;
        case -1:
            if (!a.enabled) {
                TESTCOUT1("step(-1): STATUS_AXIS_DISABLED:", (int) i);
                return STATUS_AXIS_DISABLED;
            }
            a.readAtMin(invertLim);
            if (a.atMin) {
                return STATUS_LIMIT_MIN;
            }
            if (a.position <= a.travelMin) {
                return STATUS_TRAVEL_MIN;
            }
            a.setAdvancing(false);
            fireduino::digitalWrite(a.pinStep, HIGH);
            break;
        default:
            return STATUS_STEP_RANGE_ERROR;
        }
    }
    fireduino::delay_stepper_pulse();

    for (uint8_t i = 0; i < QUAD_ELEMENTS; i++) { // Pulse trailing edges
        if (pulse.value[i]) {
            Axis &a(*motorAxis[i]);
            fireduino::digitalWrite(a.pinStep, LOW);
            a.position += pulse.value[i];
            usDelay = maxval(usDelay, a.usDelay);
        }
    }

    fireduino::delayMicroseconds(usDelay); // maximum pulse rate throttle

    return STATUS_OK;
}


/**
 * Set direction based on the sign of each pulse value
 * and check bounds
 */
Status Machine::stepDirection(const Quad<StepDV> &pulse) {
    for (uint8_t i = 0; i < QUAD_ELEMENTS; i++) { // Pulse leading edges
        Axis &a(*motorAxis[i]);
        if (pulse.value[i] > 0) {
            if (!a.enabled) {
                TESTCOUT1("step(1): STATUS_AXIS_DISABLED:", (int) i);
                return STATUS_AXIS_DISABLED;
            }
            if (a.position + pulse.value[i] > a.travelMax) {
                return STATUS_TRAVEL_MAX;
            }
            a.setAdvancing(true);
            a.position += pulse.value[i];
        } else if (pulse.value[i] < 0) {
            if (!a.enabled) {
                TESTCOUT1("step(-1): STATUS_AXIS_DISABLED:", (int) i);
                return STATUS_AXIS_DISABLED;
            }
            a.readAtMin(invertLim);
            if (a.atMin) {
                return STATUS_LIMIT_MIN;
            }
            if (a.position + pulse.value[i] < a.travelMin) {
                return STATUS_TRAVEL_MIN;
            }
            a.setAdvancing(false);
            a.position += pulse.value[i];
        }
    }

    return STATUS_OK;
}


/**
 * Send stepper pulses without updating position.
 * This is important for homing, test and calibration
 * Return STATUS_OK on success
 */
Status Machine::pulse(Quad<StepCoord> &pulses) {
    while (!pulses.isZero()) {
        Quad<StepCoord> motorPos = getMotorPosition();
        Quad<StepCoord> pulse(pulses.sgn());
        motorPos -= pulse;
        setMotorPosition(motorPos); // permit infinite travel

        Quad<StepDV> steps(
            (StepDV) pulse.value[0],
            (StepDV) pulse.value[1],
            (StepDV) pulse.value[2],
            (StepDV) pulse.value[3]);
        Status status = step(steps);
        if (status != STATUS_OK) {
            return status;
        }
        pulses -= pulse;
    }

    return STATUS_OK;
}

Status Machine::home(Status status) {
    for (MotorIndex i = 0; i < QUAD_ELEMENTS; i++) {
        Axis &a(*motorAxis[i]);
        if (a.homing) {
            if (!a.enabled || a.pinMin == NOPIN) {
                return STATUS_AXIS_DISABLED;
            }
        }
    }

    switch (status) {
    default:
        if (stepHome(fastSearchPulses, searchDelay/5) > 0) {
            //TESTCOUT1("home.A fastSearchPulses:", fastSearchPulses);
            status = STATUS_BUSY_MOVING;
        } else {
            //TESTCOUT1("home.B fastSearchPulses:", fastSearchPulses);
            backoffHome(searchDelay);
            status = STATUS_BUSY_CALIBRATING;
        }
        break;
    case STATUS_BUSY_CALIBRATING:
        fireduino::delay(100); // wait 0.1s for machine to settle
        while (stepHome(1, searchDelay) > 0) { }
        backoffHome(searchDelay);
        for (MotorIndex i = 0; i < QUAD_ELEMENTS; i++) {
            Axis &a(*motorAxis[i]);
            if (a.homing) {
                a.position = a.home;
                a.homing = false;
            }
        }
        status = STATUS_OK;
        break;
    }

    return status;
}

Status Machine::probe(Status status, DelayMics delay) {
    if (op.probe.pinProbe == NOPIN) {
        return STATUS_PROBE_PIN;
    }
	fireduino::pinMode(op.probe.pinProbe, pullupMode(PULLUP_PROBE));
    for (MotorIndex i = 0; i < QUAD_ELEMENTS; i++) {
        Axis &a(*motorAxis[i]);
        if (op.probe.start.value[i]!=op.probe.end.value[i] && !a.enabled) {
            return STATUS_AXIS_DISABLED;
        }
    }

    op.probe.probing = !isAtLimit(op.probe.pinProbe);
    if (op.probe.invertProbe) {
        op.probe.probing = !op.probe.probing;
    }
    if (op.probe.probing) {
        status = stepProbe(delay < 0 ? searchDelay : delay);
    } else {
        status = STATUS_OK;
    }

    return status;
}

Status Machine::stepProbe(int16_t delay) {
    Status status = STATUS_BUSY_CALIBRATING;
    //StepCoord dMax = 0;

    if (op.probe.curDelta >= op.probe.maxDelta) {
        return STATUS_PROBE_FAILED; // done
    }
    op.probe.curDelta++;
    Quad<StepDV> pulse;
    for (QuadIndex i = 0; i < QUAD_ELEMENTS; i++) {
        StepCoord dp = op.probe.interpolate(i) - getMotorAxis(i).position;
        if (dp < -1) {
            pulse.value[i] = -1;
        } else if (dp > 1) {
            pulse.value[i] = 1;
        } else if (dp) {
            pulse.value[i] = dp;
        } else {
            pulse.value[i] = 0;
        }
    }
    if (0 > (status = stepDirection(pulse))) {
        return status;
    }
    if (0 > (status = stepFast(pulse))) {
        return status;
    }
    fireduino::delayMicroseconds(delay);
    return STATUS_BUSY_CALIBRATING;
}

void Machine::backoffHome(int16_t delay) {
    bool backingOff = true;
    for (StepCoord pulses=0; backingOff==true; pulses++) {
        backingOff = false;
        for (uint8_t i = 0; i < QUAD_ELEMENTS; i++) {
            Axis &a(*motorAxis[i]);
            if (a.homing && pulses < a.latchBackoff) {
                a.pulse(true);
                backingOff = true;
            }
        }
        fireduino::delayMicroseconds(delay);
    }
}

StepCoord Machine::stepHome(StepCoord pulsesPerAxis, int16_t delay) {
    StepCoord pulses = 0;

    for (int8_t iPulse = 0; iPulse < pulsesPerAxis; iPulse++) {
        for (uint8_t i = 0; i < QUAD_ELEMENTS; i++) {
            Axis &a(*motorAxis[i]);
            if (a.homing) {
                a.readAtMin(invertLim);
                //TESTCOUT2("stepHome a:", (int) i, " atMin:", (int) a.atMin );
                a.setAdvancing(false);
            }
        }
        fireduino::delayMicroseconds(delay); // maximum pulse rate throttle

        // Move pulses as synchronously as possible for smoothness
        for (uint8_t i = 0; i < QUAD_ELEMENTS; i++) {
            Axis &a(*motorAxis[i]);
            if (a.homing && !a.atMin) {
                fireduino::pulseFast(a.pinStep);
                pulses++;
            }
        }
    }

    return pulses;
}

/**
 * Return position of currently driven axes bound to motors
 */
Quad<StepCoord> Machine::getMotorPosition() {
    return Quad<StepCoord>(
               motorAxis[0]->position,
               motorAxis[1]->position,
               motorAxis[2]->position,
               motorAxis[3]->position
           );
}

Status Machine::idle(Status status) {
    for (MotorIndex i = 0; i < MOTOR_COUNT; i++) {
        if (motorAxis[i]->enabled && motorAxis[i]->idleSnooze) {
            motorAxis[i]->enable(false);
            fireduino::delayMicroseconds(motorAxis[i]->idleSnooze);
            motorAxis[i]->enable(true);
        }
    }
    return status;
}

/**
 * Set position of currently driven axes bound to motors
 */
void Machine::setMotorPosition(const Quad<StepCoord> &position) {
    for (MotorIndex i = 0; i < QUAD_ELEMENTS; i++) {
        motorAxis[i]->position = position.value[i];
    }
}

char * Machine::saveSysConfig(char *out, size_t maxLen) {
    *out++ = '{';
    // priority 1
    out = saveConfigValue("ch", hash(), out);
    out = saveConfigValue("pc", (uint8_t) pinConfig, out);
    out = saveConfigValue("to", (uint8_t) topology, out);

    // priority 2
    out = saveConfigValue("ah", autoHome, out);
    //out = saveConfigValue("as", autoSync, out); // too dangerous
    out = saveConfigValue("db", debounce, out);
    //out = saveConfigValue("eu", eeUser, out); // saved separately
    out = saveConfigValue("hp", fastSearchPulses, out);
    out = saveConfigValue("jp", jsonPrettyPrint, out);
    out = saveConfigValue("lh", invertLim, out);
    out = saveConfigValue("mv", vMax, out);
    out = saveConfigValue("om", (uint8_t) outputMode, out);
    out = saveConfigValue("pb", op.probe.pinProbe, out);
    out = saveConfigValue("pi", pinStatus, out);
    out = saveConfigValue("pu", pullups, out);
    out = saveConfigValue("tv", tvMax, out);

    out--;
    *out++ = '}';
    *out = 0;
    return out;
}

char * Machine::saveDimConfig(char *out, size_t maxLen) {
    *out++ = '{';
    // save these so that they will load first before angles
    out = saveConfigValue("bx", bed.getXScale(), out, 4);
    out = saveConfigValue("by", bed.getYScale(), out, 4);
    out = saveConfigValue("bz", bed.getZOffset(), out);
    out = saveConfigValue("e", delta.getEffectorTriangleSide(), out);
    out = saveConfigValue("f", delta.getBaseTriangleSide(), out);
    out = saveConfigValue("gr1", delta.getGearRatio(DELTA_AXIS_1), out, 5);
    out = saveConfigValue("gr2", delta.getGearRatio(DELTA_AXIS_2), out, 5);
    out = saveConfigValue("gr3", delta.getGearRatio(DELTA_AXIS_3), out, 5);
    out = saveConfigValue("ha", homeAngle, out);
    out = saveConfigValue("hz", homeZ, out);
    out = saveConfigValue("mi", delta.getMicrosteps(), out);
    out = saveConfigValue("re", delta.getEffectorLength(), out);
    out = saveConfigValue("rf", delta.getBaseArmLength(), out);
    out = saveConfigValue("spa", delta.getSPEAngle(), out, 3);
    out = saveConfigValue("spr", delta.getSPERatio(), out, 5);
    out = saveConfigValue("st", delta.getSteps360(), out);
    out--;
    *out++ = '}';
    *out = 0;
    return out;
}

void Machine::enableEEUser(bool enable) {
    if (isEEUserEnabled() != enable) {
        fireduino::eeprom_write_byte((uint8_t *)EEUSER_ENABLED, (uint8_t)(enable ? 'y' : 'n'));
    }
}

bool Machine::isEEUserEnabled() {
    return 'y' == fireduino::eeprom_read_byte((uint8_t *)EEUSER_ENABLED);
}

void Machine::loadDeltaCalculator() {
    delta.setHomeAngle(homeAngle);
}

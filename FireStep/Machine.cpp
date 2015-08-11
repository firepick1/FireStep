#ifdef CMAKE
#include <cstring>
#include <cmath>
#endif
#include "Arduino.h"
#include "FireUtils.h"
#include "Machine.h"
#include "AnalogRead.h"
#include "version.h"
#include "ProgMem.h"

using namespace firestep;

template class Quad<int16_t>;
template class Quad<int32_t>;

TESTDECL(int32_t, firestep::delayMicsTotal = 0);

/////////// Silly things done without snprintf (ARDUINO!!!!) /////////////
char * firestep::saveConfigValue(const char *key, const char *value, char *out) {
    sprintf(out, "\"%s\":%s,", key, value);
    return out+strlen(out);
}

char * firestep::saveConfigValue(const char *key, bool value, char *out) {
    return saveConfigValue(key, value ? "1":"0", out);
}

char * firestep::saveConfigValue(const char *key, int16_t value, char *out) {
    sprintf(out, "\"%s\":%d,", key, value);
    return out+strlen(out);
}

char * firestep::saveConfigValue(const char *key, int32_t value, char *out) {
    sprintf(out, "\"%s\":%ld,", key, (long) value);
    return out+strlen(out);
}

char * firestep::saveConfigValue(const char *key, PH5TYPE value, char *out, uint8_t places) {
    bool minus = (value < 0);
    if (minus) {
        value = -value;
    }
    int32_t ivalue = value;
    TESTCOUT3("key:", key, " value:", value, " ivalue:", ivalue);
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
    digitalWrite(pinEnable, active ? PIN_ENABLE : PIN_DISABLE);
    setAdvancing(true);
    enabled = active;
    return STATUS_OK;
}

#define BIT_HASH ((uint32_t)0x3)
int32_t Axis::hash() {
    int32_t result = 0
                     ^ (dirHIGH ? (BIT_HASH << 0) : 0)
                     ^ (enabled ? (BIT_HASH << 1) : 0)
                     ^ ((uint32_t) home << 0)
                     ^ ((uint32_t) idleSnooze << 1)
                     ^ ((uint32_t) microsteps << 2)
                     ^ ((uint32_t) travelMin << 3)
                     ^ ((uint32_t) travelMax << 4)
                     ^ ((uint32_t) usDelay << 5)
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
        out = saveConfigValue("mi", microsteps, out);
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

Machine::Machine()
    : autoHome(false),invertLim(false), pDisplay(&nullDisplay), jsonPrettyPrint(false), vMax(12800),
      tvMax(0.7), fastSearchPulses(3), latchBackoff(LATCH_BACKOFF),
      searchDelay(800), pinStatus(NOPIN), topology(MTO_RAW),
      outputMode(OUTPUT_ARRAY1), debounce(0), autoSync(false), syncHash(0)
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
}

void Machine::setup(PinConfig cfg) {
    setPinConfig(cfg);
    for (AxisIndex i=0; i<AXIS_COUNT; i++) {
        axis[i].enable(false); // toggle
        axis[i].enable(true);
    }
}

int32_t Machine::hash() {
    int32_t result = 0
                     ^ ((uint32_t) outputMode << 8)
                     ^ ((uint32_t) topology << 9)
                     ^ ((uint32_t) pinConfig << 10)
                     ^ (invertLim ? (BIT_HASH << 16) : 0)
                     ^ (pinEnableHigh ? (BIT_HASH << 17) : 0)
                     ^ (autoSync ? (BIT_HASH << 18) : 0)
                     ^ (jsonPrettyPrint ? (BIT_HASH << 19) : 0)
                     ^ (autoHome ? (BIT_HASH << 20) : 0)
                     ^ delta.hash()
                     ^ (vMax)
                     ^ (*(uint32_t *)(void*)&tvMax)
                     ^ (debounce)
                     ^ (fastSearchPulses)
                     ^ (latchBackoff)
                     ^ (searchDelay)
                     ^ (pinStatus)
                     ^ (*(uint32_t *)(void*)&bed.a)
                     ^ (*(uint32_t *)(void*)&bed.b)
                     ^ (*(uint32_t *)(void*)&bed.c)
                     //^ (eeUser)
                     ;
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
    setPin(axis[0].pinMin, PC1_X_MIN_PIN, INPUT);
    setPin(axis[0].pinEnable, PC1_X_ENABLE_PIN, OUTPUT, HIGH);
    setPin(axis[1].pinStep, PC1_Y_STEP_PIN, OUTPUT);
    setPin(axis[1].pinDir, PC1_Y_DIR_PIN, OUTPUT);
    setPin(axis[1].pinMin, PC1_Y_MIN_PIN, INPUT);
    setPin(axis[1].pinEnable, PC1_Y_ENABLE_PIN, OUTPUT, HIGH);
    setPin(axis[2].pinStep, PC1_Z_STEP_PIN, OUTPUT);
    setPin(axis[2].pinDir, PC1_Z_DIR_PIN, OUTPUT);
    setPin(axis[2].pinMin, PC1_Z_MIN_PIN, INPUT);
    setPin(axis[2].pinEnable, PC1_Z_ENABLE_PIN, OUTPUT, HIGH);
    setPin(axis[3].pinStep, PC1_TOOL_STEP_PIN, OUTPUT);
    setPin(axis[3].pinDir, PC1_TOOL_DIR_PIN, OUTPUT);
    setPin(axis[3].pinMin, NOPIN, INPUT);
    setPin(axis[3].pinEnable, PC1_TOOL1_ENABLE_PIN, OUTPUT, HIGH);
    setPin(axis[4].pinStep, PC1_TOOL_STEP_PIN, OUTPUT);
    setPin(axis[4].pinDir, PC1_TOOL_DIR_PIN, OUTPUT);
    setPin(axis[4].pinMin, NOPIN, INPUT);
    setPin(axis[4].pinEnable, PC1_TOOL2_ENABLE_PIN, OUTPUT, HIGH);
    setPin(axis[5].pinStep, PC1_TOOL_STEP_PIN, OUTPUT);
    setPin(axis[5].pinDir, PC1_TOOL_DIR_PIN, OUTPUT);
    setPin(axis[5].pinMin, NOPIN, INPUT);
    setPin(axis[5].pinEnable, PC1_TOOL3_ENABLE_PIN, OUTPUT, HIGH);

    //FirePick Delta specific stuff
    pinMode(PC1_TOOL1_ENABLE_PIN,OUTPUT);
    pinMode(PC1_TOOL2_ENABLE_PIN,OUTPUT);
    pinMode(PC1_TOOL3_ENABLE_PIN,OUTPUT);
    pinMode(PC1_TOOL4_ENABLE_PIN,OUTPUT);
    pinMode(PC1_PWR_SUPPLY_PIN,OUTPUT);
    pinMode(PC1_TOOL1_DOUT,OUTPUT);
    pinMode(PC1_TOOL2_DOUT,OUTPUT);
    pinMode(PC1_TOOL3_DOUT,OUTPUT);
    pinMode(PC1_TOOL4_DOUT,OUTPUT);
    pinMode(PC1_SERVO1,OUTPUT);
    pinMode(PC1_SERVO2,OUTPUT);
    pinMode(PC1_SERVO3,OUTPUT);
    pinMode(PC1_SERVO4,OUTPUT);
    digitalWrite(PC1_TOOL1_ENABLE_PIN,HIGH);
    digitalWrite(PC1_TOOL2_ENABLE_PIN,HIGH);
    digitalWrite(PC1_TOOL3_ENABLE_PIN,HIGH);
    digitalWrite(PC1_TOOL4_ENABLE_PIN,HIGH);
    digitalWrite(PC1_PWR_SUPPLY_PIN,LOW);
    digitalWrite(PC1_TOOL1_DOUT,LOW);
    digitalWrite(PC1_TOOL2_DOUT,LOW);
    digitalWrite(PC1_TOOL3_DOUT,LOW);
    digitalWrite(PC1_TOOL4_DOUT,LOW);
    digitalWrite(PC1_SERVO1,LOW);
    digitalWrite(PC1_SERVO2,LOW);
    digitalWrite(PC1_SERVO3,LOW);
    digitalWrite(PC1_SERVO4,LOW);

    return status;
}

Status Machine::setPinConfig_RAMPS1_4() {
    Status status = STATUS_OK;
    pinStatus = PC2_STATUS_PIN;
    op.probe.pinProbe = PC2_PROBE_PIN;
    setPin(axis[0].pinStep, PC2_X_STEP_PIN, OUTPUT);
    setPin(axis[0].pinDir, PC2_X_DIR_PIN, OUTPUT);
    setPin(axis[0].pinMin, PC2_X_MIN_PIN, INPUT);
    setPin(axis[0].pinEnable, PC2_X_ENABLE_PIN, OUTPUT, HIGH);
    setPin(axis[1].pinStep, PC2_Y_STEP_PIN, OUTPUT);
    setPin(axis[1].pinDir, PC2_Y_DIR_PIN, OUTPUT);
    setPin(axis[1].pinMin, PC2_Y_MIN_PIN, INPUT);
    setPin(axis[1].pinEnable, PC2_Y_ENABLE_PIN, OUTPUT, HIGH);
    setPin(axis[2].pinStep, PC2_Z_STEP_PIN, OUTPUT);
    setPin(axis[2].pinDir, PC2_Z_DIR_PIN, OUTPUT);
    setPin(axis[2].pinMin, PC2_Z_MIN_PIN, INPUT);
    setPin(axis[2].pinEnable, PC2_Z_ENABLE_PIN, OUTPUT, HIGH);
    setPin(axis[3].pinStep, PC2_E0_STEP_PIN, OUTPUT);
    setPin(axis[3].pinDir, PC2_E0_DIR_PIN, OUTPUT);
    setPin(axis[3].pinEnable, PC2_E0_ENABLE_PIN, OUTPUT, HIGH);
    setPin(axis[4].pinStep, PC2_E1_STEP_PIN, OUTPUT);
    setPin(axis[4].pinDir, PC2_E1_DIR_PIN, OUTPUT);
    setPin(axis[4].pinEnable, PC2_E1_ENABLE_PIN, OUTPUT, HIGH);
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
    if (strcmp_P(OP_1, name) == 0) {
        return 0;
    } else if (strcmp_P(OP_2, name) == 0) {
        return 1;
    } else if (strcmp_P(OP_3, name) == 0) {
        return 2;
    } else if (strcmp_P(OP_4, name) == 0) {
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
    // Axis reference
    AxisIndex iAxis;
    if (strcmp_P(OP_x, name) == 0) {
        return X_AXIS;
    } else if (strcmp_P(OP_y, name) == 0) {
        return Y_AXIS;
    } else if (strcmp_P(OP_z, name) == 0) {
        return Z_AXIS;
    } else if (strcmp_P(OP_a, name) == 0) {
        return A_AXIS;
    } else if (strcmp_P(OP_b, name) == 0) {
        return B_AXIS;
    } else if (strcmp_P(OP_c, name) == 0) {
        return C_AXIS;
    }

    // Motor reference
    if (strcmp_P(OP_1, name) == 0) {
        return motor[0];
    } else if (strcmp_P(OP_2, name) == 0) {
        return motor[1];
    } else if (strcmp_P(OP_3, name) == 0) {
        return motor[2];
    } else if (strcmp_P(OP_4, name) == 0) {
        return motor[3];
    }

    return INDEX_NONE;
}

void Machine::setPin(PinType &pinDst, PinType pinSrc, int16_t mode, int16_t value) {
    pinDst = pinSrc;
    if (pinDst != NOPIN) {
        pinMode(pinDst, mode);
        if (mode == OUTPUT) {
            digitalWrite(pinDst, value);
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
            digitalWrite(a.pinStep, HIGH);
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
            digitalWrite(a.pinStep, HIGH);
            break;
        default:
            return STATUS_STEP_RANGE_ERROR;
        }
    }
    PULSE_WIDTH_DELAY();

    for (uint8_t i = 0; i < QUAD_ELEMENTS; i++) { // Pulse trailing edges
        if (pulse.value[i]) {
            Axis &a(*motorAxis[i]);
            digitalWrite(a.pinStep, LOW);
            a.position += pulse.value[i];
            usDelay = max(usDelay, a.usDelay);
        }
    }

    delayMics(usDelay); // maximum pulse rate throttle

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
        delayMics(100000); // wait 0.1s for machine to settle
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
    delayMics(delay);
    return STATUS_BUSY_CALIBRATING;
}

void Machine::backoffHome(int16_t delay) {
    bool backingOff = true;
    for (StepCoord pulses=0; backingOff==true; pulses++) {
        backingOff = false;
        for (uint8_t i = 0; i < QUAD_ELEMENTS; i++) {
            Axis &a(*motorAxis[i]);
            if (a.homing && pulses < latchBackoff) {
                a.pulse(true);
                backingOff = true;
            }
        }
        delayMics(delay);
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
        delayMics(delay); // maximum pulse rate throttle

        // Move pulses as synchronously as possible for smoothness
        for (uint8_t i = 0; i < QUAD_ELEMENTS; i++) {
            Axis &a(*motorAxis[i]);
            if (a.homing && !a.atMin) {
                pulseFast(a.pinStep);
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
            delayMics(motorAxis[i]->idleSnooze);
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
    out = saveConfigValue("pc", pinConfig, out);
    out = saveConfigValue("to", topology, out);

    // priority 2
    out = saveConfigValue("ah", autoHome, out);
    out = saveConfigValue("as", autoSync, out);
    out = saveConfigValue("db", debounce, out);
    //out = saveConfigValue("eu", eeUser, out); // saved separately
    out = saveConfigValue("hp", fastSearchPulses, out);
    out = saveConfigValue("jp", jsonPrettyPrint, out);
    out = saveConfigValue("lb", latchBackoff, out);
    out = saveConfigValue("lh", invertLim, out);
    out = saveConfigValue("mv", vMax, out);
    out = saveConfigValue("om", outputMode, out);
    out = saveConfigValue("pi", pinStatus, out);
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
    out = saveConfigValue("gr", delta.getGearRatio(), out, 3);
    out = saveConfigValue("ha", homeAngle, out);
    out = saveConfigValue("mi", delta.getMicrosteps(), out);
    out = saveConfigValue("re", delta.getEffectorLength(), out);
    out = saveConfigValue("rf", delta.getBaseArmLength(), out);
    out = saveConfigValue("st", delta.getSteps360(), out);
    out--;
    *out++ = '}';
    *out = 0;
    return out;
}

void Machine::enableEEUser(bool enable) {
    if (isEEUserEnabled() != enable) {
        eeprom_write_byte((uint8_t *)EEUSER_ENABLED, (uint8_t)(enable ? 'y' : 'n'));
    }
}

bool Machine::isEEUserEnabled() {
    return 'y' == eeprom_read_byte((uint8_t *)EEUSER_ENABLED);
}

void Machine::loadDeltaCalculator() {
	delta.setHomeAngle(homeAngle);
    delta.setHomePulses((axis[0].home+axis[1].home+axis[2].home)/3);
}

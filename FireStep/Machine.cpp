#ifdef CMAKE
#include <cstring>
#include <cmath>
#endif
#include "Arduino.h"
#include "FireUtils.h"
#include "Machine.h"
#include "AnalogRead.h"
#include "version.h"

using namespace firestep;

template class Quad<int16_t>;
template class Quad<int32_t>;

TESTDECL(int32_t, firestep::delayMicsTotal = 0);

/////////////////////////// Axis ///////////////////////

Status Axis::enable(bool active) {
    if (pinEnable == NOPIN || pinStep == NOPIN || pinDir == NOPIN) {
        return STATUS_NOPIN;
    }
    digitalWrite(pinEnable, active ? PIN_ENABLE : PIN_DISABLE);
    setAdvancing(true);
    enabled = active;
    return STATUS_OK;
}

uint32_t Axis::hash() {
	uint32_t result = 0;
	uint8_t * pStart = (uint8_t *)(void*)&AXIS_CONFIG_START;
	uint8_t * pEnd = (uint8_t *)(void*)&AXIS_CONFIG_END;
	size_t bytes = pEnd - pStart;
	for (size_t i=0; i<bytes; i++) {
		result ^= (*pStart++ << (i%24));
	}
	TESTCOUT2("axis hash bytes:", bytes, " result:", result);

	return result;
}

char * Axis::saveConfig(char *out, size_t maxLen) {
	snprintf(out, maxLen, "{"
		"\"dh\":%s,"
		"\"en\":%s,"
		"\"ho\":%d,"
		"\"is\":%d,"
		"\"mi\":%d,"
		"\"sa\":%.1f,"
		"\"tm\":%d,"
		"\"tn\":%d,"
		"\"ud\":%d}",
		dirHIGH ? "true":"false",
		enabled ? "true":"false",
		home,
		idleSnooze,
		microsteps,
		stepAngle,
		travelMax,
		travelMin,
		usDelay,
		NULL);
	return out + strlen(out);
}

////////////////////// Machine /////////////////////////

Machine::Machine()
    : invertLim(false), pDisplay(&nullDisplay), jsonPrettyPrint(false), vMax(12800),
      tvMax(0.7), homingPulses(3), latchBackoff(LATCH_BACKOFF),
      searchDelay(800), pinStatus(NOPIN), eeUser(2000), topology(MTO_RAW),
	  outputMode(OUTPUT_ARRAY1), debounce(0)
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
}

uint32_t Machine::hash() {
	uint32_t result = 0;
	uint8_t *pStart = (uint8_t*)(void*) &MACHINE_CONFIG_START;
	uint8_t *pEnd = (uint8_t*)(void*) &MACHINE_CONFIG_END;
	size_t bytes = pEnd - pStart;
	for (size_t i=0; i<bytes; i++) {
		result ^= (*pStart++ << (i%24));
	}
	for (AxisIndex i=0; i<AXIS_COUNT; i++) {
		result ^= axis[i].hash();
	}
	TESTCOUT2("machine hash bytes:", bytes, " result:", result);

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
    for (AxisIndex i=0; i<AXIS_COUNT; i++) {
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
        axis[i].enable(true);
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
    if (strcmp("1", name) == 0) {
        return 0;
    } else if (strcmp("2", name) == 0) {
        return 1;
    } else if (strcmp("3", name) == 0) {
        return 2;
    } else if (strcmp("4", name) == 0) {
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
    if (strcmp("x", name) == 0) {
        return X_AXIS;
    } else if (strcmp("y", name) == 0) {
        return Y_AXIS;
    } else if (strcmp("z", name) == 0) {
        return Z_AXIS;
    } else if (strcmp("a", name) == 0) {
        return A_AXIS;
    } else if (strcmp("b", name) == 0) {
        return B_AXIS;
    } else if (strcmp("c", name) == 0) {
        return C_AXIS;
    }

    // Motor reference
    if (strcmp("1", name) == 0) {
        return motor[0];
    } else if (strcmp("2", name) == 0) {
        return motor[1];
    } else if (strcmp("3", name) == 0) {
        return motor[2];
    } else if (strcmp("4", name) == 0) {
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

Status Machine::finalizeHome() {
	Status status = STATUS_OK;
    switch (topology) {
    case MTO_RAW:
    default:
        break;
    case MTO_FPD: {
        Quad<StepCoord> limit = getMotorPosition();
        Step3D pulses = delta.calcPulses(XYZ3D());
        op.probe.setup(limit, Quad<StepCoord>(
			pulses.p1,
			pulses.p2,
			pulses.p3,
			limit.value[3]
		));
		status = STATUS_BUSY_CALIBRATING;
        do {
            // fast probe because we don't expect to hit anything
            status = probe(status, 0);
			//TESTCOUT2("finalizeHome status:", (int) status, " 1:", axis[0].position);
        } while (status == STATUS_BUSY_CALIBRATING);
        if (status == STATUS_PROBE_FAILED) {
			// we didn't hit anything and that is good
            status = STATUS_OK;
        } else if (status == STATUS_OK) {
			// we hit something and that's not good
            status = STATUS_TRAVEL_MAX;
        }
    } // case MTO_FPD
    break;
    }
	return status;
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
        if (stepHome(homingPulses, searchDelay/5) > 0) {
            //TESTCOUT1("home.A homingPulses:", homingPulses);
            status = STATUS_BUSY_MOVING;
        } else {
            //TESTCOUT1("home.B homingPulses:", homingPulses);
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
        status = finalizeHome();
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
		if (topology == MTO_FPD && op.probe.dataSource == PDS_Z) {
			XYZ3D xyz = getXYZ3D();
			op.probe.archiveData(xyz.z);
		}
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

void Machine::idle() {
    for (MotorIndex i = 0; i < MOTOR_COUNT; i++) {
        if (motorAxis[i]->enabled && motorAxis[i]->idleSnooze) {
            motorAxis[i]->enable(false);
            delayMics(motorAxis[i]->idleSnooze);
            motorAxis[i]->enable(true);
        }
    }
}

/**
 * Set position of currently driven axes bound to motors
 */
void Machine::setMotorPosition(const Quad<StepCoord> &position) {
    for (MotorIndex i = 0; i < QUAD_ELEMENTS; i++) {
        motorAxis[i]->position = position.value[i];
    }
}

XYZ3D Machine::getXYZ3D() {
    switch (topology) {
    case MTO_RAW:
    default:
        return XYZ3D(
                   motorAxis[0]->position,
                   motorAxis[1]->position,
                   motorAxis[2]->position
               );
    case MTO_FPD:
        return delta.calcXYZ(Step3D(
                                 motorAxis[0]->position,
                                 motorAxis[1]->position,
                                 motorAxis[2]->position
                             ));
    }
}

char * Machine::saveSysConfig(char *out, size_t maxLen) {
	snprintf(out, maxLen, "{"
		"\"db\":%d,"
		"\"eu\":%d,"
		"\"hp\":%d,"
		"\"jp\":%s,"
		"\"lb\":%d,"
		"\"lh\":%s,"
		"\"mv\":%ld,"
		"\"om\":%d,"
		"\"pc\":%d,"
		"\"pi\":%d,"
		"\"to\":%d,"
		"\"tv\":%.2f}",
		debounce,
		eeUser,
		homingPulses,
		jsonPrettyPrint ? "true":"false",
		latchBackoff,
		invertLim ? "true":"false",
		(long) vMax,
		outputMode,
		pinConfig,
		pinStatus,
		topology,
		tvMax,
		NULL);
	return out + strlen(out);
}

char * Machine::saveDimConfig(char *out, size_t maxLen) {
	Angle3D ha = delta.getHomeAngles();
	snprintf(out, maxLen, "{"
		"\"e\":%.2f,"
		"\"f\":%.2f,"
		"\"gr\":%.4f,"
		"\"ha1\":%.2f,"
		"\"ha2\":%.2f,"
		"\"ha3\":%.2f,"
		"\"mi\":%d,"
		"\"re\":%.2f,"
		"\"rf\":%.2f,"
		"\"st\":%d}",
		delta.getEffectorLength(),
		delta.getBaseArmLength(),
		delta.getGearRatio(),
		ha.theta1,
		ha.theta2,
		ha.theta3,
		delta.getMicrosteps(),
		delta.getEffectorTriangleSide(),
		delta.getBaseTriangleSide(),
		delta.getSteps360(),
		NULL);
	return out + strlen(out);
}


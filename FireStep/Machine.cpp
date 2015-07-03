#ifdef CMAKE
#include <cstring>
#include <cmath>
#endif
#include "Arduino.h"
#include "Machine.h"
#include "AnalogRead.h"
#include "version.h"
#ifdef TEST
#include "../test/FireUtils.hpp"
#endif

using namespace firestep;

template class Quad<int16_t>;
template class Quad<int32_t>;

TESTDECL(int32_t, firestep::delayMicsTotal = 0);

Status Axis::enable(bool active) {
    if (pinEnable == NOPIN || pinStep == NOPIN || pinDir == NOPIN) {
        return STATUS_NOPIN;
    }
    digitalWrite(pinEnable, active ? PIN_ENABLE : PIN_DISABLE);
	setAdvancing(true);
    enabled = active;
    return STATUS_OK;
}

Machine::Machine()
    : invertLim(false), pDisplay(&nullDisplay), jsonPrettyPrint(false), vMax(12800), tvMax(0.7) {
    pinEnableHigh = false;
    for (QuadIndex i = 0; i < QUAD_ELEMENTS; i++) {
        setAxisIndex((MotorIndex)i, (AxisIndex)i);
    }

    for (MotorIndex i = 0; i < MOTOR_COUNT; i++) {
        motor[i] = i;
    }
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

Status Machine::setPinConfig(PinConfig pc) {
	for (AxisIndex i=0; i<AXIS_COUNT; i++) {
		axis[i].enable(false);
	}
    switch (pc) {
    default:
        return STATUS_PIN_CONFIG;
    case PC0_NOPIN:
        setPin(axis[0].pinStep, PC0_X_STEP_PIN, OUTPUT);
        setPin(axis[0].pinDir, PC0_X_DIR_PIN, OUTPUT);
        setPin(axis[0].pinMin, PC0_X_MIN_PIN, INPUT);
        setPin(axis[0].pinEnable, PC0_X_ENABLE_PIN, OUTPUT, HIGH);
        setPin(axis[1].pinStep, PC0_Y_STEP_PIN, OUTPUT);
        setPin(axis[1].pinDir, PC0_Y_DIR_PIN, OUTPUT);
        setPin(axis[1].pinMin, PC0_Y_MIN_PIN, INPUT);
        setPin(axis[1].pinEnable, PC0_Y_ENABLE_PIN, OUTPUT, HIGH);
        setPin(axis[2].pinStep, PC0_Z_STEP_PIN, OUTPUT);
        setPin(axis[2].pinDir, PC0_Z_DIR_PIN, OUTPUT);
        setPin(axis[2].pinMin, PC0_Z_MIN_PIN, INPUT);
        setPin(axis[2].pinEnable, PC0_Z_ENABLE_PIN, OUTPUT, HIGH);
        break;
    case PC1_EMC02:
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
        break;
    case PC2_RAMPS_1_4:
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
        break;
    }

    pinConfig = pc;
	for (AxisIndex i=0; i<AXIS_COUNT; i++) {
		axis[i].enable(true);
	}
    return STATUS_OK;
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

Status Machine::home() {
    int16_t searchDelay = 0;
    for (MotorIndex i = 0; i < QUAD_ELEMENTS; i++) {
        Axis &a(*motorAxis[i]);
        if (a.homing) {
            if (!a.enabled && a.pinMin != NOPIN) {
                return STATUS_AXIS_DISABLED;
            }
            a.position = a.home;
			a.setAdvancing(false);
            searchDelay = max(searchDelay, a.searchDelay);
        }
    }

    if (stepHome(32, searchDelay) > 0) {
        return STATUS_BUSY_MOVING;
    }

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


int8_t Machine::stepHome(int16_t pulsesPerAxis, int16_t searchDelay) {
    int8_t pulses = 0;

    for (int8_t iPulse = 0; iPulse < pulsesPerAxis; iPulse++) {
        for (uint8_t i = 0; i < QUAD_ELEMENTS; i++) {
            Axis &a(*motorAxis[i]);
            if (a.homing && a.enabled) {
                a.readAtMin(invertLim);
                if (a.atMin) {
                    a.homing = false;
                    delayMics(100000); // wait 0.1s for machine to settle
                    for (StepCoord lb = a.latchBackoff; lb > 0; lb--) {
                        a.pulse(true);
                        delayMics(a.searchDelay);
                    }
                } else {
                    a.pulse(false);
                    pulses++;
                }
            }
        }
        delayMics(searchDelay); // maximum pulse rate throttle
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


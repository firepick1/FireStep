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

#define PULSEAXIS_MICS 6 /* microseconds for a single pulseAxis() invocation */

#ifdef TEST
int32_t firestep::delayMicsTotal = 0;
#endif

Status Axis::enable(bool active) {
    if (pinEnable == NOPIN || pinStep == NOPIN || pinDir == NOPIN || pinMin == NOPIN) {
        return STATUS_NOPIN;
    }
    digitalWrite(pinEnable, active ? PIN_ENABLE : PIN_DISABLE);
    digitalWrite(pinDir, (advancing == dirHIGH) ? HIGH : LOW);
    enabled = active;
    return STATUS_OK;
}

Machine::Machine()
    : invertLim(false), pDisplay(&nullDisplay), jsonPrettyPrint(false) {
    pinEnableHigh = false;
    for (int8_t i = 0; i < QUAD_ELEMENTS; i++) {
        setMotorAxis((MotorIndex)i, (AxisIndex)i);
    }
	setPinConfig(PC2_RAMPS_1_4);

    for (int i = 0; i < MOTOR_COUNT; i++) {
        motor[i] = i;
    }
}

Status Machine::setPinConfig(PinConfig pc) {
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
    case PC1_EMC01:
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
	return STATUS_OK;
}

Status Machine::setMotorAxis(MotorIndex iMotor, AxisIndex iAxis) {
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
    for (int i = 0; i < QUAD_ELEMENTS; i++) {
        Axis &a(*motorAxis[i]);
        if (a.homing) {
            if (!a.enabled && a.pinMin != NOPIN) {
                return STATUS_AXIS_DISABLED;
            }
            a.position = a.home;
            searchDelay = max(searchDelay, a.searchDelay);
        }
    }

    if (stepHome(32, searchDelay) > 0) {
        return STATUS_BUSY_MOVING;
    }

    return STATUS_OK;
}

Status Machine::moveTo(Quad<StepCoord> destination, float seconds) {
    int32_t micsLeft = seconds * 1000000;
    Quad<StepCoord> delta(destination - getMotorPosition());
    Quad<StepCoord> pos;
    int8_t td = 64;
    float tdSeconds = seconds / (td + 2);
    float tdEnd = tdSeconds * 2;
    //cout << "t:" << (tdSeconds*(td-2) + tdEnd*2) << endl;
    StepCoord maxSteps = 0;
    for (int8_t tn = 0; tn <= td; tn++) {
        Quad<StepCoord> segment;
        for (MotorIndex i = 0; i < MOTOR_COUNT; i++) {
            segment.value[i] = (tn * delta.value[i]) / td - pos.value[i];
        }
        float t = (tn == 0 || tn == td) ? tdEnd : tdSeconds;	// start/stop ramp
        //cout <<  t << " segment:" << segment.toString() << endl;
        Status status = moveDelta(segment, t);
        if (status < 0) {
            return status;
        }
        pos += segment;
    }
    return STATUS_OK;
}

Status Machine::moveDelta(Quad<StepCoord> delta, float seconds) {
    int32_t micsDelay = 0;
    if (delta.isZero()) {
        return STATUS_OK;	// at destination
    }
    int16_t usDelay = 0;
    StepCoord maxDelta = 0;
    for (MotorIndex i = 0; i < QUAD_ELEMENTS; i++) {
        if (delta.value[i]) {
            if (!motorAxis[i]->enabled) {
                return STATUS_AXIS_DISABLED;
            }
            maxDelta = max(maxDelta, delta.value[i]);
            usDelay = max(usDelay, motorAxis[i]->usDelay);
        }
    }
    for (StepCoord iStep = 1; iStep <= maxDelta; iStep++) {
        int8_t pulses = 0;
        for (MotorIndex i = 0; i < QUAD_ELEMENTS; i++) {
            StepCoord step = delta.value[i];
            Axis &a = *motorAxis[i];
            if (step == 0) {
                // do nothing
            } else if (-step >= iStep) {
                a.readAtMin(invertLim);
                if (a.atMin) {
                    return STATUS_LIMIT_MIN;
                }
                if (a.position <= a.travelMin) {
                    return STATUS_TRAVEL_MIN;
                }
                a.pulse(false);
                a.position--;
                pulses++;
            } else if (step >= iStep) {
                if (a.position >= a.travelMax) {
                    return STATUS_TRAVEL_MAX;
                }
                a.pulse(true);
                a.position++;
                pulses++;
            }
        }
        int16_t us = (usDelay - (pulses - 1) * PULSEAXIS_MICS);
        delayMics(us);
        micsDelay += usDelay;
    }
    delayMics(seconds * 1000000 - micsDelay);
    return STATUS_BUSY_MOVING;
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

void Machine::enable(bool active) {
    for (int i = 0; i < AXIS_COUNT; i++) {
        axis[i].enable(active);
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
Status Machine::step(const Quad<StepCoord> &pulse) {
    int16_t usDelay = 0;
    for (uint8_t i = 0; i < QUAD_ELEMENTS; i++) { // Pulse leading edges
        Axis &a(*motorAxis[i]);
        switch (pulse.value[i]) {
        case 1:
            if (!a.enabled) {
                return STATUS_AXIS_DISABLED;
            }
            if (a.position >= a.travelMax) {
                return STATUS_TRAVEL_MAX;
            }
            digitalWrite(a.pinDir, a.dirHIGH ? HIGH : LOW);
            digitalWrite(a.pinStep, HIGH);
            break;
        case 0:
            break;
        case -1:
            if (!a.enabled) {
                return STATUS_AXIS_DISABLED;
            }
            a.readAtMin(invertLim);
            if (a.atMin) {
                return STATUS_LIMIT_MIN;
            }
            if (a.position <= a.travelMin) {
                return STATUS_TRAVEL_MIN;
            }
            digitalWrite(a.pinDir, a.dirHIGH ? LOW : HIGH);
            digitalWrite(a.pinStep, HIGH);
            break;
        default:
            return STATUS_STEP_RANGE_ERROR;
        }
    }
    PULSE_DELAY;

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

        Status status = step(pulse);
        if (status != STATUS_OK) {
            return status;
        }
        pulses -= pulse;
    }

    return STATUS_OK;
}


int8_t Machine::stepHome(int16_t pulsesPerAxis, int16_t searchDelay) {
    int8_t pulses = 0;

    for (int8_t iPulse=0; iPulse<pulsesPerAxis; iPulse++) {
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
        if (motorAxis[i]->enabled) {
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
    for (int i = 0; i < QUAD_ELEMENTS; i++) {
        motorAxis[i]->position = position.value[i];
    }
}




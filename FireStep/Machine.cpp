#ifdef CMAKE
#include <cstring>
#endif
#include <cmath>
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

// A stepper pulse cycle requires 3 digitalWrite()'s for 
// step direction, pulse high, and pulse low.
// Arduino 16-MHz digitalWrite pair takes 3.833 microseconds,
// so a full stepper pulse cycle should take ~5.7 microseconds:
//   http://skpang.co.uk/blog/archives/323
// A4983 stepper driver pulse cycle requires 2 microseconds
// DRV8825 stepper driver requires 3.8 microseconds
//   http://www.ti.com/lit/ds/symlink/drv8825.pdf
// Therefore, for currently fashionable stepper driver chips,
// Arduino digitalWrite() is slow enough to be its own delay (feature!)
// If you need longer pulse time, just add a delay:
// #define PULSE_DELAY DELAY500NS /* increase pulse cycle by 1 microsecond */
#define PULSE_DELAY /* no delay */

#define STEPONE_MICS 6 /* microseconds for a single stepOne() invocation */

inline void stepOne(Axis &a, bool advance) {
	digitalWrite(a.pinDir, (advance == a.dirHIGH) ? HIGH : LOW); 
	digitalWrite(a.pinStep, HIGH);
	PULSE_DELAY;
	digitalWrite(a.pinStep, LOW);
}

#ifdef TEST
int32_t firestep::delayMicsTotal = 0;
#endif

/**
 * inline replacement for Arduino delayMicroseconds()
 */
inline void delayMics(int32_t usDelay) {
    if (usDelay > 0) {
#ifdef TEST
        delayMicsTotal += usDelay;
#endif
        while (usDelay-- > 0) {
            DELAY500NS;
            DELAY500NS;
        }
    }
}

Status Axis::enable(bool active) {
    if (pinEnable == NOPIN || pinStep == NOPIN || pinDir == NOPIN || pinMin == NOPIN) {
        return STATUS_NOPIN;
    }
    digitalWrite(pinEnable, active ? PIN_ENABLE : PIN_DISABLE);
    enabled = active;
    return STATUS_OK;
}

Machine::Machine()
    : invertLim(false), pDisplay(&nullDisplay), jsonPrettyPrint(false) {
    pinEnableHigh = false;
    for (int8_t i = 0; i < QUAD_ELEMENTS; i++) {
        setMotorAxis((MotorIndex)i, (AxisIndex)i);
    }
    setPin(axis[0].pinStep, X_STEP_PIN, OUTPUT);
    setPin(axis[0].pinDir, X_DIR_PIN, OUTPUT);
    setPin(axis[0].pinMin, X_MIN_PIN, INPUT);
    setPin(axis[0].pinEnable, X_ENABLE_PIN, OUTPUT, HIGH);
    setPin(axis[1].pinStep, Y_STEP_PIN, OUTPUT);
    setPin(axis[1].pinDir, Y_DIR_PIN, OUTPUT);
    setPin(axis[1].pinMin, Y_MIN_PIN, INPUT);
    setPin(axis[1].pinEnable, Y_ENABLE_PIN, OUTPUT, HIGH);
    setPin(axis[2].pinStep, Z_STEP_PIN, OUTPUT);
    setPin(axis[2].pinDir, Z_DIR_PIN, OUTPUT);
    setPin(axis[2].pinMin, Z_MIN_PIN, INPUT);
    setPin(axis[2].pinEnable, Z_ENABLE_PIN, OUTPUT, HIGH);

    for (int i = 0; i < MOTOR_COUNT; i++) {
        motor[i] = i;
    }
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
    for (int i = 0; i < QUAD_ELEMENTS; i++) {
        Axis &a(*motorAxis[i]);
        if (a.homing) {
            if (!a.enabled && a.pinMin != NOPIN) {
                return STATUS_AXIS_DISABLED;
            }
            a.position = a.home;
        }
    }

    if (stepHome() > 0) {
        return STATUS_BUSY_MOVING;
    }

    return STATUS_OK;
}

Status Machine::moveTo(Quad<StepCoord> destination, float seconds) {
	int32_t micsLeft = seconds * 1000000;
	Quad<StepCoord> delta(destination - getMotorPosition());
	Quad<StepCoord> pos;
	int8_t td = 64;
	float tdSeconds = seconds/(td+2);
	float tdEnd = tdSeconds*2;
	//cout << "t:" << (tdSeconds*(td-2) + tdEnd*2) << endl;
	StepCoord maxSteps = 0;
	for (int8_t tn=0; tn<=td; tn++) {
		Quad<StepCoord> segment;
		for (MotorIndex i=0; i<MOTOR_COUNT; i++) {
			segment.value[i] = (tn*delta.value[i])/td - pos.value[i];
		}
		float t = (tn==0 || tn==td) ? tdEnd : tdSeconds;	// start/stop ramp
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
                stepOne(a, false);
                a.position--;
                pulses++;
            } else if (step >= iStep) {
                if (a.position >= a.travelMax) {
                    return STATUS_TRAVEL_MAX;
                }
                stepOne(a, true);
                a.position++;
                pulses++;
            }
        }
		int16_t us = (usDelay - (pulses - 1)*STEPONE_MICS);
        delayMics(us);
		micsDelay += usDelay;
    }
	delayMics(seconds*1000000 - micsDelay);
    return STATUS_BUSY_MOVING;
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

// The step() method is the "stepper inner loop" that creates the
// stepper pulse train to QUAD_ELEMENTS steppers. The pulse parameter
// must have a value of -1, 0, or 1 for each stepper.
//
// Steppers can't be driven too quickly--they stall. Each
// axis has a usDelay field that specifies a minimum delay
// between pulses.
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

int8_t Machine::stepHome() {
    int16_t searchDelay = 0;
    int8_t pulses = 0;
    for (uint8_t i = 0; i < QUAD_ELEMENTS; i++) {
        Axis &a(*motorAxis[i]);
        if (a.homing) {
            a.readAtMin(invertLim);
            if (a.atMin) {
                a.homing = false;
                for (StepCoord lb = a.latchBackoff; lb > 0; lb--) {
                    stepOne(a, true);
                    delayMics(a.searchDelay);
                }
            } else {
                stepOne(a, false);
                searchDelay = max(searchDelay, a.searchDelay);
                pulses++;
            }
        }
    }

    delayMics(searchDelay); // maximum pulse rate throttle
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




#ifdef CMAKE
#include <cstring>
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

Status Axis::enable(bool active) {
    if (pinEnable == NOPIN || pinStep == NOPIN || pinDir == NOPIN || pinMin == NOPIN) {
        return STATUS_NOPIN;
    }
    digitalWrite(pinEnable, active ? PIN_ENABLE : PIN_DISABLE);
    enabled = active;
    return STATUS_OK;
}

Machine::Machine()
    : invertLim(false), pDisplay(&nullDisplay) {
    pinEnableHigh = false;
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
        motor[i].axisMap = i;
    }
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
// stepper pulse train to 4 steppers. The pulse parameter
// must have a value of -1, 0, or 1 for each stepper.
//
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
//
// Steppers can't be driven too quickly--they stall. Each
// axis has a usDelay field that specifies a minimum delay
// between pulses.
#define PULSE_DELAY /* no delay */
Status Machine::step(const Quad<StepCoord> &pulse) {
	int16_t usDelay = 0;
    for (int i = 0; i < 4; i++) { // Pulse leading edges
        Axis &a(axis[motor[i].axisMap]);
        switch (pulse.value[i]) {
        case 1:
            if (!a.enabled) {
                return STATUS_AXIS_DISABLED;
            }
            if (a.position >= a.travelMax) {
                return STATUS_TRAVEL_MAX;
            }
            digitalWrite(a.pinDir, a.invertDir ? LOW : HIGH);
            digitalWrite(a.pinStep, HIGH);
			usDelay = max(usDelay, a.usDelay);
            break;
        case 0:
            break;
        case -1:
            if (!a.enabled) {
                return STATUS_AXIS_DISABLED;
            }
            a.readAtMin(invertLim);
            if (a.atMin || a.position <= a.travelMin) {
                return STATUS_TRAVEL_MIN;
            }
            digitalWrite(a.pinDir, a.invertDir ? HIGH : LOW);
            digitalWrite(a.pinStep, HIGH);
			usDelay = max(usDelay, a.usDelay);
            break;
        default:
            return STATUS_STEP_RANGE_ERROR;
        }
    }
	PULSE_DELAY;

    for (int i = 0; i < 4; i++) { // Pulse trailing edges
        if (pulse.value[i]) {
            Axis &a(axis[motor[i].axisMap]);
            digitalWrite(a.pinStep, LOW);
            a.position += pulse.value[i];
        }
    }

	delayMicroseconds(usDelay); // maximum pulse rate throttle

    return STATUS_OK;
}

/**
 * Return position of currently driven axes bound to motors
 */
Quad<StepCoord> Machine::getMotorPosition() {
    return Quad<StepCoord>(
               axis[motor[0].axisMap].position,
               axis[motor[1].axisMap].position,
               axis[motor[2].axisMap].position,
               axis[motor[3].axisMap].position
           );
}

/**
 * Set position of currently driven axes bound to motors
 */
void Machine::setMotorPosition(const Quad<StepCoord> &position) {
    for (int i = 0; i < 4; i++) {
        axis[motor[i].axisMap].position = position.value[i];
    }
}




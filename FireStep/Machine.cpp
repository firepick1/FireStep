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

	for (int i=0; i<MOTOR_COUNT; i++) {
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

void Machine::init() {
}

Status Machine::step(const Quad<StepCoord> &pulse) {
    for (int i = 0; i < 4; i++) {
        Axis &a(axis[motor[i].axisMap]);
        a.readAtMin(invertLim);
        if (a.pinStep == NOPIN || a.pinDir == NOPIN) {
            continue;
        }
        switch (pulse.value[i]) {
        case 1:
            if (a.position >= a.travelMax) {
                continue;
            }
            if (!a.enabled) {
                return STATUS_AXIS_DISABLED;
            }
            digitalWrite(a.pinDir, a.invertDir ? LOW : HIGH);
            break;
        case 0:
            continue;
        case -1:
            if (a.atMin || a.position <= a.travelMin) {
                continue;
            }
            if (!a.enabled) {
                return STATUS_AXIS_DISABLED;
            }
            digitalWrite(a.pinDir, a.invertDir ? HIGH : LOW);
            break;
        default:
            return STATUS_STEP_RANGE_ERROR;
        }
        digitalWrite(a.pinStep, LOW);
    }
    STEPPER_PULSE_DELAY;
    for (int i = 0; i < 4; i++) {
        Axis &a(axis[motor[i].axisMap]);
        if (!a.enabled || a.pinStep == NOPIN || a.pinDir == NOPIN) {
            continue;
        }
        switch (pulse.value[i]) {
        case 1:
            if (a.position >= a.travelMax) {
                continue;
            }
            break;
        case 0:
            continue;
        case -1:
            if (a.atMin || a.position <= a.travelMin) {
                continue;
            }
            break;
        default:
            return STATUS_STEP_RANGE_ERROR;
        }
        digitalWrite(a.pinStep, HIGH);
        a.position += pulse.value[i];
    }

    return STATUS_OK;
}

/**
 * Return position of currently driven axes bound to motors
 */
Quad<StepCoord> Machine::motorPosition() {
    return Quad<StepCoord>(
               axis[motor[0].axisMap].position,
               axis[motor[1].axisMap].position,
               axis[motor[2].axisMap].position,
               axis[motor[3].axisMap].position
           );
}




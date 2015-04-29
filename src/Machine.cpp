#include <cstring>
#include "Arduino.h"
#include "Machine.h"
#include "AnalogRead.h"
#include "version.h"
#include "build.h"
#ifdef TEST
#include "../test/FireUtils.hpp"
#endif

using namespace firestep;

template class Quad<int16_t>;
template class Quad<int32_t>;

#ifndef ASM
#define ASM(op) asm(op)
#endif

void MachineThread::setup() {
    id = 'M';
#ifdef THROTTLE_SPEED
    ADC_LISTEN8(ANALOG_SPEED_PIN);
#endif
    Thread::setup();
}

void MachineThread::Heartbeat() {
#ifdef THROTTLE_SPEED
    controller.speed = ADCH;
    if (controller.speed <= 251) {
        ThreadEnable(false);
        for (byte iPause = controller.speed; iPause <= 247; iPause++) {
            for (byte iIdle = 0; iIdle < 10; iIdle++) {
				DELAY500NS;
				DELAY500NS;
            }
        }
        ThreadEnable(true);
    }
#endif

	nextHeartbeat.ticks = 0;
}

Machine::Machine() 
	: invertLim(false) {
	axis[0].pinStep = X_STEP_PIN;
	axis[0].pinDir = X_DIR_PIN;
	axis[0].pinMin = X_MIN_PIN;
	axis[0].pinEnable = X_ENABLE_PIN;
	axis[1].pinStep = Y_STEP_PIN;
	axis[1].pinDir = Y_DIR_PIN;
	axis[1].pinMin = Y_MIN_PIN;
	axis[1].pinEnable = Y_ENABLE_PIN;
	axis[2].pinStep = Z_STEP_PIN;
	axis[2].pinDir = Z_DIR_PIN;
	axis[2].pinMin = Z_MIN_PIN;
	axis[2].pinEnable = Z_ENABLE_PIN;

	for (int i=0; i<MOTOR_COUNT; i++) {
		motor[i].axisMap = i;
	}
}

void Machine::init() {
}

Status Machine::step(const Quad<StepCoord> &pulse) {
    for (int i = 0; i < 4; i++) {
		int iAxis = motor[i].axisMap;
#ifdef TEST
		ASSERT(0<=iAxis && iAxis<AXIS_COUNT);
#endif
        Axis &a(axis[iAxis]);
        if (a.pinMin != NOPIN) {
			bool minHigh = digitalRead(a.pinMin);
            bool atMin = (invertLim == !minHigh);
            if (atMin != a.atMin) {
#ifdef TEST_TRACE
                cout << "axis[" << i << "] CHANGE" 
					<< " atMin:" << atMin 
					<< " pinMin:" << (int) a.pinMin
					<< endl;
#endif
                a.atMin = atMin;
            }
        }
        if (a.pinStep == NOPIN || a.pinDir == NOPIN) {
#ifdef TEST_TRACE
            cout << "axis[" << i << "] NOPIN" << endl;
#endif
            continue;
        }
        switch (pulse.value[i]) {
        case 1:
            if (a.position >= a.travelMax) {
#ifdef TEST_TRACE
                cout << "axis[" << i << "] travelMax:" << a.travelMax << endl;
#endif
                continue;
            }
            digitalWrite(a.pinDir, a.invertDir ? LOW : HIGH);
            break;
        case 0:
            continue;
        case -1:
            if (a.atMin) {
#ifdef TEST_TRACE
                cout << "axis[" << i << "] atMin:" << endl;
#endif
                continue;
            }
            if (a.position <= a.travelMin) {
#ifdef TEST_TRACE
                cout << "axis[" << i << "] travelMin:" << a.travelMin;
#endif
                continue;
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
        if (a.pinStep == NOPIN || a.pinDir == NOPIN) {
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
            if (a.atMin || (a.position <= a.travelMin)) {
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




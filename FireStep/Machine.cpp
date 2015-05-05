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

///////////////// Axis ////////////////

Axis::Axis() :
    mode((uint8_t)MODE_STANDARD),
    pinStep(NOPIN),
    pinDir(NOPIN),
    pinMin(NOPIN),
    pinMax(NOPIN),
    pinEnable(NOPIN),
    travelMin(0),
    travelMax(10000),
    searchVelocity(200),
    position(0),
    stepAngle(1.8),
    microsteps(16),
    invertDir(0),				// 0:normal direction, 1:inverted direction
    powerManagementMode(0),	// 0:off, 1:on, 2:on in cycle, 3:on when moving
    atMin(false),
    atMax(false)
{
}

void Axis::readLimits(bool invertLim) {
    if (pinMin != NOPIN) {
        bool minHigh = digitalRead(pinMin);
        bool atMinNew = (invertLim == !minHigh);
        if (atMinNew != atMin) {
            atMin = atMinNew;
        }
    }
    if (pinMax != NOPIN) {
        bool maxHigh = digitalRead(pinMax);
        bool atMaxNew = (invertLim == !maxHigh);
        if (atMaxNew != atMax) {
            atMax = atMaxNew;
        }
    }
}

//////////////// Machine ////////////////

Machine::Machine()
    : invertLim(false), pDisplay(&nullDisplay) {
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

    for (int i = 0; i < MOTOR_COUNT; i++) {
        motor[i].axisMap = i;
    }
}

void Machine::init() {
}

Status Machine::step(const Quad<StepCoord> &pulse) {
    for (int i = 0; i < 4; i++) {
        Axis &a(axis[motor[i].axisMap]);
        if (a.pinEnable != NOPIN) {
            bool enable = digitalRead(a.pinEnable);
            if (!enable) {
                a.mode = MODE_DISABLE;
            }
        }
        if (a.mode == MODE_DISABLE) {
            continue;
        }
        a.readLimits(invertLim);
        if (a.pinStep == NOPIN || a.pinDir == NOPIN) {
            continue;
        }
        switch (pulse.value[i]) {
        case 1:
            if (a.position >= a.travelMax) {
                continue;
            }
            digitalWrite(a.pinDir, a.invertDir ? LOW : HIGH);
            break;
        case 0:
            continue;
        case -1:
            if (a.atMin || a.position <= a.travelMin) {
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
        if (a.mode == MODE_DISABLE || a.pinStep == NOPIN || a.pinDir == NOPIN) {
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




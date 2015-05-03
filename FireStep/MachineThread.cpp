#ifdef CMAKE
#include <cstring>
#endif
#include "Arduino.h"
#include "AnalogRead.h"
#include "version.h"
#include "MachineThread.h"

using namespace firestep;

void MachineThread::setup() {
    id = 'M';
#ifdef THROTTLE_SPEED
    ADC_LISTEN8(ANALOG_SPEED_PIN);
#endif
    Thread::setup();
	machine.pDisplay->setup();
	status = STATUS_SETUP;
}

MachineThread::MachineThread()
    : status(STATUS_IDLE) {
}

void MachineThread::displayStatus() {
    switch (status) {
    case STATUS_OK:
    case STATUS_IDLE:
        machine.pDisplay->setStatus(DISPLAY_WAIT_IDLE);
        break;
    case STATUS_SERIAL_EOL_WAIT:
        machine.pDisplay->setStatus(DISPLAY_WAIT_EOL);
        break;
    case STATUS_DISPLAY_CAMERA:
        machine.pDisplay->setStatus(DISPLAY_WAIT_CAMERA);
        break;
    case STATUS_DISPLAY_OPERATOR:
        machine.pDisplay->setStatus(DISPLAY_WAIT_OPERATOR);
        break;
    case STATUS_SETUP:
        machine.pDisplay->setStatus(DISPLAY_BUSY_SETUP);
        break;
    case STATUS_DISPLAY_BUSY:
    case STATUS_PROCESSING:
        machine.pDisplay->setStatus(DISPLAY_BUSY);
        break;
    case STATUS_DISPLAY_MOVING:
        machine.pDisplay->setStatus(DISPLAY_BUSY_MOVING);
        break;
    default:	// errors
        if (status < 0) {
            machine.pDisplay->setStatus(DISPLAY_WAIT_ERROR);
        } else {
            machine.pDisplay->setStatus(DISPLAY_BUSY);
        }
        break;
    }

    machine.pDisplay->show();
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

    switch (status) {
	default:
    case STATUS_IDLE:
        if (Serial.available()) {
            command.clear();
            status = command.parse();
        }
        break;
    case STATUS_SERIAL_EOL_WAIT:
        if (Serial.available()) {
            status = command.parse();
        }
        break;
    case STATUS_JSON_PARSED:
    case STATUS_PROCESSING:
        status = controller.process(machine, command);
		if (status != STATUS_PROCESSING) {
			command.response().printTo(Serial);
			Serial.println();
		}
        break;
	case STATUS_SETUP:
    case STATUS_OK:
        status = STATUS_IDLE;
        break;
    }

    displayStatus();

    nextHeartbeat.ticks = 0; // Highest priority
}


#ifdef CMAKE
#include <cstring>
#endif
#include "Arduino.h"
#include "AnalogRead.h"
#include "version.h"
#include "MachineThread.h"

using namespace firestep;

void MachineThread::setup(PinConfig pc) {
    id = 'M';
#ifdef THROTTLE_SPEED
    ADC_LISTEN8(ANALOG_SPEED_PIN);
#endif
    Thread::setup();
    machine.pDisplay->setup();
	machine.setPinConfig(pc);
    status = STATUS_BUSY_SETUP;
    displayStatus();
    controller.setup();
}

MachineThread::MachineThread()
    : status(STATUS_WAIT_IDLE) , controller(machine) {
}

void MachineThread::displayStatus() {
    switch (status) {
    case STATUS_OK:
    case STATUS_WAIT_IDLE:
        machine.pDisplay->setStatus(DISPLAY_WAIT_IDLE);
        break;
    case STATUS_WAIT_EOL:
        machine.pDisplay->setStatus(DISPLAY_WAIT_EOL);
        break;
    case STATUS_WAIT_CAMERA:
        machine.pDisplay->setStatus(DISPLAY_WAIT_CAMERA);
        break;
    case STATUS_WAIT_OPERATOR:
        machine.pDisplay->setStatus(DISPLAY_WAIT_OPERATOR);
        break;
    case STATUS_BUSY_SETUP:
    case STATUS_WAIT_BUSY:
    case STATUS_BUSY:
        machine.pDisplay->setStatus(DISPLAY_BUSY);
        break;
    case STATUS_WAIT_CANCELLED:
        machine.pDisplay->setStatus(DISPLAY_WAIT_CANCELLED);
        break;
    case STATUS_BUSY_MOVING:
    case STATUS_WAIT_MOVING:
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

void MachineThread::loop() {
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
    case STATUS_WAIT_IDLE:
    case STATUS_WAIT_CAMERA:
    case STATUS_WAIT_OPERATOR:
    case STATUS_WAIT_MOVING:
    case STATUS_WAIT_BUSY:
    case STATUS_WAIT_CANCELLED:
        if (Serial.available()) {
            command.clear();
            status = command.parse();
        } else {
            machine.idle();
        }
        break;
    case STATUS_WAIT_EOL:
        if (Serial.available()) {
            status = command.parse();
        }
        break;
    case STATUS_BUSY_PARSED:
    case STATUS_BUSY:
    case STATUS_BUSY_MOVING:
        if (Serial.available()) {
            status = controller.cancel(command, STATUS_SERIAL_CANCEL);
        } else {
            status = controller.process(command);
        }
        break;
    case STATUS_BUSY_SETUP: {
        char buf[100];
        machine.enable(true);
        snprintf(buf, sizeof(buf), "FireStep %d.%d.%d",
                 VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
        Serial.println(buf);
        status = STATUS_WAIT_IDLE;
        break;
    }
    case STATUS_OK:
        status = STATUS_WAIT_IDLE;
        break;
    }

    displayStatus();

    nextLoop.ticks = 0; // Highest priority
}


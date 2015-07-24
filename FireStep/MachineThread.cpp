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
    case STATUS_BUSY_OK:
    case STATUS_BUSY_SETUP:
    case STATUS_BUSY_EEPROM:
    case STATUS_WAIT_BUSY:
    case STATUS_BUSY:
        machine.pDisplay->setStatus(DISPLAY_BUSY);
        break;
    case STATUS_WAIT_CANCELLED:
        machine.pDisplay->setStatus(DISPLAY_WAIT_CANCELLED);
        break;
    case STATUS_BUSY_CALIBRATING:
        machine.pDisplay->setStatus(DISPLAY_BUSY_CALIBRATING);
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

size_t MachineThread::readEEPROM(uint8_t *eeprom_addr, char *dst, size_t maxLen) {
    uint8_t c = eeprom_read_byte(eeprom_addr);
    if (!dst || (c != '{' && c != '[')) {
		return 0;
	}

	size_t len = 0;
	for (len=0; len<maxLen-2; len++) {
		c = eeprom_read_byte(eeprom_addr+len);
		if (c == 255 || c == 0) {
			break;
		}
		dst[len] = c;
	}
	dst[len] = 0;

	return len;
}

Status MachineThread::executeEEPROM() {
	command.clear();
	char *buf = command.allocate(MAX_JSON);
	ASSERT(buf);

	size_t len = 0;
	buf[len++] = '[';
	len += readEEPROM((uint8_t*)(size_t) 0, buf+len, MAX_JSON-len);
	if (len > 1) {
		buf[len++] = ',';
	}
	size_t len2 = readEEPROM((uint8_t*)(size_t) machine.eeUser, buf+len, MAX_JSON-len);
	if (len2==0 && len>1) {
		len--; // remove comma
	} else {
		if (buf[len] == '[') {
			buf[len] = ' ';
		}
		len += len2;
	}
	if (buf[len-1] != ']') {
		buf[len++] = ']';
	}
	buf[len] = 0;

	TESTCOUT3("executeEEPROM:", buf, " len:", len, " status:", (int) status);
	status = command.parse(buf, status);
	TESTCOUT1("executeEEPROM status:", (int) status);

	return status;
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
            status = command.parse(NULL, status);
        } else {
            machine.idle();
        }
        break;
    case STATUS_WAIT_EOL:
        if (Serial.available()) {
            status = command.parse(NULL, status);
        }
        break;
    case STATUS_BUSY_PARSED:
    case STATUS_BUSY_OK:
    case STATUS_BUSY:
    case STATUS_BUSY_CALIBRATING:
    case STATUS_BUSY_MOVING:
        if (Serial.available()) {
            status = controller.cancel(command, STATUS_SERIAL_CANCEL);
        } else {
            status = controller.process(command);
        }
        break;
    case STATUS_BUSY_EEPROM:
        status = executeEEPROM();
        break;
    case STATUS_BUSY_SETUP: {
        uint8_t c = eeprom_read_byte((uint8_t*) 0);
        if (c == '{' || c == '[') {
            status = STATUS_BUSY_EEPROM;
        } else {
            char msg[100];
            snprintf(msg, sizeof(msg), "FireStep %d.%d.%d",
                     VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
            Serial.println(msg);
            status = STATUS_WAIT_IDLE;
        }
        break;
    }
    case STATUS_OK:
        status = STATUS_WAIT_IDLE;
        break;
    }

    displayStatus();

    nextLoop.ticks = 0; // Highest priority
}


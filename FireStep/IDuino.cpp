#include "IDuino.h"
#include "ProgMem.h"
#include "MachineThread.h"

using namespace firestep;

bool IDuino::selftest() {
	IDuino* pDuino = this;
	char buf[100];

	TESTCOUT1("IDuino selftest:", "begin");
	PM_strcpy(buf, iduino_selftest);
	pDuino->serial_println();
	pDuino->serial_println(buf);
	PM_strcpy(buf, iduino_println);
	pDuino->serial_println(buf);

	// Turn on pin #4 (SERVO1) LED
	TESTCOUT1("IDuino selftest:", "Turn on pin #4");
	PM_strcpy(buf, iduino_pinMode);
	pDuino->serial_println(buf);
	pDuino->pinMode(PC1_SERVO1, OUTPUT);
	PM_strcpy(buf, iduino_led_on);
	pDuino->serial_println(buf);
	pDuino->digitalWrite(PC1_SERVO1, HIGH);
	PM_strcpy(buf, iduino_enableTicks);
	pDuino->serial_println(buf);

	// Start ticks()
	TESTCOUT1("IDuino selftest:", "Start ticks()");
	pDuino->enableTicks(true);
	if (!pDuino->isTicksEnabled()) {
		PM_strcpy(buf, iduino_error);
		pDuino->serial_println(buf);
		return false;
	}
	Ticks tStart = pDuino->ticks();
	PM_strcpy(buf, iduino_ticks);
	pDuino->serial_print(buf);
	pDuino->serial_println((int)tStart);
	pDuino->delay(1000);
	Ticks tEnd = pDuino->ticks();
	PM_strcpy(buf, iduino_ticks);
	pDuino->serial_print(buf);
	pDuino->serial_println((int)tEnd);
	Ticks tElapsed = tEnd - tStart;
	if (tElapsed < MS_TICKS(1000)) {
		PM_strcpy(buf, iduino_error);
		pDuino->serial_println(buf);
		return false;
	}

	// Turn off pin #4 (SERVO1)
	PM_strcpy(buf, iduino_led_off);
	pDuino->serial_println(buf);
	pDuino->digitalWrite(PC1_SERVO1, LOW);

	// Sizes
	PM_strcpy(buf, iduino_size_mach);
	pDuino->serial_print(buf);
	int bytes = (int) sizeof(Machine);
	pDuino->serial_println(bytes);
	bytes = (int) sizeof(MachineThread);
	PM_strcpy(buf, iduino_size_mt);
	pDuino->serial_print(buf);
	pDuino->serial_println(bytes);
	bytes = (int) pDuino->minFreeRam();
	PM_strcpy(buf, iduino_free);
	pDuino->serial_print(buf);
	pDuino->serial_println(bytes);

	// Read something
	TESTCOUT1("IDuino selftest:", "serial_read()");
	PM_strcpy(buf, iduino_read);
	pDuino->serial_println(buf);
	uint32_t msStart = pDuino->millis();
	int i = 0;
	for (char c; i+2<sizeof(buf) && pDuino->millis()-msStart<10000; ) {
		if (!pDuino->serial_available()) {
			pDuino->delay(1000);
			TESTCOUT1("IDuino selftest:", "waiting for input...");
			continue;
		}
		c = pDuino->serial_read();
		if (c == '\r' || c == '\n') {
			buf[i++] = 0;
			pDuino->serial_println(buf);
			break;
		}
		buf[i++] = c;
	}
	if (i == 0) {
		PM_strcpy(buf, iduino_error);
		pDuino->serial_println(buf);
		return false;
	}

	TESTCOUT1("IDuino selftest:", "done");
	PM_strcpy(buf, iduino_done);
	pDuino->serial_println(buf);

	return true;
}

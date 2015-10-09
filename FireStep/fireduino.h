#ifndef FIREDUINO_H
#define FIREDUINO_H

#ifdef __AVR_ATmega2560__
#include "fireduino_mega2560.h"
#else
#include "MockDuino.h"
#endif

namespace fireduino {
	inline void serial_println() {
		serial_print('\n');
	}
	inline void serial_println(const char value) {
		serial_print(value);
		serial_print('\n');
	}
	inline void serial_println(const char* value) {
		serial_print(value);
		serial_print('\n');
	}
}

#endif

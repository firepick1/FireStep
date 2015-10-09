#ifndef FIREDUINO_H
#define FIREDUINO_H

#if defined(FIREDUINO_API) 
namespace fireduino {
	//////////////////// ARDUINO SPECIFIC ///////////////////
	Print& get_Print();
	int16_t serial_read();
	int16_t serial_available();
	void serial_begin(int32_t baud);
	void serial_print(const char *value);
	void serial_print(const char value);
	void serial_print(int16_t value, int16_t format = DEC);
	void pinMode(int16_t pin, int16_t inout);
	int16_t digitalRead(int16_t pin);
	void digitalWrite(int16_t dirPin, int16_t value);
	void analogWrite(int16_t dirPin, int16_t value);
	int16_t analogRead(int16_t dirPin);
	void delay(int ms);
	void delayMicroseconds(uint16_t usDelay);
	uint8_t eeprom_read_byte(uint8_t *addr);
	void	eeprom_write_byte(uint8_t *addr, uint8_t value);

	////////////////// FIRESTEP SPECIFIC ///////////////////
	/**
	 /* IMPORTANT!!!
	 /* The digitalWrite/digitalRead methods match the Arduino
	 /* with one critical difference. They must take at least
	 /* 1 microsecond to complete. This constraint ensures that
	 /* pulse generation will generate the 2 microsecond pulse
	 /* required by DRV8825. When implementing IDuino for fast
	 /* CPUs, take care to observe this limitation.
	 /*/
	void pulseFast(uint8_t pin);
	void delay_stepper_pulse(); // delay for 2 microsecond rise-to-fall stepper pulse width
	/**
	 * With the standard ATMEGA 16,000,000 Hz system clock and TCNT1 / 1024 prescaler:
	 * 1 tick = 1024 clock cycles = 64 microseconds
	 * Clock overflows in 2^31 * 0.000064 seconds = ~38.1 hours
	 */
	uint16_t get_timer1();
	void setup_timer1();
	void clear_timer1();
	void enable_timer1(bool enable);
} // namespace fireduino

#elif defined( __AVR_ATmega2560__)
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

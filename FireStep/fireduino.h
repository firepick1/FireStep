#ifndef FIREDUINO_H
#define FIREDUINO_H

#define minval(a,b) ((a)<(b)?(a):(b))
#define maxval(a,b) ((a)>(b)?(a):(b))
#define absval(x) ((x)>0?(x):-(x))
#define roundval(x)     ((x)>=0?(long)((x)+0.5):(long)((x)-0.5))
#ifndef radians
#define radians(deg) ((deg)*DEG_TO_RAD)
#define degrees(rad) ((rad)*RAD_TO_DEG)
#endif

#if defined(FIREDUINO_API) 
namespace fireduino { // abstract API implementable any way you like
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
	uint32_t millis();
	void delay(int ms);
	void delayMicroseconds(uint16_t usDelay);
	uint8_t eeprom_read_byte(uint8_t *addr);
	void	eeprom_write_byte(uint8_t *addr, uint8_t value);

	////////////////// FIRESTEP SPECIFIC ///////////////////
	void pulseFast(uint8_t pin); // >= 2 microsecond rise-to-fall stepper pulse width
	/*
	 * The following is called between a digitalWrite(HIGH/LOW) pair
	 * to ensure a stepper pulse is >= 2 microseconds. On the Arduino,
	 * this does nothing since digitalWrite() is so slow.
	 */
	void delay_stepper_pulse(); 
	uint16_t get_timer1(); // ticks @ 64 microseconds
	void setup_timer1();
	void clear_timer1();
	void enable_timer1(bool enable);
	int16_t freeRam ();
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

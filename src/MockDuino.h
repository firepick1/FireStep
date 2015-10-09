#ifndef MOCKDUINO_H
#define MOCKDUINO_H

#include <stdint.h>

namespace mockduino {
	int16_t	digitalRead(int16_t pin);
	void digitalWrite(int16_t dirPin, int16_t value);
	int16_t analogRead(int16_t pin);
	void analogWrite(int16_t pin, int16_t value);
	void pinMode(int16_t pin, int16_t inout);
	void delay(int ms);
	uint8_t eeprom_read_byte(uint8_t *addr);
	void eeprom_write_byte(uint8_t *addr, uint8_t value);
}

#endif

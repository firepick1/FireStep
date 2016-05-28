#ifndef FIREDUINO_DUE_H
#define FIREDUINO_DUE_H

#include <Arduino.h>
#if defined(_SAM3XA_)
#include "DueTimer.h"
#endif
extern uint32_t fireduino_timer;
extern void fireduino_timer_handler();

#define EEPROM_BYTES 0 /* DUE HAS NO EEPROM */
#define EEPROM_END 0

// The choice of stepper pulse delay is critical. FireStep can drive up to 4
// steppers concurrently each at up to 25,000 pulses per second. 
// Since FireStep timeslices its pulse generation, FireStep must emit 100,000
// pulses per second. That pulse rate requires that a pulse be generated every
// 10 microseconds. Stepper drivers require minimal pulse widths for the high
// and low portion of the pulse waveform (e.g., DRV8824 requires 2us for each).
// Although the Arduino is capable of generating short pulses, FireStep uses
// longer pulses to ensure that the stepper driver microstep indexer is triggered
// properly.
#define STEPPER_PULSE_DELAY 4

namespace fireduino {
	inline Print& get_Print() {
		return Serial;
	}
	inline int16_t serial_read() {
		return Serial.read();
	}
	inline int16_t serial_available() {
		return Serial.available();
	}
	inline void serial_begin(int32_t baud) {
		Serial.begin(baud);
	}
	inline void serial_print(const char *value) {
		Serial.print(value);
	}
	inline void serial_print(const char value) {
		Serial.print(value);
	}
	inline void serial_print(int16_t value, int16_t format = DEC) {
		Serial.print(value, format);
	}
	inline void pinMode(int16_t pin, int16_t inout) {
		::pinMode(pin, inout);
	}
	inline int16_t digitalRead(int16_t pin) {
		return ::digitalRead(pin);
	}
	inline void digitalWrite(int16_t dirPin, int16_t value) {
		::digitalWrite(dirPin, value);
	}
	inline void analogWrite(int16_t dirPin, int16_t value) {
		::analogWrite(dirPin, value);
	}
	inline int16_t analogRead(int16_t dirPin) {
		return ::analogRead(dirPin);
	}
	inline void pulseFast(uint8_t pin) { // >=2 microsecond pulse
		digitalWrite(pin, HIGH);
		delayMicroseconds(STEPPER_PULSE_DELAY); 
		digitalWrite(pin, LOW);
	}
	inline void delay(int ms) {
		::delay(ms);
	}
	inline void delayMicroseconds(uint16_t usDelay) {
		::delayMicroseconds(usDelay);
	}
	inline uint8_t eeprom_read_byte(uint8_t *addr) {
		return 0; // DUE HAS NO EEPROM
	}
	inline void	eeprom_write_byte(uint8_t *addr, uint8_t value) {
		// DUE HAS NO EEPROM
	}
	inline uint32_t millis() {
		return ::millis();
	}
	inline void delay_stepper_pulse() {
		delayMicroseconds(STEPPER_PULSE_DELAY);
	}
	inline uint32_t get_timer64us() {
		return ::fireduino_timer;
	}
	inline void enable_timer64us(bool enable) {
		if (enable) {
			Timer1.start(64);
		} else {
			Timer1.stop();
		}
	}
	inline void setup_timer64us() {
		Timer1.attachInterrupt(fireduino_timer_handler);
	}
	inline void clear_timer64us() {
		::fireduino_timer = 0;
	}
	inline int16_t freeRam () {
		return 9999; // We don't know and we don't care
	}
} // namespace fireduino

#endif

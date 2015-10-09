#ifndef FIREDUINO_H
#define FIREDUINO_H

#ifdef Arduino_h
namespace fireduino {
inline int16_t digitalRead(int16_t pin) {
	return ::digitalRead(pin);
}
} // fireduino
#else
#include "MockDuino.h"
namespace fireduino {

inline Print& get_Print() {
	return mockduino::get_Print();
}
inline uint8_t serial_read() {
	return mockduino::serial_read();
}
inline int16_t serial_available() {
	return mockduino::serial_available();
}
inline void serial_begin(int32_t baud) {
	mockduino::serial_begin(baud);
}
inline void serial_print(const char *value) {
	mockduino::serial_print(value);
}
inline void serial_print(const char value) {
	mockduino::serial_print(value);
}
inline void serial_print(int16_t value, int16_t format = DEC) {
	mockduino::serial_print(value, format);
}
inline void pinMode(int16_t pin, int16_t inout) {
	mockduino::pinMode(pin, inout);
}
inline int16_t digitalRead(int16_t pin) {
	return mockduino::digitalRead(pin);
}
inline void digitalWrite(int16_t dirPin, int16_t value) {
	mockduino::digitalWrite(dirPin, value);
}
inline void analogWrite(int16_t dirPin, int16_t value) {
	mockduino::analogWrite(dirPin, value);
}
inline int16_t analogRead(int16_t dirPin) {
	return mockduino::analogRead(dirPin);
}
/**
 /* IMPORTANT!!!
 /* The digitalWrite/digitalRead methods match the Arduino
 /* with one critical difference. They must take at least
 /* 1 microsecond to complete. This constraint ensures that
 /* pulse generation will generate the 2 microsecond pulse
 /* required by DRV8825. When implementing IDuino for fast
 /* CPUs, take care to observe this limitation.
 /*/
inline void pulseFast(uint8_t pin) {
	digitalWrite(pin, HIGH);
	digitalWrite(pin, LOW);
}
inline void delay(int ms) {
	mockduino::delay(ms);
}
inline void delayMicroseconds(uint16_t usDelay) {
	mockduino::delayMicroseconds(usDelay);
}
inline uint8_t eeprom_read_byte(uint8_t *addr) {
	return mockduino::eeprom_read_byte(addr);
}
inline void	eeprom_write_byte(uint8_t *addr, uint8_t value) {
	mockduino::eeprom_write_byte(addr, value);
}
//public: // FireStep
    //Ticks ticks(bool peek=false);
    //virtual void enableTicks(bool enable);
    //virtual bool isTicksEnabled();
//
//public: // Testing: other
    //void dump();
    //void clear(); // NOT setup()! clears mock eeprom
    //int16_t& MEM(int addr);
    //void delay500ns();
    //void setTicks(Ticks value);
    //int16_t getPinMode(int16_t pin);
    //int16_t getPin(int16_t pin);
    //void setPin(int16_t pin, int16_t value);
    //void setPinMode(int16_t pin, int16_t value);
    //int32_t pulses(int16_t pin);
    //uint32_t get_usDelay() {
        //return usDelay;
    //}
} // fireduino
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

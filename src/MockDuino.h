#ifndef MOCKDUINO_H
#define MOCKDUINO_H

#include <iostream>
#include <sstream>
#include <iomanip>
//#include <stdint.h>
//#include "../ArduinoJson/include/ArduinoJson/Arduino/Print.hpp"

#include <string>
//#include <sstream>
#include <stdint.h>
#include "../ArduinoJson/include/ArduinoJson/Arduino/Print.hpp"
#include "fireduino_types.h"

#define EEPROM_BYTES 512 /* Actual capacity will be less because eeprom buffer is part of MAX_JSON */
#define EEPROM_END 4096

#define NOVALUE 32767 /* 0x77FF */
#define NOVALUESTR "32767"

#define ADCH arduino.MEM(0)
#define ADCSRA arduino.MEM(1)
#define ADCSRB arduino.MEM(2)
#define ADMUX arduino.MEM(3)
#define CLKPR arduino.MEM(4)
#define DIDR0 arduino.MEM(5)
#define PRR arduino.MEM(6)
#define SREGI arduino.MEM(7)
#define TCCR1A arduino.MEM(8)
#define TCCR1B arduino.MEM(9)
#define TCNT1 arduino.MEM(10)
#define TIMSK1 arduino.MEM(11)

#define ADEN 7
#define ADSC 6
#define ADATE 5
//#define ADFR 5
#define ADIF 4
#define ADIE 3
#define ADPS2 2
#define ADPS1 1
#define ADPS0 0
//MUX bit definitions
#define REFS1 7
#define REFS0 6
#define ADLAR 5
#define MUX3 3
#define MUX2 2
#define MUX1 1
#define MUX0 0
#define PRADC 0
#define TOIE1 0
#define CS10 0
#define CS11 1
#define CS12 2

#define cli() (SREGI=0)
#define sei() (SREGI=1)

// uint16_t hardware timer
#define TIMER_SETUP() TCCR1A = 0 /* Timer mode */; TIMSK1 = (0 << TOIE1) /* disable interrupts */
#define TIMER_VALUE() TCNT1
#define TIMER_CLEAR()	TCNT1 = 0
#define TIMER_ENABLE(enable) \
    if (enable) {\
        TCCR1B = 1 << CS12 | 0 << CS11 | 1 << CS10; /* Timer prescaler div1024 (15625Hz) */\
    } else {\
        TCCR1B = 0;	/* stop clock */\
    }

namespace mockduino { // fireduino implementation
	int16_t	digitalRead(int16_t pin);
	void digitalWrite(int16_t dirPin, int16_t value);
	int16_t analogRead(int16_t pin);
	void analogWrite(int16_t pin, int16_t value);
	void pinMode(int16_t pin, int16_t inout);
	void delay(int ms);
	void delayMicroseconds(uint16_t usDelay);
	uint8_t eeprom_read_byte(uint8_t *addr);
	void eeprom_write_byte(uint8_t *addr, uint8_t value);
	Print& get_Print();
	int16_t serial_read();
	int16_t serial_available();
	void serial_begin(int32_t baud);
	void serial_print(const char *value);
	void serial_print(const char value);
	void serial_print(int16_t value, int16_t format = DEC);
}

typedef class MockSerial : public Print {
	private:
		std::string serialout;
		std::string serialline;

	public:
		void clear();
		void push(uint8_t value);
		void push(int16_t value);
		void push(int32_t value);
		void push(float value);
		void push(std::string value);
		void push(const char * value);
		std::string output();

	public:
		int available();
		void begin(long speed) ;
		uint8_t read() ;
		virtual size_t write(uint8_t value);
		void print(const char value);
		void print(const char *value);
		void print(int value, int format = DEC);
} MockSerial;
extern MockSerial mockSerial;

#define MOCKDUINO_PINS 127
#define MOCKDUINO_MEM 1024

typedef class MockDuino {
	friend void mockduino::delayMicroseconds(uint16_t us);
	friend void mockduino::digitalWrite(int16_t pin, int16_t value);
	friend void mockduino::analogWrite(int16_t pin, int16_t value);
	friend int16_t mockduino::digitalRead(int16_t pin);
	friend int16_t mockduino::analogRead(int16_t pin);
	friend void mockduino::pinMode(int16_t pin, int16_t inout);
	private: 
		int16_t pin[MOCKDUINO_PINS];
        int16_t _pinMode[MOCKDUINO_PINS];
		int32_t pinPulses[MOCKDUINO_PINS];
        int16_t mem[MOCKDUINO_MEM];
		int32_t usDelay;
    public:

    public:
        MockDuino();
		void dump();
		int16_t& MEM(int addr);
		void clear();
		void timer64us(int increment=1);
		void delay500ns();
		int16_t getPinMode(int16_t pin);
		int16_t getPin(int16_t pin);
		void setPin(int16_t pin, int16_t value);
		void setPinMode(int16_t pin, int16_t value);
		int32_t pulses(int16_t pin);
		uint32_t get_usDelay() {return usDelay;}
} MockDuino;
#define DELAY500NS arduino.delay500ns();
extern MockDuino arduino;

//extern MockDuino arduino;

#define A0 54
#define A1 (A0+1)
#define A2 (A1+1)
#define A3 (A2+1)
#define A4 (A3+1)
#define A5 (A4+1)
#define A6 (A5+1)

namespace fireduino {
	inline Print& get_Print() {
		return mockduino::get_Print();
	}
	inline int16_t serial_read() {
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
	inline void analogWrite(int16_t pin, int16_t value) {
		mockduino::analogWrite(pin, value);
	}
	inline int16_t analogRead(int16_t pin) {
		return mockduino::analogRead(pin);
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
	inline uint32_t millis() {
		extern uint32_t get_timer64us();
		return get_timer64us()/64;
	}
	inline void delay_stepper_pulse() {
		delayMicroseconds(2);
	}
	inline uint32_t get_timer64us() {
		return (* (volatile uint16_t *) &TCNT1);
	}
	inline void setup_timer64us() {
		TIMER_SETUP();
	}
	inline void clear_timer64us() {
		TIMER_CLEAR();
	}
	inline void enable_timer64us(bool enable) {
		TIMER_ENABLE(enable);
	}
	inline int16_t freeRam () {
		return 1000;
	}
} // namespace fireduino

std::string eeprom_read_string(uint8_t *addr);

#endif

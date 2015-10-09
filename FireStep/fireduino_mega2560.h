#ifndef FIREDUINO_MEGA2560_H
#define FIREDUINO_MEGA2560_H

#include "Arduino.h"

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

#ifndef DELAY500NS
#define DELAY500NS \
  asm("nop");asm("nop");asm("nop");asm("nop"); asm("nop");asm("nop");asm("nop");asm("nop");
#endif

// A4983 stepper driver pulse cycle requires 2 microseconds
// DRV8825 stepper driver requires 3.8 microseconds
//   http://www.ti.com/lit/ds/symlink/drv8825.pdf
#define PULSE_WIDTH_DELAY() /* no delay */

#define A4988_PULSE_DELAY 	DELAY500NS;DELAY500NS
#define DRV8825_PULSE_DELAY DELAY500NS;DELAY500NS;DELAY500NS;DELAY500NS
#define STEPPER_PULSE_DELAY DRV8825_PULSE_DELAY

namespace fireduino {
	inline Ticks ticks() {
		return 
	}
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
		/**
		 * This rewrite of the Arduino Mega digitalWrite() method
		 * is over twice as fast as the original, which greatly
		 * improves stepper smoothness, especially when all four
		 * motors are active.
		 */
		//uint8_t timer = digitalPinToTimer(pin);
		uint8_t bit = digitalPinToBitMask(pin);
		uint8_t port = digitalPinToPort(pin);
		volatile uint8_t *out;

		//if (port == NOT_A_PIN) return;

		// If the pin that support PWM output, we need to turn it off
		// before doing a digital write.
		//if (timer != NOT_ON_TIMER) turnOffPWM(timer);

		out = portOutputRegister(port);

		uint8_t oldSREG = SREG;
		cli();

		//if (val == LOW) {
		//*out &= ~bit;
		//} else {
		//*out |= bit;
		//}
		*out |= bit;
		STEPPER_PULSE_DELAY;
		*out &= ~bit;

		SREG = oldSREG;
	}
	inline void delay(int ms) {
		::delay(ms);
	}
	inline void delayMicroseconds(uint16_t usDelay) {
		::delayMicroseconds(usDelay);
	}
	inline uint8_t eeprom_read_byte(uint8_t *addr) {
		return ::eeprom_read_byte(addr);
	}
	inline void	eeprom_write_byte(uint8_t *addr, uint8_t value) {
		::eeprom_write_byte(addr, value);
	}
	inline void delay_stepper_pulse() {
		// Mega is so slow that no delay is needed
	}
	inline uint16_t get_timer1() {
		return (* (volatile uint16_t *) &TCNT1);
	}
	inline void timer1_enable(bool enable) {
		TIMER_ENABLE(enable);
	}
	inline void setup_timer1() {
		TIMER_SETUP();
	}
	inline void clear_timer1() {
		TIMER_CLEAR();
	}
} // namespace fireduino

#endif

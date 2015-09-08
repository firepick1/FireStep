#ifndef MEGA2560_H
#define MEGA2560_H

/**
 * WARNING:
 * This file should only be included for use on Arduino Mega2560.
 */

#include "Arduino.h"
#include "IDuino.h"
#include "Thread.h"

#ifdef EEPROM_SIZE
#undef EEPROM_SIZE
#endif
#define EEPROM_SIZE 4096

#define CLOCK_HZ 16000000L	// cycles per second
#define TIMER_PRESCALE	1024 /* 1, 8, 64, 256, 1024 */
#define FREQ_CYCLES(freq) (CLOCK_HZ/(freq))
#define MS_CYCLES(ms) FREQ_CYCLES(1000.0 / (ms))
#define MS_TICKS_REAL(ms) (FREQ_CYCLES(1000.0 / (ms))/TIMER_PRESCALE)
#define MS_TICKS(ms) ((int32_t) MS_TICKS_REAL(ms))
#define MAX_GENERATIONS 50010
#define GENERATION_RESET 50000
#define TICK_MICROSECONDS ((TIMER_PRESCALE * 1000L)/(CLOCK_HZ/1000))
#define TICKS_PER_SECOND ((int32_t)MS_TICKS(1000))

// uint16_t hardware timer
#define TIMER_CLEAR() TCNT1 = 0
#define TIMER_SETUP() TCCR1A = 0 /* Timer mode */; TIMSK1 = (0 << TOIE1) /* disable interrupts */
#define TIMER_VALUE() TCNT1

namespace firestep {

/**
 * FireStep hardware implementation for Arduino Mega2560
 */
typedef class Mega2560 : public IDuino {
public: // ArduinoJson Print
    virtual size_t write(uint8_t value) {
    }
public: // Serial
    virtual void serial_begin(long baud) {
        Serial.begin(baud);
    }
    virtual int serial_available() {
        return Serial.available();
    }
    virtual byte serial_read() {
        return Serial.read();
    }
    virtual inline void serial_print(const char value) {
        Serial.print(value);
    }
    virtual inline void serial_print(const char *value) {
        Serial.print(value);
    }
    virtual inline void serial_print(int value, int format = DEC) {
        Serial.print(value, format);
    }

public: // Pins
    virtual inline void analogWrite(int16_t dirPin, int16_t value) {
        ::analogWrite(dirPin, value);
    }
    virtual inline int16_t analogRead(int16_t dirPin) {
        return ::analogRead(dirPin);
    }
    virtual inline void digitalWrite(int16_t dirPin, int16_t value) {
        ::digitalWrite(dirPin, value);
    }
    virtual inline void delayMicroseconds(uint16_t usDelay) {
        ::delayMicroseconds(usDelay);
    }
    virtual inline int16_t digitalRead(int16_t dirPin) {
        return ::digitalRead(dirPin);
    }
    virtual inline void pinMode(int16_t pin, int16_t inout) {
        ::pinMode(pin, inout);
    }

public: // misc
    virtual inline void delay(int ms) {
        ::delay(ms);
    }
    virtual inline void timer_enable(bool enable) {
        if (enable) {
            TCCR1B = 1 << CS12 | 0 << CS11 | 1 << CS10; /* Timer prescaler div1024 (15625Hz) */
        } else {
            TCCR1B = 0;	/* stop clock */
        }
    }
    virtual inline bool timer_enabled() {
        return (TCCR1B & (1<<CS12 || 1<<CS11 || 1<<CS10)) ? true : false;
    }

public: // EEPROM
    virtual inline uint8_t eeprom_read_byte(uint8_t *addr) {
#ifdef Arduino_H
        return ::eeprom_read_byte(addr);
#else
		return 0;
#endif
    }
    virtual inline void eeprom_write_byte(uint8_t *addr, uint8_t value) {
#ifdef Arduino_H
		::eeprom_write_byte(addr, value);
#endif
    }

    virtual inline string eeprom_read_string(uint8_t *addr) {
#ifdef Arduino_H
        return ::eeprom_read_string(addr);
#else
		return "(Not Mega2560)";
#endif
    }

public: // PROGMEM
	virtual inline int PM_strcmp(const char *a, const char *b) {
		return -strcmp_P(b, a);
	}
	virtual inline char * PM_strcpy(char *dst, const char *src) {
		return strcpy_P(dst, src);
	}
	virtual inline size_t PM_strlen(const char *src) {
		return strlen_P(src);
	}
	virtual inline byte PM_read_byte_near(const void *src) {
		return pgm_read_byte_near(src);
	}

public: // FireStep
    virtual inline void pulseFast(uint8_t pin) {
#ifdef Arduino_H
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
#endif
    }
    virtual Ticks ticks() {
		return firestep::ticks();
    }
} Mega2560;

extern Mega2560 mega2560;

} // namespace firestep

#endif

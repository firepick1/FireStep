#ifndef MEGA2560_H
#define MEGA2560_H

/**
 * WARNING:
 * This file should only be included for use on Arduino Mega2560.
 */

#include "Arduino.h"
#include "IDuino.h"

#define EEPROM_BYTES 4096


namespace firestep {

/**
 * FireStep hardware implementation for Arduino Mega2560
 */
typedef class Mega2560 : public IDuino {
public: // Serial
    virtual inline void print(const char value) {
        print(value);
    }
    virtual inline void print(const char *value) {
        print(value);
    }
    virtual inline void print(int value, int format = DEC) {
        print(value, format);
    }
    virtual inline void println(const char value, int format = DEC) {
        println(value, format);
    }

public: // Pins
    virtual inline void analogWrite(int16_t dirPin, int16_t value) {
        analogWrite(dirPin, value);
    }
    virtual inline int16_t analogRead(int16_t dirPin) {
        return analogRead(dirPin);
    }
    virtual inline void digitalWrite(int16_t dirPin, int16_t value) {
        digitalWrite(dirPin, value);
    }
    virtual inline void delayMicroseconds(uint16_t usDelay) {
        delayMicroseconds(usDelay);
    }
    virtual inline int16_t digitalRead(int16_t dirPin) {
        return digitalRead(dirPin);
    }
    virtual inline void pinMode(int16_t pin, int16_t inout) {
        pinMode(pin, inout);
    }

public: // misc
    virtual inline void delay(int ms) {
        delay(ms);
    }

public: // EEPROM
    virtual inline uint8_t eeprom_read_byte(uint8_t *addr) {
        return eeprom_read_byte(addr);
    }
    virtual inline void eeprom_write_byte(uint8_t *addr, uint8_t value) {
        eeprom_write_byte(addr, value);
    }

    virtual inline string eeprom_read_string(uint8_t *addr) {
        return eeprom_read_string(addr);
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
} Mega2560;

extern Mega2560 mega2560;

} // namespace firestep

#endif

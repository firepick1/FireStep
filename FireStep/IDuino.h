#ifndef IDUINO_H
#define IDUINO_H

#include <Arduino.h>
#include <ArduinoJson.h>

#ifdef Arduino_h
// Use Arduino definitions
#else
#include <iostream>
#include <sstream>
#include <iomanip>
#include <stdint.h>
#include <cmath>
#include <cstring>

typedef uint8_t byte;
typedef uint8_t boolean;

#define NOVALUE 32767 /* 0x77FF */
#define NOVALUESTR "32767"

#define PROGMEM

#define DEC 1
#define BYTE 0
#define HEX 2
#define HIGH 1
#define LOW 0
#define INPUT 0
#define OUTPUT 1

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

extern "C" {
    extern unsigned long millis();
}

#define ARDUINO_PINS 127
#define ARDUINO_MEM 1024

#define A0 54
#define A1 (A0+1)
#define A2 (A1+1)
#define A3 (A2+1)
#define A4 (A3+1)
#define A5 (A4+1)
#define A6 (A5+1)
#define A7 (A6+1)
#define A8 (A7+1)
#define A9 (A8+1)
#define A10 (A9+1)
#define A11 (A10+1)
#define A12 (A11+1)
#define A13 (A12+1)
#define A14 (A13+1)
#define A15 (A14+1)

#endif

#ifdef Arduino_h
#else
#include <string.h>
#endif

/* IDuino implementations may undef and re-define LARGER values. These are minimums: */
#define EEPROM_CMD_BYTES	512		/* JsonController command buffer for EEPROM interaction */
#define EEPROM_SIZE			4096	/* Available EEPROM memory size */

/**
 * Each IDuino tick is 64 microseconds. This timescale is chosen
 * to match the Arduino's original 16MHz clock cycle with a 1024
 * timer prescale. A tick is well-suited to real-time CNC control
 * and provides finer granularity than a millisecond, which facilitates
 * integera acceleration calculations. For human timescales, a tick is a
 * bit awkward, and you may find the following defines helpful.
 * IDuino implementations should not have to change tick timescale.
 */
#define CLOCK_HZ 16000000L	// cycles per second
#define TIMER_PRESCALE	1024 /* 1, 8, 64, 256, 1024 */
#define FREQ_CYCLES(freq) (CLOCK_HZ/(freq))
#define MS_CYCLES(ms) FREQ_CYCLES(1000.0 / (ms))
#define MS_TICKS_REAL(ms) (FREQ_CYCLES(1000.0 / (ms))/TIMER_PRESCALE)
#define MS_TICKS(ms) ((int32_t) MS_TICKS_REAL(ms))
#define TICK_MICROSECONDS ((TIMER_PRESCALE * 1000L)/(CLOCK_HZ/1000))
#define TICKS_PER_SECOND ((int32_t)MS_TICKS(1000))

/**
 * The following definitions match the Arduino
 * and avoid contention with stdlib
 */
#ifndef HIGH
#define HIGH 1
#define LOW 0
#define DEC 1
#define BYTE 0
#define HEX 2
#define INPUT 0
#define OUTPUT 1
#endif
#ifndef PI
#define PI 3.1415926535897932384626433832795
#define HALF_PI 1.5707963267948966192313216916398
#define TWO_PI 6.283185307179586476925286766559
#define DEG_TO_RAD 0.017453292519943295769236907684886
#define RAD_TO_DEG 57.295779513082320876798154814105
#define EULER 2.718281828459045235360287471352
#endif
#define minval(a,b) ((a)<(b)?(a):(b))
#define maxval(a,b) ((a)>(b)?(a):(b))
#define absval(x) ((x)>0?(x):-(x))
#define roundval(x)     ((x)>=0?(long)((x)+0.5):(long)((x)-0.5))
#ifndef radians
#define radians(deg) ((deg)*DEG_TO_RAD)
#define degrees(rad) ((rad)*RAD_TO_DEG)
#endif

#define NOPIN 255 /* TODO: Change to NOT_A_PIN */

#ifdef Arduino_h
#define TESTCOUT1(k,v)
#define TESTCOUT2(k1,v1,k2,v2)
#define TESTCOUT3(k1,v1,k2,v2,k3,v3)
#define TESTCOUT4(k1,v1,k2,v2,k3,v3,k4,v4)
#define TESTDECL(t,v)
#define DIE()
#else
#define TESTCOUT1(k,v) cout << k << v << endl
#define TESTCOUT2(k1,v1,k2,v2) cout << k1<<v1 <<k2<<v2 << endl
#define TESTCOUT3(k1,v1,k2,v2,k3,v3) cout << k1<<v1 <<k2<<v2 <<k3<< v3 << endl
#define TESTCOUT4(k1,v1,k2,v2,k3,v3,k4,v4) cout << k1<<v1 <<k2<<v2 <<k3<<v3 <<k4<<v4 << endl
#define TESTDECL(t,v) t v
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#define DIE() kill(getpid(), SIGABRT)
#endif

namespace firestep {

typedef int32_t Ticks;

/**
 * Abstract hardware interface for FireStep usage of Arduino
 */
typedef class IDuino : public Print {
public: // ArduinoJson Print
    virtual size_t write(uint8_t value) = 0;

public: // Serial
    virtual byte serial_read() = 0;
    virtual int serial_available() = 0;
    virtual void serial_begin(long baud) = 0;
    virtual void serial_print(const char *value) = 0;
    virtual void serial_print(const char value) = 0;
    virtual void serial_print(int value, int format = DEC) = 0;

    virtual void serial_println(const char value) {
        serial_print(value);
        serial_print('\n');
    }
    void serial_println(const char *value="") {
        if (*value) {
            serial_print(value);
        }
        serial_print('\n');
    }
    virtual void serial_println(int value, int format = DEC)  {
        serial_print(value, format);
        serial_print('\n');
    }

public: // Pins
    virtual void analogWrite(int16_t dirPin, int16_t value) = 0;
    virtual int16_t analogRead(int16_t dirPin) = 0;
    virtual void pinMode(int16_t pin, int16_t inout) = 0;

public: // Pulse generation
	/**
	 * IMPORTANT!!!
	 * The digitalWrite/digitalRead methods match the Arduino
	 * with one critical difference. They must take at least
	 * 1 microsecond to complete. This constraint ensures that
	 * pulse generation will generate the 2 microsecond pulse
	 * required by DRV8825. When implementing IDuino for fast
	 * CPUs, take care to observe this limitation.
	 */
    virtual void digitalWrite(int16_t dirPin, int16_t value) = 0;
    virtual int16_t digitalRead(int16_t dirPin) = 0;
    virtual void pulseFast(uint8_t pin)  {
        digitalWrite(pin, HIGH);
        /**
         * DRV8825 requires a 2 microsecond delay between
         * leading and trailing edges. Normal Arduinos typically
         * execute digitalWrite LONGER than 2us.
         * This method should therefore be rewritten for implementations
		 * that cannot guarantee fast digitalWrite/digitalRead.
         */
        digitalWrite(pin, LOW);
    }

public: // timing
	virtual uint32_t millis() { return (uint32_t)(ticks(true) / MS_TICKS_REAL(1)); }
    virtual void delay(int ms) = 0;
    virtual void delayMicroseconds(uint16_t usDelay) = 0;

public: // EEPROM
    virtual uint8_t eeprom_read_byte(uint8_t *addr) = 0;
    virtual void eeprom_write_byte(uint8_t *addr, uint8_t value) = 0;

public: // PROGMEM
    virtual inline int PM_strcmp(const char *a, const char *b) {
        return -strcmp(b, a);
    }
    virtual inline char * PM_strcpy(char *dst, const char *src) {
        return strcpy(dst, src);
    }
    virtual inline size_t PM_strlen(const char *src) {
        return strlen(src);
    }
    virtual inline byte PM_read_byte_near(const void *src) {
        return * (uint8_t *) src;
    }

public: // FireStep
    virtual Ticks ticks(bool peek=false) = 0; // 1 Tick is 64 microseconds
    virtual void enableTicks(bool enable) = 0;
    virtual bool isTicksEnabled() = 0;
    virtual size_t minFreeRam() {
        // Return minimum amount of free ram of all prior calls.
        // Free RAM is only an issue for Mega2560 with its 8K of SRAM
        // The default method just returns 1000 and need only by
        // overridden by processors with limited SRAM (e.g., Mega2560.h).
        return 1000;
    }

} IDuino, *IDuinoPtr;

} // namespace firestep

#endif

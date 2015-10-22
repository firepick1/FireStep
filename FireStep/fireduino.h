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
#ifndef PI
#define PI 3.1415926535897932384626433832795
#endif

extern void fireduino_timer_handler();

#if defined(MOCK_MEGA2560)
#define NO_MCU
#include "MockDuino.h"
#elif defined( __AVR_ATmega2560__)
#include "fireduino_mega2560.h"
#elif defined(_SAM3XA_)
#include "fireduino_due.h"
#elif defined(__USE_LPCOPEN)
#include "fireduino_lpc17xx_40xx.h"
#else
#define NO_MCU
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
	uint32_t get_timer64us(); // ticks @ 64 microseconds
	void setup_timer64us();
	void clear_timer64us();
	void enable_timer64us(bool enable);
	int16_t freeRam ();
} // namespace fireduino
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


#include "pins.h"

#ifdef TEST
#define TESTCOUT1(k,v) cout << k << v << endl
#define TESTCOUT2(k1,v1,k2,v2) cout << k1<<v1 <<k2<<v2 << endl
#define TESTCOUT3(k1,v1,k2,v2,k3,v3) cout << k1<<v1 <<k2<<v2 <<k3<< v3 << endl
#define TESTCOUT4(k1,v1,k2,v2,k3,v3,k4,v4) cout << k1<<v1 <<k2<<v2 <<k3<<v3 <<k4<<v4 << endl
#define TESTDECL(t,v) t v
#define TESTEXP(e) e
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#define DIE() kill(getpid(), SIGABRT)
#else
#define TESTCOUT1(k,v)
#define TESTCOUT2(k1,v1,k2,v2)
#define TESTCOUT3(k1,v1,k2,v2,k3,v3)
#define TESTCOUT4(k1,v1,k2,v2,k3,v3,k4,v4)
#define TESTDECL(t,v)
#define TESTEXP(e)
#define DIE()
#endif

#define DEBUG_EOL() fireduino::serial_println("");
#define DEBUG_HEX(S,V) fireduino::serial_print(" " S ":");fireduino::serial_print(V,HEX);
#define DEBUG_DEC(S,V) fireduino::serial_print(" " S ":");fireduino::serial_print(V,DEC);

#define CLOCK_HZ 16000000L	// cycles per second
#define TIMER_PRESCALE	1024 /* 1, 8, 64, 256, 1024 */
#define FREQ_CYCLES(freq) (CLOCK_HZ/(freq))
#define MS_CYCLES(ms) FREQ_CYCLES(1000.0 / (ms))
#define MS_TICKS_REAL(ms) (FREQ_CYCLES(1000.0 / (ms))/TIMER_PRESCALE)
#define MS_TICKS(ms) ((int32_t) MS_TICKS_REAL(ms))
#define MAX_GENERATIONS 50010
#define GENERATION_RESET 50000
#define TIMER_ENABLED (TCCR1B & (1<<CS12 || 1<<CS11 || 1<<CS10))
#define TICK_MICROSECONDS ((TIMER_PRESCALE * 1000L)/(CLOCK_HZ/1000))
#define TICKS_PER_SECOND ((int32_t)MS_TICKS(1000))

#endif

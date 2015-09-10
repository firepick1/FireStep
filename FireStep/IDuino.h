#ifndef IDUINO_H
#define IDUINO_H

#include "../ArduinoJson/include/ArduinoJson/Arduino/Print.hpp"

#ifndef Arduino_h
#include <string>
#include <string.h>
#endif

/* IDuino implementations may undef and re-define LARGER values. These are minimums: */
#define EEPROM_CMD_BYTES	512		/* JsonController command buffer for EEPROM interaction */
#define EEPROM_SIZE			4096	/* Available EEPROM memory size */

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

// Arduino compatible definitions that avoid contention with stdlib
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
	virtual void digitalWrite(int16_t dirPin, int16_t value) = 0;
	virtual void delayMicroseconds(uint16_t usDelay) = 0;
	virtual int16_t digitalRead(int16_t dirPin) = 0;
	virtual void pinMode(int16_t pin, int16_t inout) = 0;

public: // misc
	virtual void delay(int ms) = 0;
	virtual void timer_enable(bool enable) = 0;
	virtual bool timer_enabled() = 0;
	virtual size_t freeRam() {
		return 1000; // free RAM is only an issue for Mega2560 with its 8K of SRAM 
	}

public: // EEPROM
	virtual uint8_t eeprom_read_byte(uint8_t *addr) = 0;
	virtual void eeprom_write_byte(uint8_t *addr, uint8_t value) = 0;
	virtual string eeprom_read_string(uint8_t *addr) = 0;

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
	virtual void pulseFast(uint8_t pin)  {
		digitalWrite(pin, HIGH);
		/**
		 * DRV8825 requires a 2 microsecond delay between
		 * leading and trailing edges. Normal Arduinos typically
		 * execute digitalWrite LONGER than 2us.
		 * This method should therefore be rewritten fo individual
		 * processors
		 */
		 digitalWrite(pin, LOW);
	 }
	 virtual Ticks ticks() = 0;
	 
} IDuino, *IDuinoPtr;

} // namespace firestep

#endif

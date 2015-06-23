#ifndef MCU_H
#define MCU_H

///////// Microcontroller definitions ///////////

#include "Arduino.h"
#include "pins.h"

#ifdef TEST
#define TESTCOUT1(k,v) cout << k << v << endl
#define TESTCOUT2(k1,v1,k2,v2) cout << k1<<v1 <<k2<<v2 << endl
#define TESTCOUT3(k1,v1,k2,v2,k3,v3) cout << k1<<v1 <<k2<<v2 <<k3<< v3 << endl
#define TESTCOUT4(k1,v1,k2,v2,k3,v3,k4,v4) cout << k1<<v1 <<k2<<v2 <<k3<<v3 <<k4<<v4 << endl
#define TESTDECL(t,v) t v
#define TESTEXP(e) e
#else
#define TESTCOUT1(k,v)
#define TESTCOUT2(k1,v1,k2,v2)
#define TESTCOUT3(k1,v1,k2,v2,k3,v3)
#define TESTCOUT4(k1,v1,k2,v2,k3,v3,k4,v4)
#define TESTDECL(t,v)
#define TESTEXP(e)
#endif

#define DEBUG_EOL() Serial.println("");
#define DEBUG_HEX(S,V) Serial.print(" " S ":");Serial.print(V,HEX);
#define DEBUG_DEC(S,V) Serial.print(" " S ":");Serial.print(V,DEC);

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

// uint16_t hardware timer
#define TIMER_CLEAR()	TCNT1 = 0
#define TIMER_SETUP() TCCR1A = 0 /* Timer mode */; TIMSK1 = (0 << TOIE1) /* disable interrupts */
#define TIMER_VALUE() TCNT1
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

#ifdef ARDUINO
/**
 * This rewrite of the Arduino Mega digitalWrite() method
 * is over twice as fast as the original, which greatly
 * improves stepper smoothness, especially when all four
 * motors are active.
 */
inline void pulseFast(uint8_t pin) {
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
#else
inline void pulseFast(uint8_t pin) {
    digitalWrite(pin, HIGH);
    // Arduino digital I/O is so slow there is no need for delay
    digitalWrite(pin, LOW);
}
#endif

inline int16_t freeRam () {
#ifdef ARDUINO
    extern int __heap_start, *__brkval;
    int v;
    return (int)(size_t)&v - (__brkval == 0 ? (int)(size_t)&__heap_start : (int)(size_t)__brkval);
#else
    return 1000;
#endif
}

#define EEPROM_BYTES 512 /* Actual capacity will be less because eeprom buffer is part of MAX_JSON */
#define EEPROM_END 4096

#ifdef TEST
extern uint8_t eeprom_read_byte(uint8_t *addr);
extern void eeprom_write_byte(uint8_t *addr, uint8_t value);
#endif

#endif

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

#endif

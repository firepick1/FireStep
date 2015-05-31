#ifndef THREAD_H
#define THREAD_H

#include "Arduino.h"
#include "pins.h"

namespace firestep {

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
#define MAX_ThreadS 32
#define TICK_MICROSECONDS ((TIMER_PRESCALE * 1000L)/(CLOCK_HZ/1000))
#define TICKS_PER_SECOND ((int32_t)MS_TICKS(1000))

typedef int32_t Ticks;

typedef union ThreadClock  {
    Ticks ticks;
    struct {
        uint16_t age;
        uint16_t generation;
    };
    ThreadClock() : ticks(0) {}
} ThreadClock, *ThreadClockPtr;

extern ThreadClock threadClock;

typedef struct Thread {
    public:
        Thread() : tardies(0), id(0), pNext(NULL) {
            nextLoop.ticks = 0;
        }
        virtual void setup();
        virtual void loop() {}

        struct Thread *pNext;

        // Threads should increment the loop as desired.
        // Threads with 0 nextLoop will always run ASAP.
        ThreadClock nextLoop;

        byte tardies;
        char id;
}
Thread, *ThreadPtr;

// Binary pulse with variable width
typedef struct PulseThread : Thread {
        virtual void setup(Ticks period, Ticks pulseWidth);
        virtual void loop();

        boolean isHigh;

    protected:
        Ticks m_LowPeriod;
        Ticks m_HighPeriod;
} PulseThread, *PulseThreadPtr;

#define LED_RED 3
#define LED_YELLOW 2
#define LED_GREEN 1
#define LED_NONE 0

typedef class MonitorThread : PulseThread {
        friend class ThreadRunner;
        friend class MachineThread;

    private:
        byte	blinkLED;

        void 	LED(byte value);

        void setup(int pinLED=NOPIN); /* PRIVATE */
        unsigned int Free(); /* PRIVATE */
        void loop(); /* PRIVATE */
        int pinLED; /* PRIVATE */

    public:
        boolean verbose;
    public:
        void Error(const char *msg, int value); /* PRIVATE */
} MonitorThread;

void Error(const char *msg, int value);
void ThreadEnable(boolean enable);
void PrintN(char c, byte n);

extern MonitorThread monitor;

extern struct Thread *pThreadList;
extern int nThreads;
extern int32_t nLoops;
extern int32_t nTardies;

typedef class ThreadRunner {
    private:
        uint16_t 	generation;
        uint16_t 	lastAge;
        uint16_t 	age;
        byte		testTardies;
        int16_t		nHB;
        byte		fast;
    public:
        ThreadRunner();
        void resetGenerations();
		void clear();
        void setup(int pinLED=NOPIN);

    public:
        void run() {
            // outer loop: bookkeeping
            for (;;) {
                outerLoop();
            }
        }
    public:
        inline uint16_t get_generation() {
            return generation;
        }
    public:
        inline uint16_t get_lastAge() {
            return lastAge;
        }
    public:
        inline uint16_t get_age() {
            return age;
        }
    public:
        inline byte get_testTardies() {
            return testTardies;
        }
    public:
        inline void outerLoop() {
            if (fast-- && innerLoop()) {
                // do nothing
            } else {
                nLoops += nHB;
                nHB = 0;
                fast = 255;
            }
        }
    public:
		inline Ticks ticks() {
            cli();
            threadClock.age = age = TCNT1;
            if (age < lastAge) {
                // 1) a generation is 4.194304s 
				// 2) generation is incremented when TCNT1 overflows
                // 3) innerLoop MUST complete within a generation
                lastAge = age;
                threadClock.generation = ++generation;
                if (generation > MAX_GENERATIONS) {
                    // TODO test this
                    // generation overflow every ~58 hours
                    resetGenerations();
                    //const char *msg ="GOVFL";
                    //Serial.println(msg);
                    //throw msg;
                }
                sei();
                return 0;
            }
            lastAge = age;
            sei();
			return threadClock.ticks;
		}
        inline byte innerLoop() {
			if (ticks() == 0) {
				return 0;
			}

            // inner loop: run active Threads
            for (ThreadPtr pThread = pThreadList; pThread; pThread = pThread->pNext) {
                if (generation < pThread->nextLoop.generation) {
                    continue; // not time yet for scheduled reactivation
                }
                if (generation == pThread->nextLoop.generation && age < pThread->nextLoop.age) {
                    continue; // not time yet for scheduled reactivation
                }

                pThread->loop();	// reactivate thread
                nHB++;

                if (testTardies-- == 0) {
                    testTardies = 5;	// test intermittently for late Threads
                    if (age <= pThread->nextLoop.age) {
                        continue;    // transient ASAP or future
                    }
                    if (generation < pThread->nextLoop.generation) {
                        continue;    // future
                    }
                    if (0 == pThread->nextLoop.age) {
                        continue;    // permanent ASAP
                    }

                    pThread->tardies++;	// thread-specific tardy count
                    nTardies++;			// global tardy count
                }
            }
            return 1;
        }
} ThreadRunner;
extern ThreadRunner threadRunner;

/**
 * With the standard ATMEGA 16,000,000 Hz system clock and TCNT1 / 1024 prescaler:
 * 1 tick = 1024 clock cycles = 64 microseconds
 * Clock overflows in 2^31 * 0.000064 seconds = ~38.1 hours
 */
extern Ticks ticks();

} // namespace firestep

#endif

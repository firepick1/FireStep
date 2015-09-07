#ifndef THREAD_H
#define THREAD_H

#include "MCU.h"
#include "IDuino.h"
//#include "Mega2560.h"

namespace firestep {

extern int16_t leastFreeRam;

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

typedef class MonitorThread : PulseThread {
    friend class ThreadRunner;
    friend class MachineThread;

private:
    byte	blinkLED;
    int16_t pinLED; /* PRIVATE */

private:
    void 	LED(byte value);
    void setup(int pinLED = NOPIN); /* PRIVATE */
    unsigned int Free(); /* PRIVATE */
    void loop(); /* PRIVATE */

public:
    boolean verbose;
public:
    void Error(const char *msg, int value); /* PRIVATE */
} MonitorThread;

void Error(const char *msg, int value);
void ThreadEnable(boolean enable);

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
    void setup(int pinLED = NOPIN);

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
        threadClock.age = age = TIMER_VALUE();
        if (age < lastAge) {
            // 1) a generation is 4.194304s
            // 2) generation is incremented when TIMER_VALUE() overflows
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

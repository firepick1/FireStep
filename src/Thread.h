#ifndef THREAD_H
#define THREAD_H

#include "Arduino.h"

namespace firestep {

#define DEBUG_EOL() Serial.println("");
#define DEBUG_HEX(S,V) Serial.print(" " S ":");Serial.print(V,HEX);
#define DEBUG_DEC(S,V) Serial.print(" " S ":");Serial.print(V,DEC);

//extern byte lastByte;		// declare this at end of program
#define CLOCK_HZ 16000000	// cycles per second
#define TIMER_PRESCALE	1024 /* 1, 8, 64, 256, 1024 */
#define FREQ_CYCLES(freq) (CLOCK_HZ/(freq))
#define MS_CYCLES(ms) FREQ_CYCLES(1000.0 / (ms))
#define MS_TIMER_CYCLES(ms) (FREQ_CYCLES(1000.0 / (ms))/TIMER_PRESCALE)
#define GENERATION_CYCLES (CLOCK_HZ / 65536)
#define MAX_GENERATIONS 50010
#define GENERATION_RESET 50000
#define TIMER_ENABLED (TCCR1B & (1<<CS12 || 1<<CS11 || 1<<CS10))

typedef uint32_t TICKS;
typedef int32_t PERIOD;

typedef union ThreadClock  {
    TICKS clock;
    struct {
        uint16_t age;
        uint16_t generation;
    };
    ThreadClock() : clock(0) {}
} ThreadClock, *ThreadClockPtr;

extern ThreadClock masterClock;

#define MAX_ThreadS 32

typedef struct Thread {
    public:
        Thread() : tardies(0), id(0), pNext(NULL) {
            nextHeartbeat.clock = 0;
        }
        virtual void setup();
        virtual void Heartbeat() {}

        struct Thread *pNext;

        // Threads should increment the heartbeat as desired.
        // Threads with 0 nextHeartbeat will always run ASAP.
        ThreadClock nextHeartbeat;

        byte tardies;
        char id;
}
Thread, *ThreadPtr;

// Binary pulse with variable width
typedef struct PulseThread : Thread {
        virtual void setup(TICKS period, TICKS pulseWidth);
        virtual void Heartbeat();

        boolean isHigh;

    protected:
        TICKS m_LowPeriod;
        TICKS m_HighPeriod;
} PulseThread, *PulseThreadPtr;

#define LED_RED 3
#define LED_YELLOW 2
#define LED_GREEN 1
#define LED_NONE 0
#define LED_PIN_RED 13
#define LED_PIN_GRN 12

typedef class MonitorThread : PulseThread {
        friend class ThreadRunner;
		friend class MachineThread;

    private:
        byte	blinkLED;

        void 	LED(byte value);

        void setup(int pin1, int pin2); /* PRIVATE */
        unsigned int Free(); /* PRIVATE */
        void Heartbeat(); /* PRIVATE */
        int m_Pin1; /* PRIVATE */
        int m_Pin2; /* PRIVATE */

	public:
        boolean verbose;
	public:
        void Error(const char *msg, int value); /* PRIVATE */
} MonitorThread;

void Error(const char *msg, int value);
void ThreadEnable(boolean enable);
void PrintN(char c, byte n);
int32_t MicrosecondsSince(TICKS lastClock);

extern MonitorThread monitor;

extern struct Thread *pThreadList;
extern ThreadClock masterClock;
extern int nThreads;
extern long nHeartbeats;
extern long nTardies;

typedef class ThreadRunner {
    private:
        uint16_t 	generation;
        uint16_t 	lastAge;
        uint16_t 	age;
        byte		testTardies;
        int16_t		nHB;
        byte		fast;
    protected:
        void resetGenerations();
    public:
        ThreadRunner();
    public:
        void setup(int monitorPin1, int monitorPin2);
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
                nHeartbeats += nHB;
                nHB = 0;
                fast = 255;
            }
        }
    public:
        inline byte innerLoop() {
            cli();
            //cout << "innerloopA:" << masterClock.clock << endl;
            masterClock.age = age = TCNT1;
            //cout << "innerloopB:" << masterClock.clock << endl;
            if (age < lastAge) {
                // a generation is 4.194304s and is incremented when TCNT1 overflows
                // Innerloop MUST complete within a generation
                lastAge = age;
                masterClock.generation = ++generation;
                //cout << "innerloopC:" << masterClock.clock << endl;
                if (generation > MAX_GENERATIONS) {
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

            // inner loop: run active Threads
            for (ThreadPtr pThread = pThreadList; pThread; pThread = pThread->pNext) {
                if (generation < pThread->nextHeartbeat.generation) {
                    continue; // not time yet for scheduled reactivation
                }
                if (generation == pThread->nextHeartbeat.generation && age < pThread->nextHeartbeat.age) {
                    continue; // not time yet for scheduled reactivation
                }

                pThread->Heartbeat();	// reactivate thread
                nHB++;

                if (testTardies-- == 0) {
                    testTardies = 5;	// test intermittently for late Threads
                    if (age <= pThread->nextHeartbeat.age) {
                        continue;    // transient ASAP or future
                    }
                    if (generation < pThread->nextHeartbeat.generation) {
                        continue;    // future
                    }
                    if (0 == pThread->nextHeartbeat.age) {
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

} // namespace firestep

#endif

#include "Arduino.h"
#include "Thread.h"

using namespace firestep;

#define MAX_THREADS 32

namespace firestep {
ThreadClock 	threadClock;
ThreadRunner 	threadRunner;
struct Thread *	pThreadList;
int 			nThreads;
int32_t 		nLoops;
int32_t 		nTardies;
int16_t			leastFreeRam = 32767;
};


void Thread::setup() {
    bool active = false;
    for (ThreadPtr pThread = pThreadList; pThread; pThread = pThread->pNext) {
        if (pThread == this) {
            active = true;
            break;
        }
    }

    if (!active) {
        pNext = pThreadList;
        pThreadList = this;
        if (id == 0) {
            id = 'a' + nThreads;
        }
        nThreads++;
        if (nThreads >= MAX_THREADS) {
            Error("SC", MAX_THREADS);
        }
    }
}

void PulseThread::setup(Ticks period, Ticks pulseWidth) {
    Thread::setup();
    if (pulseWidth == 0) {
        m_HighPeriod = period / 2;
    } else {
        m_HighPeriod = pulseWidth;
    }
    m_LowPeriod = period - m_HighPeriod;
}

void PulseThread::loop() {
    isHigh = !isHigh;
    if (isHigh) {
        nextLoop.ticks = threadClock.ticks + m_HighPeriod;
    } else {
        nextLoop.ticks = threadClock.ticks + m_LowPeriod;
    }
}

void MonitorThread::setup(int pinLED) {
    id = 'Z';
    // set monitor interval to not coincide with timer overflow
    PulseThread::setup(MS_TICKS(1000), MS_TICKS(250));
    this->pinLED = pinLED;
    verbose = false;
    if (pinLED != NOPIN) {
        fireduino::pinMode(pinLED, OUTPUT);
    }
    blinkLED = true;
}

void MonitorThread::LED(byte value) {
    if (pinLED != NOPIN) {
        fireduino::digitalWrite(pinLED, value ? HIGH : LOW);
    }
}

void MonitorThread::Error(const char *msg, int value) {
    LED(HIGH);
    for (int i = 0; i < 20; i++) {
        fireduino::serial_print('>');
    }
    fireduino::serial_print(msg);
    fireduino::serial_println(value);
}

void MonitorThread::loop() {
    PulseThread::loop();
#define MONITOR
#ifdef MONITOR
    ThreadEnable(false);
    if (blinkLED) {
        if (isHigh) {
            LED(blinkLED);
        } else {
            LED(LOW);
        }
    }
    if (nTardies > 50) {
        Error("T", nTardies);
        for (ThreadPtr pThread = pThreadList; pThread; pThread = pThread->pNext) {
            fireduino::serial_print(pThread->id);
            fireduino::serial_print(":");
            fireduino::serial_print(pThread->tardies, DEC);
            pThread->tardies = 0;
            fireduino::serial_print(" ");
        }
        fireduino::serial_println('!');
    } else if (nTardies > 20) {
        LED(HIGH);
        verbose = true;
    }
    for (ThreadPtr pThread = pThreadList; pThread; pThread = pThread->pNext) {
        if (threadClock.generation > pThread->nextLoop.generation + 1 &&
                pThread->nextLoop.generation > 0) {
            //cout << "ticks:" << threadClock.ticks
            //<< " nextLoop:" << pThread->nextLoop.ticks
            //<< " pThread:" << pThread->id << endl;
            Error("O@G", pThread->nextLoop.generation);
        }
    }
    if (isHigh) {
        if (verbose) {
            fireduino::serial_print(".");
        }
        nTardies = 0;
    }
    ThreadEnable(true);
#endif
}

MonitorThread firestep::monitor;

void firestep::Error(const char *msg, int value) {
    monitor.Error(msg, value);
}

ThreadRunner::ThreadRunner() {
    clear();
}

void ThreadRunner::clear() {
    pThreadList = NULL;
    threadClock.ticks = 0;
    nThreads = 0;
    nLoops = 0;
    nTardies = 0;
    generation = threadClock.generation;
    lastAge = 0;
    age = 0;
    nHB = 0;
    testTardies = 0;
    fast = 255;
    TIMER_CLEAR();
}

void ThreadRunner::setup(int pinLED) {
    monitor.setup(pinLED);

    TIMER_SETUP();
    lastAge = 0;
    ThreadEnable(true);
}

/**
 * The generation count has exceeded the maximum.
 * Give the machine a rest and power-cycle it.
 */
void ThreadRunner::resetGenerations() {
    threadClock.ticks = 0;
    lastAge = 0;
    for (ThreadPtr pThread = pThreadList; pThread; pThread = pThread->pNext) {
        pThread->nextLoop.ticks = 0;
    }
}

void firestep::ThreadEnable(boolean enable) {
#ifdef DEBUG_ThreadENABLE
    for (ThreadPtr pThread = pThreadList; pThread; pThread = pThread->pNext) {
        fireduino::serial_print(pThread->id);
        fireduino::serial_print(":");
        fireduino::serial_print(pThread->nextLoop.ticks, DEC);
        fireduino::serial_print(" ");
    }
#endif
    TIMER_ENABLE(enable);
}

firestep::Ticks firestep::ticks() {
#if defined(TEST)
    arduino.timer1(1);
#endif
    Ticks result = threadRunner.ticks();

    if (result == 0) {
        result = threadRunner.ticks();
    }
    return result;
}

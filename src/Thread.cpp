#include "Arduino.h"
#include "Thread.h"
#include "SerialTypes.h"

ThreadRunner threadRunner;

struct Thread *pThreadList;
ThreadClock masterClock;
int nThreads;
long nHeartbeats;
long nTardies;

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
        if (nThreads >= MAX_ThreadS) {
            Error("SC", MAX_ThreadS);
        }
    }
}

void PulseThread::setup(CLOCK period, CLOCK pulseWidth) {
    Thread::setup();
    if (pulseWidth == 0) {
        m_HighPeriod = period / 2;
    } else {
        m_HighPeriod = pulseWidth;
    }
    m_LowPeriod = period - m_HighPeriod;
}

void PulseThread::Heartbeat() {
    isHigh = !isHigh;
    if (isHigh) {
        nextHeartbeat.clock += m_HighPeriod;
    } else {
        nextHeartbeat.clock += m_LowPeriod;
    }
}


unsigned long totalHeartbeats;

void MonitorThread::setup(int pin1, int pin2) {
    id = 'Z';
    // set monitor interval to not coincide with timer overflow
    PulseThread::setup(MS_TIMER_CYCLES(1000), MS_TIMER_CYCLES(250));
    m_Pin1 = pin1;
    m_Pin2 = pin2;
    verbose = true;
    pinMode(pin1, OUTPUT);
    pinMode(pin2, OUTPUT);
    blinkLED = true;

    for (byte i = 4; i-- > 0;) {
        LED(i);
        delay(500);
        LED(0);
        delay(500);
    }
}

void MonitorThread::LED(byte value) {
    digitalWrite(m_Pin1, (value == 1 || value == 2) ? HIGH : LOW);
    digitalWrite(m_Pin2, (value == 3 || value == 2) ? HIGH : LOW);
}

void MonitorThread::Error(const char *msg, int value) {
    LED(3);
    for (int i = 0; i < 20; i++) {
        Serial.print('>');
    }
    Serial.print(msg);
    Serial.println(value);
}

/**
 * A dangerous way to find out how much memory is consumed
 */
unsigned int MonitorThread::Free() {
    unsigned int result;
    for (byte *pByte = &lastByte; *++pByte == 0x00; pByte++) {
        result++;
    }

    return result;
}

void MonitorThread::Heartbeat() {
    PulseThread::Heartbeat();
#define MONITOR
#ifdef MONITOR
    ThreadEnable(false);
    if (blinkLED) {
        if (isHigh) {
            LED(blinkLED);
        } else {
            LED(LED_NONE);
        }
    }
    if (nTardies > 50) {
        Error("T", nTardies);
        for (ThreadPtr pThread = pThreadList; pThread; pThread = pThread->pNext) {
            Serial.print(pThread->id);
            Serial.print(":");
            Serial.print(pThread->tardies, DEC);
            pThread->tardies = 0;
            Serial.print(" ");
        }
        Serial.println('!');
    } else if (nTardies > 20) {
        LED(LED_YELLOW);
        verbose = true;
    }
    for (ThreadPtr pThread = pThreadList; pThread; pThread = pThread->pNext) {
        if (masterClock.generation > pThread->nextHeartbeat.generation + 1 && 
			pThread->nextHeartbeat.generation > 0) {
            Error("O@G", pThread->nextHeartbeat.generation);
        }
    }
    if (isHigh) {
        totalHeartbeats += nHeartbeats;
        if (verbose) {
            Serial.print(".");
            //DEBUG_DEC("F", Free());
            DEBUG_DEC("S", millis() / 1000);
            DEBUG_DEC("G", masterClock.generation);
            DEBUG_DEC("H", nHeartbeats);
            if (masterClock.generation > 0) {
                DEBUG_DEC("H/G", totalHeartbeats / masterClock.generation);
            }
            DEBUG_DEC("T", nTardies);
            DEBUG_EOL();
        }
        nHeartbeats = 0;
        nTardies = 0;
    }
    ThreadEnable(true);
#endif
}

MonitorThread monitor;

void Error(const char *msg, int value) {
    monitor.Error(msg, value);
}

ThreadRunner::ThreadRunner() {
	pThreadList = NULL;
	masterClock.clock = 0;
	nThreads = 0;
	nHeartbeats = 0;
	nTardies = 0;
	generation = masterClock.generation;
	lastAge = 0;
	age = 0;
	nHB = 0;
	testTardies = 0;
	fast = 255;
}

void ThreadRunner::setup(int monitorPin1, int monitorPin2) {
    monitor.setup(monitorPin1, monitorPin2);
    DEBUG_DEC("CLKPR", CLKPR);
    DEBUG_DEC("nThreads", nThreads);
    DEBUG_EOL();

    TCCR1A = 0; // Timer mode
    TIMSK1 = 0 << TOIE1;	// disable interrupts
    ThreadEnable(true);
}

/**
 * The generation count has exceeded the maximum (~3.5minutes @ 16MHZ).
 */ 
void ThreadRunner::resetGenerations() {
    masterClock.generation -= GENERATION_RESET;
    for (ThreadPtr pThread = pThreadList; pThread; pThread = pThread->pNext) {
        if (pThread->nextHeartbeat.clock) {
            pThread->nextHeartbeat.generation -= GENERATION_RESET;
        }
    }
}

long MicrosecondsSince(CLOCK lastClock) {
    long elapsed;
    if (masterClock.clock < lastClock) {
        ThreadClock reset;
        reset.generation = GENERATION_RESET;
        reset.age = 0;
        elapsed = (masterClock.clock + reset.clock) - lastClock;
        if (elapsed < 0) {
            //SerialInt32 e;
            //e.longValue = elapsed;
            //e.send();
            //Serial.print(' ', BYTE);
            //e.longValue = masterClock.clock;
            //e.send();
            //Serial.print(' ', BYTE);
            //e.longValue = reset.clock;
            //e.send();
            //Serial.print(' ', BYTE);
            //e.longValue = lastClock;
            //e.send();
            Error("e?", 0);
        }
    } else {
        elapsed = masterClock.clock - lastClock;
    }

    return elapsed / FREQ_CYCLES(1000000);
}

// Make sure we come up for air sometimes
#define MAX_ACTIVE_HEARTBEATS (255-MAX_ThreadS)

void ThreadEnable(boolean enable) {
#ifdef DEBUG_ThreadENABLE
    DEBUG_DEC("C", masterClock.clock);
    for (ThreadPtr pThread = pThreadList; pThread; pThread = pThread->pNext) {
        Serial.print(pThread->id);
        Serial.print(":");
        Serial.print(pThread->nextHeartbeat.clock, DEC);
        Serial.print(" ");
    }
    DEBUG_EOL();
#endif
    if (enable) {
        //TCCR1B = 1<<CS12 | 0<<CS11 | 0<<CS10; // Timer prescaler div256
        //TCCR1B = 0 << CS12 | 0 << CS11 | 1 << CS10; // Timer prescaler div1 (16MHz)
        TCCR1B = 1 << CS12 | 0 << CS11 | 1 << CS10; // Timer prescaler div1024 (15625Hz)
    } else {
        TCCR1B = 0;	// Stop clock
    }
}

void PrintN(char c, byte n) {
    while (n--) {
        Serial.print(c);
    }
}


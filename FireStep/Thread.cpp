#include "Arduino.h"
#include "Thread.h"

using namespace firestep;

namespace firestep {
	ThreadClock 	threadClock;
	ThreadRunner 	threadRunner;
	struct Thread *	pThreadList;
	int 			nThreads;
	long 			nHeartbeats;
	long 			nTardies;

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
        if (nThreads >= MAX_ThreadS) {
            Error("SC", MAX_ThreadS);
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

void PulseThread::Heartbeat() {
    isHigh = !isHigh;
    if (isHigh) {
        nextHeartbeat.ticks += m_HighPeriod;
    } else {
        nextHeartbeat.ticks += m_LowPeriod;
    }
}


unsigned long totalHeartbeats;

void MonitorThread::setup(int pin1, int pin2) {
    id = 'Z';
    // set monitor interval to not coincide with timer overflow
    PulseThread::setup(MS_TICKS(1000), MS_TICKS(250));
    m_Pin1 = pin1;
    m_Pin2 = pin2;
    verbose = false;
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

///**
 //* A dangerous way to find out how much memory is consumed
 //*/
//unsigned int MonitorThread::Free() {
    //unsigned int result;
    //for (byte *pByte = &lastByte; *++pByte == 0x00; pByte++) {
        //result++;
    //}
//
    //return result;
//}

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
        if (threadClock.generation > pThread->nextHeartbeat.generation + 1 && 
			pThread->nextHeartbeat.generation > 0) {
			//cout << "ticks:" << threadClock.ticks 
				//<< " nextHeartBeat:" << pThread->nextHeartbeat.ticks 
				//<< " pThread:" << pThread->id << endl;
            Error("O@G", pThread->nextHeartbeat.generation);
        }
    }
    if (isHigh) {
        totalHeartbeats += nHeartbeats;
        if (verbose) {
            Serial.print(".");
            //DEBUG_DEC("F", Free());
            DEBUG_DEC("S", millis() / 1000);
            DEBUG_DEC("G", threadClock.generation);
            DEBUG_DEC("H", nHeartbeats);
            if (threadClock.generation > 0) {
                DEBUG_DEC("H/G", totalHeartbeats / threadClock.generation);
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
	nHeartbeats = 0;
	nTardies = 0;
	generation = threadClock.generation;
	lastAge = 0;
	age = 0;
	nHB = 0;
	testTardies = 0;
	fast = 255;
}

void ThreadRunner::setup(int monitorPin1, int monitorPin2) {
    monitor.setup(monitorPin1, monitorPin2);
    //DEBUG_DEC("CLKPR", CLKPR);
    //DEBUG_DEC("nThreads", nThreads);
    //DEBUG_EOL();

    TCCR1A = 0; // Timer mode
    TIMSK1 = 0 << TOIE1;	// disable interrupts
    ThreadEnable(true);
}

/**
 * The generation count has exceeded the maximum (~3.5minutes @ 16MHZ).
 */ 
void ThreadRunner::resetGenerations() {
    threadClock.generation -= GENERATION_RESET;
    for (ThreadPtr pThread = pThreadList; pThread; pThread = pThread->pNext) {
        if (pThread->nextHeartbeat.ticks) {
            pThread->nextHeartbeat.generation -= GENERATION_RESET;
        }
    }
}

int32_t firestep::MicrosecondsSince(Ticks lastClock) {
    int32_t elapsed;
	//cout << "ticks:" << ticks() << " lastClock:" << lastClock << endl;
    if (ticks() < lastClock) {
        ThreadClock reset;
        reset.generation = GENERATION_RESET;
        reset.age = 0;
        elapsed = (ticks() + reset.ticks) - lastClock;
        if (elapsed < 0) {
            //SerialInt32 e;
            //e.longValue = elapsed;
            //e.send();
            //Serial.print(' ', BYTE);
            //e.longValue = ticks()
            //e.send();
            //Serial.print(' ', BYTE);
            //e.longValue = reset.ticks;
            //e.send();
            //Serial.print(' ', BYTE);
            //e.longValue = lastClock;
            //e.send();
            Error("e?", 0);
        }
//cout << "elapsed:" << elapsed << endl;
    } else {
        elapsed = ticks() - lastClock;
//cout << "ELAPSED:" << elapsed  << endl;
    }

    return (elapsed * 1000000) / MS_TICKS(1000);
}

// Make sure we come up for air sometimes
#define MAX_ACTIVE_HEARTBEATS (255-MAX_ThreadS)

void firestep::ThreadEnable(boolean enable) {
#ifdef DEBUG_ThreadENABLE
    DEBUG_DEC("C", ticks());
    for (ThreadPtr pThread = pThreadList; pThread; pThread = pThread->pNext) {
        Serial.print(pThread->id);
        Serial.print(":");
        Serial.print(pThread->nextHeartbeat.ticks, DEC);
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


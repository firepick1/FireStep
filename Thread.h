#ifndef THREAD_H
#define THREAD_H

#define DEBUG_EOL() Serial.println("");
#define DEBUG_HEX(S,V) Serial.print(" " S ":");Serial.print(V,HEX);
#define DEBUG_DEC(S,V) Serial.print(" " S ":");Serial.print(V,DEC);

extern byte lastByte;		// declare this at end of program
//#define CLOCK_HZ 62500	// cycles per second
#define CLOCK_HZ 16000000	// cycles per second
#define FREQ_CYCLES(freq) (CLOCK_HZ/(freq))
#define MS_CYCLES(ms) FREQ_CYCLES(1000.0 / (ms))
#define GENERATION_CYCLES (CLOCK_HZ / 65536)
#define MIDI_CYCLES(midi) (4 * (CLOCK) Midi4MHz(midi))
		
typedef unsigned long CLOCK;
typedef long PERIOD;

union UIntByte {
	struct {
		unsigned int intValue;
	};
	struct {
		byte lsb;
		byte msb;
	};
};

typedef struct INotify {
	virtual void Notify(){}
} INotify, *INotifyPtr;

/// LIVING DANGEROUSLY
typedef union ThreadClock  {
	CLOCK clock;
	struct {
		unsigned int age;
		unsigned int generation;
	};
	struct {
		byte lsb;
		byte b2;
		byte b3;
		byte msb;
	};

	inline void Increment(unsigned int delta) __attribute__((always_inline));

	void Sleep(byte seconds) {
		clock += MS_CYCLES(seconds * 1000);
	}

	void Repeat() {
		clock = 0;
	}

} ThreadClock, *ThreadClockPtr;


// About 23/32 instruction cycles inline vs. 28
void ThreadClock::Increment(unsigned int delta) {
	age = age + delta;
	if (age < delta) {
		generation++;
	}
}


extern ThreadClock masterClock;

#define MAX_ThreadS 32

typedef struct Thread {
	public:
		virtual void Setup();
		virtual void Heartbeat(){}

		struct Thread *pNext;

		// Threads should increment the heartbeat as desired.
		// Threads with 0 nextHeartbeat will always run ASAP.
		ThreadClock nextHeartbeat;

		byte tardies;
		char id;
} 
Thread, *ThreadPtr;

typedef void (*ValueDelegatePtr)(void *sender, int value);

// Binary pulse with variable width
typedef struct PulseThread : Thread
{
	void Setup(CLOCK period, CLOCK pulseWidth);
	void Heartbeat();

	boolean isHigh;

	protected:
		CLOCK m_LowPeriod;	
		CLOCK m_HighPeriod;
} PulseThread, *PulseThreadPtr;

#define LED_RED 3
#define LED_YELLOW 2
#define LED_GREEN 1
#define LED_NONE 0
#define LED_PIN_RED 13
#define LED_PIN_GRN 12

typedef struct MonitorThread : PulseThread {
	boolean verbose;
	byte	blinkLED;

	void 	LED(byte value);

	/* PRIVATE */ void Setup(int pin1, int pin2);
	/* PRIVATE */ void Error(char *msg, int value);
	/* PRIVATE */ unsigned int Free();
	/* PRIVATE */ void Heartbeat();
	/* PRIVATE */ int m_Pin1;
	/* PRIVATE */ int m_Pin2;
} MonitorThread;

void Error(char *msg, int value);
void ThreadSetup(int monitorPin1, int monitorPin2);
void ThreadRunner();
void ThreadEnable(boolean enable);
void PrintN(char c, byte n);
long MicrosecondsSince(CLOCK lastClock);

extern MonitorThread monitor;
#endif

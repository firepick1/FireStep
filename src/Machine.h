#ifndef MACHINE_H
#define MACHINE_H
#include "SerialTypes.h"
#include "Thread.h"
#include "JCommand.h"

namespace firestep {

#define PIN_X_DIR	2
#define PIN_X		3
#define PIN_Y_DIR	5
#define PIN_Y		6
#define PIN_Z_DIR	9
#define PIN_Z		10
#define ANALOG_SPEED_PIN	5 /* ADC5 (A5) */

#ifdef PRR
/* DIECIMILA */
#define PIN_X_LIM	14
#define PIN_Y_LIM	15
#define PIN_Z_LIM	16
#else
/* MEGA */
#define PIN_X_LIM	54
#define PIN_Y_LIM	55
#define PIN_Z_LIM	56
#endif

// Throttles driver speed from high (255) to low (0)
// #define THROTTLE_SPEED

#define A4988_PULSE_DELAY 	DELAY500NS;DELAY500NS

#define DRV8825_PULSE_DELAY DELAY500NS;DELAY500NS;DELAY500NS;DELAY500NS

#define STEPPER_PULSE_DELAY DRV8825_PULSE_DELAY

#define DELTA_COUNT 120

#ifndef DELAY500NS
#define DELAY500NS \
	asm("nop");asm("nop");asm("nop");asm("nop"); asm("nop");asm("nop");asm("nop");asm("nop");
#endif

typedef struct MachineThread : Thread {
    void setup();
    void Heartbeat();
} MachineThread;

typedef struct CommandParser {
    int cPeek;
    byte peekAvail;

    byte readCommand();
    byte peek(byte c);
    void reset();
} CommandParser;

typedef struct Slack {
    byte maxSlack;
    byte curSlack;
    bool isPlusDelta;

    void init(byte maxSlack);
    int deltaWithBacklash(int delta);
} Slack;

typedef struct SlackVector {
    Slack x;
    Slack y;
    Slack z;
    void init(byte xMax, byte yMax, byte zMax);
} SlackVector;

typedef struct Machine {
    float pathPosition;
    SerialInt16 maxPulses;
    SerialInt32 pulses;
    SerialInt32 heartbeats;
    SerialInt32 heartbeatMicros;
    SerialInt32 actualMicros;
    SerialVector32 unitLengthSteps;
    SerialVector32 drivePos;
    bool xReverse;
    bool yReverse;
    bool zReverse;
    union {
        struct {
            SerialInt32 estimatedMicros;
            SerialVector32 startPos;
            SerialVector32 endPos;
            SerialVector32 segmentStart;
            SerialVector32 segmentStartPos;
            SerialVector16 toolVelocity;
        };
        struct {
            SerialVector16 jogDelta;
            SerialInt32 jogFrequency;
            SerialInt32 jogCount;
            bool		jogOverride;
        };
        struct {
            SerialVector32 backlash;
        };
    };
    SerialVectorF drivePathScale;
    SlackVector slack;
    int segmentIndex;
    SerialInt16 deltaCount;
    SerialVector8 deltas[DELTA_COUNT];

    void init();
	void process(JCommand &jcmd);
    bool doJog();
    bool doAccelerationStroke();
    bool pulseDrivePin(byte stepPin, byte dirPin, byte limitPin, int delta, bool reverse, char axis);
    bool pulseLow(byte stepPin, byte limitPin);
    void sendXYZResponse(struct SerialVector32 *pVector);
    void sendBacklashResponse(struct SerialVector32 *pVector);
} Machine;

typedef struct Controller {
    CommandParser	parser;
    char			guardStart;
    CLOCK			lastClock;
    byte			cmd;
    byte			speed;
    Machine 		machine;
    byte			guardEnd;

    void init();
    byte readCommand();
    bool processCommand();
    bool readAccelerationStroke();
} Controller;

} // namespace firestep

#endif

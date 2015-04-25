#ifndef MACHINE_H
#define MACHINE_H

#include "SerialTypes.h"
#include "Stroke.h"
#include "pins.h"

namespace firestep {

// #define THROTTLE_SPEED /* Throttles driver speed from high (255) to low (0) */

#define A4988_PULSE_DELAY 	DELAY500NS;DELAY500NS
#define DRV8825_PULSE_DELAY DELAY500NS;DELAY500NS;DELAY500NS;DELAY500NS
#define STEPPER_PULSE_DELAY DRV8825_PULSE_DELAY
#define DELTA_COUNT 120
#define MOTOR_COUNT 4
#define AXIS_COUNT 6

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

typedef struct Motor {
    uint8_t	axisMap; 	// index into axis array
    Motor() {}
} Motor;

enum AxisMode {
    MODE_DISABLE = 0,
    MODE_STANDARD = 1,
};

typedef class Axis {
    public:
        uint8_t 	mode;
        PinType 	pinStep;
        PinType 	pinDir;
        PinType 	pinMin;
        PinType 	pinEnable;
        StepCoord 	travelMin;
        StepCoord 	travelMax;
        StepCoord 	searchVelocity;
        StepCoord 	position;
		float		stepAngle;
		uint8_t		microsteps;
		uint8_t		invert;
		uint8_t 	powerManagementMode;
		bool		atMin;
        Axis() :
            mode((uint8_t)MODE_STANDARD),
            pinStep(NOPIN),
            pinDir(NOPIN),
            pinMin(NOPIN),
            travelMin(0),
            travelMax(10000),
            searchVelocity(200),
            position(0),
			stepAngle(1.8),
			microsteps(16),
			invert(0),				// 0:normal direction, 1:inverted direction
			powerManagementMode(0),	// 0:off, 1:on, 2:on in cycle, 3:on when moving
			atMin(false)
        {};
} Axis;

typedef class Machine : public QuadStepper {
        friend class Controller;
        friend class JsonController;
    private:
        Stroke stroke;
        int32_t processMicros;

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
	public:
        Motor motor[MOTOR_COUNT];
        Axis axis[AXIS_COUNT];

    public:
        Machine();
        void init();
		virtual Status step(const Quad<StepCoord> &pulse);

        bool doJog();
		Quad<StepCoord> motorPosition();
        bool doAccelerationStroke();
        bool pulseDrivePin(byte stepPin, byte dirPin, byte limitPin, int delta, bool reverse, char axis);
        bool pulseLow(byte stepPin, byte limitPin);
        void sendXYZResponse(struct SerialVector32 *pVector);
        void sendBacklashResponse(struct SerialVector32 *pVector);
} Machine;

typedef class Controller {
    private:
        CommandParser	parser;
        char			guardStart;
        Ticks			lastClock;
        byte			speed;
        byte			guardEnd;

    public:
        byte 			cmd; // TODO
        Machine 		machine; // TODO
        void init();
        byte readCommand();
        bool processCommand();
        bool readAccelerationStroke();
} Controller;

} // namespace firestep

#endif

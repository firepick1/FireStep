#include "Arduino.h"
#include "Machine.h"
#include "AnalogRead.h"
#include "version.h"
#include "build.h"

using namespace firestep;

#define COMMAND_RESULT_NOP 0
#define COMMAND_RESULT_READ 1
#define COMMAND_RESULT_ERR 3

#define GUARD_BYTE 0x71

#define CMD_ERROR -1
#define CMD_VERSION 1
#define CMD_ACCELERATION_STROKE 2
#define CMD_GET_XYZ 3
#define CMD_SET_XYZ 4
#define CMD_IDLE 5
#define CMD_DIAG 6
#define CMD_GET_UNIT_LENGTH_STEPS 7
#define CMD_SET_UNIT_LENGTH_STEPS 8
#define CMD_RESET 9
#define CMD_JOG 10
#define CMD_SET_BACKLASH 11

#define X_BACKLASH 120

#ifndef ASM
#define ASM(op) asm(op)
#endif

Controller	controller;
bool 		isProcessing;

void MachineThread::setup() {
    id = 'M';
#ifdef THROTTLE_SPEED
    ADC_LISTEN8(ANALOG_SPEED_PIN);
#endif
    Thread::setup();
    controller.init();
    isProcessing = false;
}

void MachineThread::Heartbeat() {
#ifdef THROTTLE_SPEED
    controller.speed = ADCH;
    if (controller.speed <= 251) {
        ThreadEnable(false);
        for (byte iPause = controller.speed; iPause <= 247; iPause++) {
            for (byte iIdle = 0; iIdle < 10; iIdle++) {
				DELAY500NS;
				DELAY500NS;
            }
        }
        ThreadEnable(true);
    }
#endif

    monitor.blinkLED = isProcessing ? LED_GREEN : LED_RED;
    if (isProcessing) {
        long hbDelta = 0;
        switch (controller.cmd) {
        case CMD_JOG:
            hbDelta = FREQ_CYCLES(controller.machine.jogFrequency.longValue);
            nextHeartbeat.clock = masterClock.clock + hbDelta;
            break;
        default:
            nextHeartbeat.Repeat();
            break;
        }
        if (controller.processCommand()) {
            isProcessing = false;
            nextHeartbeat.Repeat();
        }
    } else if (Serial.available() > 0) {
        nextHeartbeat.Repeat();
        monitor.LED(LED_YELLOW);
        byte result = controller.readCommand();
        switch (result) {
        case COMMAND_RESULT_ERR:
            // Command error is 10 second LED_YELLOW
            monitor.blinkLED = LED_NONE;
            nextHeartbeat.clock = masterClock.clock + MS_CYCLES(10000);
            break;
        case COMMAND_RESULT_READ:
            isProcessing = true;
            break;
        }
    } else {
        nextHeartbeat.clock = masterClock.clock + MS_CYCLES(2);	// Simulate work
    }

}


void CommandParser::reset() {
    cPeek = 0x7f;
    peekAvail = false;
}

byte CommandParser::peek(byte c) {
    while (!peekAvail) {
        cPeek = Serial.read();
        peekAvail = cPeek >= 0 ? true : false;
    }
    if (cPeek == c) {
        peekAvail = false;
        return true;
    }
    return false;
}

byte CommandParser::readCommand() {
    reset();
    if (peek('A')) {
        if (peek('S')) {
            return CMD_ACCELERATION_STROKE;
        }
    } else if (peek('D')) {
        if (peek('I') && peek('A') && peek('G') && peek(']')) {
            return CMD_DIAG;
        }
    } else if (peek('G')) {
        if (peek('U')) {
            if (peek('L') && peek('S') && peek(']')) {
                return CMD_GET_UNIT_LENGTH_STEPS;
            }
        } else if (peek('X')) {
            if (peek('Y') && peek('Z') && peek(']')) {
                return CMD_GET_XYZ;
            }
        }
    } else if (peek('I')) {
        if (peek('D') && peek('L') && peek('E')) {
            return CMD_IDLE;
        }
    } else if (peek('J')) {
        if (peek('O') && peek('G')) {
            return CMD_JOG;
        }
    } else if (peek('R')) {
        if (peek('S') && peek('E') && peek('T') && peek(']')) {
            return CMD_RESET;
        }
    } else if (peek('S')) {
        if (peek('U')) {
            if (peek('L') && peek('S')) {
                return CMD_SET_UNIT_LENGTH_STEPS;
            }
        } else if (peek('X')) {
            if (peek('Y') && peek('Z')) {
                return CMD_SET_XYZ;
            }
        } else if (peek('B')) {
            if (peek('A') && peek('K')) {
                return CMD_SET_BACKLASH;
            }
        }
    } else if (peek('V')) {
        if (peek(']') ) {
            return CMD_VERSION;
        }
    }
    return CMD_ERROR;
}


void Controller::init() {

    pinMode(PIN_X, OUTPUT);
    pinMode(PIN_Y, OUTPUT);
    pinMode(PIN_Z, OUTPUT);
    digitalWrite(PIN_X, HIGH);
    digitalWrite(PIN_Y, HIGH);
    digitalWrite(PIN_Z, HIGH);

    pinMode(PIN_X_DIR, OUTPUT);
    pinMode(PIN_Y_DIR, OUTPUT);
    pinMode(PIN_Z_DIR, OUTPUT);

    pinMode(PIN_X_LIM, INPUT);
    pinMode(PIN_Y_LIM, INPUT);
    pinMode(PIN_Z_LIM, INPUT);
    digitalWrite(PIN_X_LIM, HIGH); // enable pull-up resistor
    digitalWrite(PIN_Y_LIM, HIGH); // enable pull-up resistor
    digitalWrite(PIN_Z_LIM, HIGH); // enable pull-up resistor

    guardStart = GUARD_BYTE;
    guardEnd = GUARD_BYTE;
    machine.init();
}

byte Controller::readCommand() {
    machine.heartbeats.longValue = 0;
    int bAvail = Serial.available();
    if (bAvail == 0) {
        return COMMAND_RESULT_NOP;
    }

    if (guardStart != GUARD_BYTE) {
        monitor.Error("GS", guardStart);
    }
    if (guardEnd != GUARD_BYTE) {
        monitor.Error("GE", guardEnd);
    }

    int c = Serial.read();
    if (c == ' ') {
        return COMMAND_RESULT_NOP; // space permits quicker resync
    } else if (c == '[') {
        cmd = parser.readCommand();
        switch (cmd) {
        default:
            return COMMAND_RESULT_ERR;
        case CMD_VERSION:
            break;
        case CMD_DIAG:
            break;
        case CMD_GET_XYZ:
            break;
        case CMD_SET_BACKLASH:
            machine.backlash.read();
            if (parser.peek(']')) {
                machine.slack.init(machine.backlash.x.lsb, machine.backlash.y.lsb, machine.backlash.z.lsb);
                break;
            }
            Serial.println("SBAK?");
            break;
        case CMD_GET_UNIT_LENGTH_STEPS:
            break;
        case CMD_SET_UNIT_LENGTH_STEPS:
            machine.unitLengthSteps.read();
            if (machine.unitLengthSteps.x.longValue < 0) {
                machine.xReverse = true;
                machine.unitLengthSteps.x.longValue = -machine.unitLengthSteps.x.longValue;
            }
            if (machine.unitLengthSteps.y.longValue < 0) {
                machine.yReverse = true;
                machine.unitLengthSteps.y.longValue = -machine.unitLengthSteps.y.longValue;
            }
            if (machine.unitLengthSteps.z.longValue < 0) {
                machine.zReverse = true;
                machine.unitLengthSteps.z.longValue = -machine.unitLengthSteps.z.longValue;
            }
            if (parser.peek(']')) {
                break;
            }
            Serial.println("SULS?");
            return COMMAND_RESULT_ERR;
        case CMD_SET_XYZ:
            machine.drivePos.read();
            if (parser.peek(']')) {
                break;
            }
            Serial.println("SXYZ?");
            return COMMAND_RESULT_ERR;
        case CMD_JOG:
            machine.jogDelta.read();
            machine.jogFrequency.read();
            machine.jogCount.read();
            machine.jogOverride = parser.peek('!');
            if (parser.peek(']')) {
                break;
            }
            Serial.println("JOG?");
            return COMMAND_RESULT_ERR;
        case CMD_ACCELERATION_STROKE:
            if (readAccelerationStroke()) {
                break;
            }
            Serial.println("AS?");
            return COMMAND_RESULT_ERR;
        case CMD_IDLE:
            machine.deltaCount.read();
            if (parser.peek(']')) {
                break;
            }
            Serial.println("IDLE?");
            return COMMAND_RESULT_ERR;
        }
        lastClock = 0;
        machine.actualMicros.longValue = 0;
        machine.pulses.longValue = 0;
        machine.heartbeatMicros.longValue = 0;
        machine.maxPulses.intValue = 0;
        return COMMAND_RESULT_READ;
    }

    Serial.println("CMD?");
}


bool Controller::readAccelerationStroke() {
    SerialVector32 scale;

    machine.startPos.copyFrom(&machine.drivePos);
    machine.endPos.read();
    machine.endPos.increment(&machine.drivePos);
    scale.read();
    machine.estimatedMicros.read();
    machine.deltaCount.read();
    for (int i = 0; i < machine.deltaCount.intValue; i++) {
        machine.deltas[i].read();
    }

    machine.pathPosition = 0;
    machine.segmentIndex = 0;
    machine.toolVelocity.clear();
    machine.segmentStart.clear();
    machine.drivePathScale.copyFrom(&machine.unitLengthSteps);
    machine.drivePathScale.divide(&scale);

    return parser.peek(']');
}

bool Controller::processCommand() {
    bool completed = true;
    machine.heartbeats.longValue++;
    int sz;

    if (lastClock == 0) {
        lastClock = masterClock.clock;
    }
    long elapsed = MicrosecondsSince(lastClock);
    machine.actualMicros.longValue = machine.actualMicros.longValue + elapsed;
    if (elapsed > machine.heartbeatMicros.longValue) {
        machine.heartbeatMicros.longValue = elapsed;
    }
    //if (elapsed> 2000) {
    //	monitor.Error("HB", (int) elapsed);
    //}
    lastClock = masterClock.clock;

    switch (cmd) {
    default:
        Serial.write('E');
        Serial.write('c');
        Serial.write('o');
        Serial.write('C');
        Serial.write('N');
        Serial.write('C');
        Serial.write('?');
        Serial.println(cmd, HEX);
        break;
    case CMD_VERSION:
        Serial.write('[');
        Serial.write('v');
        Serial.write('1');
        Serial.write('.');
        Serial.write('0');
        Serial.println("]");
        break;
    case CMD_DIAG:
        sz = sizeof(Controller);
        Serial.write('[');
        Serial.write('D');
        Serial.write('I');
        Serial.write('A');
        Serial.write('G');
        Serial.print(sz);
        Serial.write(' ');
        sz = DELTA_COUNT;
        Serial.print(sz);
        //Serial.write(' ');
        //sz = (byte*)&completed - (byte*)&guardEnd; // stack free
        //Serial.print(sz);
        Serial.write(' ');
        Serial.write('X');
        sz = digitalRead(PIN_X_LIM);
        Serial.print(sz);
        Serial.write('Y');
        sz = digitalRead(PIN_Y_LIM);
        Serial.print(sz);
        Serial.write('Z');
        sz = digitalRead(PIN_Z_LIM);
        Serial.print(sz);
        Serial.println("]");
        break;
    case CMD_SET_BACKLASH:
        machine.backlash.x.longValue = machine.slack.x.maxSlack;
        machine.backlash.y.longValue = machine.slack.y.maxSlack;
        machine.backlash.z.longValue = machine.slack.z.maxSlack;
        machine.sendBacklashResponse(&machine.backlash);
        break;
    case CMD_SET_UNIT_LENGTH_STEPS:
    case CMD_GET_UNIT_LENGTH_STEPS:
        machine.sendXYZResponse(&machine.unitLengthSteps);
        break;
    case CMD_JOG:
        completed = machine.doJog();
        break;
    case CMD_GET_XYZ:
    case CMD_SET_XYZ:
        machine.sendXYZResponse(&machine.drivePos);
        break;
    case CMD_ACCELERATION_STROKE:
        completed = machine.doAccelerationStroke();
        if (completed) {
            machine.sendXYZResponse(&machine.drivePos);
        }
        break;
    case CMD_IDLE:
        if (machine.deltaCount.intValue > 0) {
            machine.deltaCount.intValue--;
            completed = false;
        } else {
            machine.sendXYZResponse(&machine.drivePos);
        }
        break;
    }

    if (completed) {
        cmd = CMD_ERROR;
    }

    return completed;
}

void Slack::init(byte backlash) {
    curSlack = 0;
    maxSlack = backlash;
    isPlusDelta = true;
}

/* Return delta with backlash compensation */
int Slack::deltaWithBacklash(int delta) {
    int result = delta;
    if (delta < 0) {
        if (isPlusDelta) {
            curSlack = maxSlack - curSlack;
            isPlusDelta = false;
        }
    } else if (delta > 0) {
        if (!isPlusDelta) {
            curSlack = maxSlack - curSlack;
            isPlusDelta = true;
        }
    }
    if (curSlack > 0) {
        if (isPlusDelta) {
            result++;
        } else {
            result--;
        }
        curSlack--;
    }

    return result;
}

void SlackVector::init(byte xMax, byte yMax, byte zMax) {
    x.init(xMax);
    y.init(yMax);
    z.init(zMax);
}

Machine::Machine() {
	for (int i=0; i<MOTOR_COUNT; i++) {
		motor[i].axisMap = i;
	}
}

void Machine::init() {
    xReverse = false;
    yReverse = false;
    zReverse = false;
}

bool Machine::doJog() {
    if (jogCount.longValue > 0) {
        int deltaBacklash;

        deltaBacklash = slack.x.deltaWithBacklash(jogDelta.x.intValue);
        if (!pulseDrivePin(PIN_X, PIN_X_DIR, PIN_X_LIM, deltaBacklash, xReverse, 'X')) {
            if (!jogOverride) {
                goto PULSE_ERROR;
            }
        }
        drivePos.x.longValue = drivePos.x.longValue + jogDelta.x.intValue;

        deltaBacklash = slack.y.deltaWithBacklash(jogDelta.y.intValue);
        if (!pulseDrivePin(PIN_Y, PIN_Y_DIR, PIN_Y_LIM, deltaBacklash, yReverse, 'Y')) {
            if (!jogOverride) {
                goto PULSE_ERROR;
            }
        }
        drivePos.y.longValue = drivePos.y.longValue + jogDelta.y.intValue;

        deltaBacklash = slack.z.deltaWithBacklash(jogDelta.z.intValue);
        if (!pulseDrivePin(PIN_Z, PIN_Z_DIR, PIN_Z_LIM, deltaBacklash, zReverse, 'Z')) {
            if (!jogOverride) {
                goto PULSE_ERROR;
            }
        }
        drivePos.z.longValue = drivePos.z.longValue + jogDelta.z.intValue;

        jogCount.longValue--;
        return false;
    }

    sendXYZResponse(&drivePos);
    return true;

PULSE_ERROR:
    jogCount.longValue = 0;
    return false;
}

void Machine::sendXYZResponse(struct SerialVector32 *pVector) {
    int sz;

    Serial.print("[XYZ");
    pVector->x.send();
    Serial.write(' ');
    pVector->y.send();
    Serial.write(' ');
    pVector->z.send();
    Serial.write(' ');
    Serial.write('X');
    sz = digitalRead(PIN_X_LIM);
    Serial.print(sz);
    Serial.write('Y');
    sz = digitalRead(PIN_Y_LIM);
    Serial.print(sz);
    Serial.write('Z');
    sz = digitalRead(PIN_Z_LIM);
    Serial.print(sz);
    Serial.write(' ');
    actualMicros.send();
    Serial.write(' ');
    heartbeats.send();
    Serial.write(' ');
    heartbeatMicros.send();
    Serial.write(' ');
    pulses.send();
    Serial.write(' ');
    maxPulses.send();
    Serial.write(']');
    Serial.println();
}

void Machine::sendBacklashResponse(struct SerialVector32 *pVector) {
    int sz;

    Serial.write('[');
    Serial.write('S');
    Serial.write('B');
    Serial.write('A');
    Serial.write('K');
    Serial.write(' ');
    pVector->x.send();
    Serial.write(' ');
    pVector->y.send();
    Serial.write(' ');
    pVector->z.send();
    Serial.write(' ');
    Serial.write(']');
    Serial.println();
}

char pulseErr[3];
#define MAX_PULSE 128	/* maximum pulses in one cycle */
#define MAX_PULSE_LIMIT 4 /* maximum pulses when limit switch is active */

// Return true if motion was unrestricted. Return false if limit switch was tripped
bool Machine::pulseDrivePin(byte stepPin, byte dirPin, byte limitPin, int delta, bool reverse, char axis) {
    byte limitCount = 0;

    if (delta < 0) {
        digitalWrite(dirPin, reverse ? HIGH : LOW);
        pulses.longValue += -delta;
        if (-delta > maxPulses.intValue) {
            maxPulses.intValue = -delta;
            if (delta < -MAX_PULSE) {
                pulseErr[0] = '-';
                pulseErr[1] = axis;
                pulseErr[2] = 0;
                monitor.Error(pulseErr, delta);
            }
        }
        while (delta++ < 0 && limitCount < MAX_PULSE_LIMIT) {
            limitCount += pulseLow(stepPin, limitPin);
        }
    } else if (delta > 0) {
        digitalWrite(dirPin, reverse ? LOW : HIGH);
        pulses.longValue += delta;
        if (delta > maxPulses.intValue) {
            maxPulses.intValue = delta;
            if (delta > MAX_PULSE) {
                pulseErr[0] = '+';
                pulseErr[1] = axis;
                pulseErr[2] = 0;
                monitor.Error(pulseErr, delta);
            }
        }
        while (delta-- > 0 && limitCount < MAX_PULSE_LIMIT) {
            limitCount += pulseLow(stepPin, limitPin);
        }
    }

    return limitCount == 0;
}

// Send one pulse to motor. Return true if limit switch was tripped.
bool Machine::pulseLow(byte stepPin, byte limitPin) {
    bool atLimit;

    digitalWrite(stepPin, LOW);

    atLimit = digitalRead(limitPin);
	STEPPER_PULSE_DELAY;

    digitalWrite(stepPin, HIGH);

    return atLimit;
}

bool Machine::doAccelerationStroke() {
    SerialVector32 delta;
    float newPathPosition = actualMicros.longValue / (float) estimatedMicros.longValue;
    bool completed = newPathPosition >= 1;

    if (completed) {
        newPathPosition = 1;
        delta.copyFrom(&endPos);
    } else {
        float segmentCoordinate = newPathPosition * deltaCount.intValue;
        while (segmentIndex < segmentCoordinate) {
            segmentStart.increment(&toolVelocity);
            toolVelocity.increment(&deltas[segmentIndex]);
            segmentIndex++;
            if (segmentIndex >= deltaCount.intValue) {
                segmentStartPos.copyFrom(&drivePos);
            }
        }

        float pDelta = segmentCoordinate - (segmentIndex - 1);
        if (segmentIndex < deltaCount.intValue) {
            SerialVectorF pathOffset;
            pathOffset.copyFrom(&toolVelocity);
            pathOffset.scale(pDelta);
            pathOffset.increment(&segmentStart);
            pathOffset.multiply(&drivePathScale);
            delta.copyFrom(&pathOffset);
            delta.increment(&startPos);
        } else {
            delta.copyFrom(&segmentStartPos);
            delta.interpolateTo(&endPos, pDelta);
        }

    }

    delta.decrement(&drivePos);
    int deltaBacklash;

    deltaBacklash = slack.x.deltaWithBacklash(delta.x.lsInt);
    if (!pulseDrivePin(PIN_X, PIN_X_DIR, PIN_X_LIM, deltaBacklash, xReverse, 'x')) {
        return true;
    }
    deltaBacklash = slack.y.deltaWithBacklash(delta.y.lsInt);
    if (!pulseDrivePin(PIN_Y, PIN_Y_DIR, PIN_Y_LIM, deltaBacklash, yReverse, 'y')) {
        return true;
    }
    deltaBacklash = slack.z.deltaWithBacklash(delta.z.lsInt);
    if (!pulseDrivePin(PIN_Z, PIN_Z_DIR, PIN_Z_LIM, deltaBacklash, zReverse, 'z')) {
        return true;
    }
    drivePos.increment(&delta);

    pathPosition = newPathPosition;

    return completed;
}

static const char * processed = "processed";
void Machine::process(JCommand& jcmd) {
	const char *s;
	JsonObject& root = jcmd.root();
	JsonVariant node;
	node = root;
	Status status = STATUS_COMPLETED;

	if ((s=root["sys"]) && *s==0) {
		node = root["sys"] = jcmd.createJsonObject();
		node["fb"] = BUILD;
		node["fv"] = VERSION_MAJOR*100 + VERSION_MINOR + VERSION_PATCH/100.0;
	} else {
		int i = 0;
		for (JsonObject::iterator it=root.begin(); it!=root.end(); ++it, i++) {
		}
	}

	jcmd.setStatus(status);
}


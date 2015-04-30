#include <cstring>
#include "Arduino.h"
#include "AnalogRead.h"
#include "version.h"
#include "build.h"
#include "MachineThread.h"

using namespace firestep;

void MachineThread::setup() {
    id = 'M';
#ifdef THROTTLE_SPEED
    ADC_LISTEN8(ANALOG_SPEED_PIN);
#endif
    Thread::setup();
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

	nextHeartbeat.ticks = 0;
}


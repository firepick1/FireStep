#ifndef MACHINETHREAD_H
#define MACHINETHREAD_H

#include "JsonController.h"

namespace firestep {

typedef class MachineThread : Thread {
    public:
        void setup();
        void Heartbeat();
        Machine machine;
		JsonController controller;
} MachineThread;

} // namespace firestep

#endif

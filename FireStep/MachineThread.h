#ifndef MACHINETHREAD_H
#define MACHINETHREAD_H

#include "JsonController.h"

namespace firestep {

typedef class MachineThread : Thread {
    public:
		Status status;
        Machine machine;
		JsonCommand command;
		JsonController controller;

	public:
		MachineThread();
        void setup();
        void Heartbeat();
} MachineThread;

} // namespace firestep

#endif

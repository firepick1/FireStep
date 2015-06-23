#ifndef MACHINETHREAD_H
#define MACHINETHREAD_H

#include "JsonController.h"

extern void test_Home();

namespace firestep {

typedef class MachineThread : Thread {
        friend void test_Home();

    protected:
        void displayStatus();
		void executeEEPROM(uint8_t *eeprom_addr);

    public:
        Status status;
        Machine machine;
        JsonCommand command;
        JsonController controller;

    public:
        MachineThread();
        void setup(PinConfig pc);
        void loop();
} MachineThread;

} // namespace firestep

#endif

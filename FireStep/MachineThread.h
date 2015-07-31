#ifndef MACHINETHREAD_H
#define MACHINETHREAD_H

#include "JsonController.h"

extern void test_Home();

namespace firestep {

typedef class MachineThread : Thread {
    friend void test_Home();

protected:
    void displayStatus();
	char * buildStartupJson();
    Status executeEEPROM();
    size_t readEEPROM(uint8_t *eeprom_addr, char *dst, size_t maxLen);
	void printBanner();

public:
    Status status;
    Machine machine;
    JsonCommand command;
    JsonController controller;
	bool printBannerOnIdle;

public:
    MachineThread();
    void setup(PinConfig pc);
    void loop();
    Status syncConfig();
} MachineThread;

} // namespace firestep

#endif

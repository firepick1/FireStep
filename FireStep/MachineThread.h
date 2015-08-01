#ifndef MACHINETHREAD_H
#define MACHINETHREAD_H

#include "JsonController.h"
#include "FPDController.h"

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
	JsonController rawController;
	FPDController fpdController;
    JsonController &controller;
	bool printBannerOnIdle;

public:
    MachineThread();
	void setController(JsonController &controller);
    void setup(PinConfig pc);
    void loop();
    Status syncConfig();
} MachineThread;

} // namespace firestep

#endif

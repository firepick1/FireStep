#ifndef MACHINE_H
#define MACHINE_H
#include "SerialTypes.h"
#include "Thread.h"

#define PIN_X_DIR	2
#define PIN_X		3
#define PIN_Y_DIR	5
#define PIN_Y		6
#define PIN_Z_DIR	9
#define PIN_Z		10

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

#define DELTA_COUNT 120

typedef struct MachineThread : Thread {
	void Setup();
	void Heartbeat();
} MachineThread;

#endif

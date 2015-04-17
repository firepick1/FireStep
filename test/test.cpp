#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include "FireLog.h"
#include "FireUtils.hpp"
#include "version.h"
#include "Arduino.h"
#include "SerialTypes.h"
#include "Machine.h"

byte lastByte;

void test_tick(int ticks) {
	arduino.timer1(ticks);
	threadRunner.outerLoop();
}

void test_Serial() {
    cout << "TEST	: test_Serial() BEGIN" << endl;

	ASSERTEQUAL(0, Serial.available());
	Serial.bytes.clear();
	Serial.bytes.push_back(0x01);
	Serial.bytes.push_back(0x02);
	ASSERTEQUAL(2, Serial.available());
	ASSERTEQUAL(0x01, Serial.read());
	ASSERTEQUAL(0x02, Serial.read());
	ASSERTEQUAL(0, Serial.available());

	ASSERTEQUALS("", Serial.output().c_str());
	Serial.write('x');
	ASSERTEQUALS("x", Serial.output().c_str());
	Serial.print("a");
	ASSERTEQUALS("a", Serial.output().c_str());
	Serial.println("xyz");
	ASSERTEQUALS("xyz\n", Serial.output().c_str());

	SerialInt16 si16;
	Serial.bytes.clear();
	Serial.bytes.push_back(0x01);
	Serial.bytes.push_back(0x02);
	si16.read();
	ASSERTEQUAL(0x0102, si16.intValue);
	ASSERTEQUAL(0, Serial.available());

	SerialInt32 si32;
	Serial.bytes.clear();
	Serial.bytes.push_back(0x01);
	Serial.bytes.push_back(0x02);
	Serial.bytes.push_back(0x03);
	Serial.bytes.push_back(0x04);
	si32.read();
	ASSERTEQUAL(0x0304, si32.lsInt);
	ASSERTEQUAL(0x0102, si32.msInt);
	ASSERTEQUAL(0x01020304, si32.longValue);
	ASSERTEQUAL(0, Serial.available());

	SerialVector8 sv8;
	Serial.bytes.clear();
	Serial.bytes.push_back(0x01);
	Serial.bytes.push_back(0xFF);
	Serial.bytes.push_back(0x7F);
	sv8.read();
	ASSERTEQUAL(1, sv8.x);
	ASSERTEQUAL(-1, sv8.y);
	ASSERTEQUAL(127, sv8.z);
	ASSERTEQUAL(0, Serial.available());

	SerialVector16 sv16;
	Serial.bytes.clear();
	Serial.push((int16_t) 123);
	Serial.push((int16_t) -456);
	Serial.push((int16_t) 789);
	sv16.read();
	ASSERTEQUAL(123, sv16.x.intValue);
	ASSERTEQUAL(-456, sv16.y.intValue);
	ASSERTEQUAL(789, sv16.z.intValue);
	ASSERTEQUAL(0, Serial.available());

	SerialVector16 sv16_copy;
	sv16_copy.clear();
	ASSERTEQUAL(0, sv16_copy.x.intValue);
	ASSERTEQUAL(0, sv16_copy.y.intValue);
	ASSERTEQUAL(0, sv16_copy.z.intValue);
	sv16_copy.copyFrom(&sv16);
	ASSERTEQUAL(123, sv16_copy.x.intValue);
	ASSERTEQUAL(-456, sv16_copy.y.intValue);
	ASSERTEQUAL(789, sv16_copy.z.intValue);
	sv16_copy.clear();
	sv16_copy.increment(&sv8);
	ASSERTEQUAL(1, sv16_copy.x.intValue);
	ASSERTEQUAL(-1, sv16_copy.y.intValue);
	ASSERTEQUAL(127, sv16_copy.z.intValue);

	SerialVectorF svf;
	SerialVector32 sv32;
	sv32.clear();
	ASSERTEQUAL(0, sv32.x.longValue);
	ASSERTEQUAL(0, sv32.y.longValue);
	ASSERTEQUAL(0, sv32.z.longValue);
	Serial.bytes.clear();
	Serial.push((int32_t) 12345);
	Serial.push((int32_t) -45678);
	Serial.push((int32_t) 67890);
	ASSERTEQUAL(12, Serial.available());
	sv32.read();
	ASSERTEQUAL(0, Serial.available());
	ASSERTEQUAL(12345, sv32.x.longValue);
	ASSERTEQUAL(-45678, sv32.y.longValue);
	ASSERTEQUAL(67890, sv32.z.longValue);
	SerialVector32 sv32b;
	Serial.bytes.clear();
	Serial.push((int32_t) 1);
	Serial.push((int32_t) 2);
	Serial.push((int32_t) 3);
	sv32b.read();
	ASSERTEQUAL(1, sv32b.x.longValue);
	ASSERTEQUAL(2, sv32b.y.longValue);
	ASSERTEQUAL(3, sv32b.z.longValue);
	svf.copyFrom(&sv32b);
	sv32b.copyFrom(&sv32);
	ASSERTEQUAL(12345, sv32b.x.longValue);
	ASSERTEQUAL(-45678, sv32b.y.longValue);
	ASSERTEQUAL(67890, sv32b.z.longValue);
	sv32b.copyFrom(&svf);
	ASSERTEQUAL(1, sv32b.x.longValue);
	ASSERTEQUAL(2, sv32b.y.longValue);
	ASSERTEQUAL(3, sv32b.z.longValue);
	sv32b.increment(&sv16);
	ASSERTEQUAL(124, sv32b.x.longValue);
	ASSERTEQUAL(-454, sv32b.y.longValue);
	ASSERTEQUAL(792, sv32b.z.longValue);
	sv32b.increment(&sv32b);
	ASSERTEQUAL(2*124, sv32b.x.longValue);
	ASSERTEQUAL(2*-454, sv32b.y.longValue);
	ASSERTEQUAL(2*792, sv32b.z.longValue);
	sv32b.clear();
	sv32b.increment(&svf);
	ASSERTEQUAL(1, sv32b.x.longValue);
	ASSERTEQUAL(2, sv32b.y.longValue);
	ASSERTEQUAL(3, sv32b.z.longValue);
	sv32b.decrement(&sv32b);
	ASSERTEQUAL(0, sv32b.x.longValue);
	ASSERTEQUAL(0, sv32b.y.longValue);
	ASSERTEQUAL(0, sv32b.z.longValue);
	sv32b.increment(&svf);
	sv32b.clear();
	sv32b.interpolateTo(&sv32, 1);
	ASSERTEQUAL(12345, sv32b.x.longValue);
	ASSERTEQUAL(-45678, sv32b.y.longValue);
	ASSERTEQUAL(67890, sv32b.z.longValue);
	sv32b.interpolateTo(&sv32, 1);
	ASSERTEQUAL(12345, sv32b.x.longValue);
	ASSERTEQUAL(-45678, sv32b.y.longValue);
	ASSERTEQUAL(67890, sv32b.z.longValue);
	sv32b.clear();
	sv32b.interpolateTo(&sv32, 0);
	ASSERTEQUAL(0, sv32b.x.longValue);
	ASSERTEQUAL(0, sv32b.y.longValue);
	ASSERTEQUAL(0, sv32b.z.longValue);
	sv32b.interpolateTo(&sv32, 0.1);
	ASSERTEQUAL(1234, sv32b.x.longValue);
	ASSERTEQUAL(-4567, sv32b.y.longValue);
	ASSERTEQUAL(6789, sv32b.z.longValue);
	sv32b.clear();
	sv32b.interpolateTo(&sv32, 0.9);
	ASSERTEQUAL(11110, sv32b.x.longValue);
	ASSERTEQUAL(-41110, sv32b.y.longValue);
	ASSERTEQUAL(61101, sv32b.z.longValue);

	float epsilon = 0.001;
	svf.copyFrom(&sv16);
	ASSERTEQUALT(123, svf.x, epsilon);
	ASSERTEQUALT(-456, svf.y, epsilon);
	ASSERTEQUALT(789, svf.z, epsilon);
	svf.multiply(&svf);
	ASSERTEQUALT(123*123, svf.x, epsilon);
	ASSERTEQUALT(-456*-456, svf.y, epsilon);
	ASSERTEQUALT(789*789, svf.z, epsilon);
	svf.copyFrom(&sv32);
	ASSERTEQUALT(12345, svf.x, epsilon);
	ASSERTEQUALT(-45678, svf.y, epsilon);
	ASSERTEQUALT(67890, svf.z, epsilon);
	svf.increment(&sv32);
	ASSERTEQUALT(2*12345, svf.x, epsilon);
	ASSERTEQUALT(2*-45678, svf.y, epsilon);
	ASSERTEQUALT(2*67890, svf.z, epsilon);
	svf.divide(&sv32);
	ASSERTEQUALT(2, svf.x, epsilon);
	ASSERTEQUALT(2, svf.y, epsilon);
	ASSERTEQUALT(2, svf.z, epsilon);
	svf.scale(-1.5);
	ASSERTEQUALT(-3, svf.x, epsilon);
	ASSERTEQUALT(-3, svf.y, epsilon);
	ASSERTEQUALT(-3, svf.z, epsilon);

	cout << "TEST	:=== test_Serial() OK " << endl;
}

void test_Thread() {
    cout << "TEST	: test_Thread() BEGIN" << endl;
	arduino.clear();

	threadRunner.setup(LED_PIN_RED, LED_PIN_GRN);

	arduino.dump();
	ASSERTEQUALS(" CLKPR:0 nThreads:1\n", Serial.output().c_str());
	ASSERTEQUAL(OUTPUT, arduino.pinMode[LED_PIN_RED]);
	ASSERTEQUAL(LOW, digitalRead(LED_PIN_RED));
	ASSERTEQUAL(OUTPUT, arduino.pinMode[LED_PIN_GRN]);
	ASSERTEQUAL(LOW, digitalRead(LED_PIN_GRN));
	ASSERTEQUAL(0x0000, TIMSK1); 	// Timer/Counter1 interrupt mask; no interrupts
	ASSERTEQUAL(0x0000, TCCR1A);	// Timer/Counter1 normal port operation
	ASSERTEQUAL(0x0001, TCCR1B);	// Timer/Counter1 active; no prescale
	ASSERTEQUAL(NOVALUE, SREGI); 	// Global interrupts enabled
	test_tick(MS_CYCLES(1));
	ASSERTEQUALS(". S:0 G:0 H:0 T:0\n", Serial.output().c_str());
	for (int i=0; i<6; i++) {
		test_tick(MS_CYCLES(1));
	}
	ASSERTEQUALS(". S:0 G:1 H:1 H/G:1 T:0\n", Serial.output().c_str());

	cout << "TEST	:=== test_Thread() OK " << endl;
}

void test_command(const char *cmd, const char* expected) {
	Serial.push(cmd);
	ASSERTEQUAL(strlen(cmd), Serial.available());
	test_tick(MS_CYCLES(1));
	test_tick(MS_CYCLES(1));
	ASSERTEQUAL(0, Serial.available());
	test_tick(MS_CYCLES(1));
	test_tick(MS_CYCLES(1));
	ASSERTEQUALS(expected, Serial.output().c_str());
}

void test_Machine() {
    cout << "TEST	: test_Machine() BEGIN" << endl;
	arduino.clear();

	MachineThread machThread;
	machThread.setup();
	threadRunner.setup(LED_PIN_RED, LED_PIN_GRN);
	monitor.verbose = false;

	arduino.dump();
	ASSERTEQUALS(" CLKPR:0 nThreads:2\n", Serial.output().c_str());
	ASSERTEQUAL(OUTPUT, arduino.pinMode[PIN_X]);
	ASSERTEQUAL(OUTPUT, arduino.pinMode[PIN_Y]);
	ASSERTEQUAL(OUTPUT, arduino.pinMode[PIN_Z]);
	ASSERTEQUAL(OUTPUT, arduino.pinMode[PIN_X_DIR]);
	ASSERTEQUAL(OUTPUT, arduino.pinMode[PIN_Y_DIR]);
	ASSERTEQUAL(OUTPUT, arduino.pinMode[PIN_Z_DIR]);
	ASSERTEQUAL(INPUT, arduino.pinMode[PIN_X_LIM]);
	ASSERTEQUAL(INPUT, arduino.pinMode[PIN_Y_LIM]);
	ASSERTEQUAL(INPUT, arduino.pinMode[PIN_Z_LIM]);
	ASSERTEQUAL(HIGH, digitalRead(PIN_X_LIM));	// pull-up enabled
	ASSERTEQUAL(HIGH, digitalRead(PIN_Y_LIM));	// pull-up enabled
	ASSERTEQUAL(HIGH, digitalRead(PIN_Z_LIM));	// pull-up enabled
	ASSERTEQUAL(OUTPUT, arduino.pinMode[LED_PIN_RED]);
	ASSERTEQUAL(LOW, digitalRead(LED_PIN_RED));
	ASSERTEQUAL(OUTPUT, arduino.pinMode[LED_PIN_GRN]);
	ASSERTEQUAL(LOW, digitalRead(LED_PIN_GRN));
	ASSERTEQUAL(0x0000, TIMSK1); 	// Timer/Counter1 interrupt mask; no interrupts
	ASSERTEQUAL(0x0000, TCCR1A);	// Timer/Counter1 normal port operation
	ASSERTEQUAL(0x0001, TCCR1B);	// Timer/Counter1 active; no prescale
#ifdef THROTTLE_SPEED
	ASSERTEQUAL((1<<ADEN)|(1<<ADPS2), 
		ADCSRA & ((1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0)));	// ADC 1MHz prescale
	ASSERTEQUAL(0x0000, ADCSRB);	// ADC Control/Status 
	ASSERTEQUAL(0x0065, ADMUX);		// Timer/Counter1 active; no prescale
	ASSERTEQUAL(1, DIDR0&(1<<ANALOG_SPEED_PIN) ? 1 : 0);	// digital pin disable
	ASSERTEQUAL(0, PRR&PRADC);		// Power Reduction Register; ADC enabled
#else
	ASSERTEQUAL(0,
		ADCSRA & ((1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0)));	// ADC 1MHz prescale
#endif
	ASSERTEQUAL(NOVALUE, SREGI); 	// Global interrupts enabled

	// Clock should increase with TCNT1
	CLOCK lastClock = masterClock.clock;
	test_tick(MS_CYCLES(1));
	ASSERT(lastClock < masterClock.clock);
	lastClock = masterClock.clock;
	arduino.dump();

	test_command("[DIAG]", "[DIAG536 120 X1Y1Z1]\n");
	test_command("[V]", "[v1.0]\n");
	//stringstream sscmd;
	//sscmd << "[IDLE";
	//sscmd << "]";
	//test_command(sscmd.str(), "[XYZ00000000 00000000 00000000 X1Y1Z1 00000000 00000001 00000000 00000000 0000]\n");
	test_command("[GULS]", "[XYZ00000000 00000000 00000000 X1Y1Z1 00000000 00000001 00000000 00000000 0000]\n");
	test_command("[GXYZ]", "[XYZ00000000 00000000 00000000 X1Y1Z1 00000000 00000001 00000000 00000000 0000]\n");

	cout << "TEST	:=== test_Machine() OK " << endl;
}

int main(int argc, char *argv[]) {
    LOGINFO3("INFO	: FireStep test v%d.%d.%d",
		VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
    firelog_level(FIRELOG_TRACE);

    test_Serial();
	test_Thread();
	test_Machine();

    cout << "TEST	: END OF TEST main()" << endl;
}

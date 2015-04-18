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
#include "build.h"
#include "JCommand.h"

byte lastByte;

using namespace firestep;

void test_tick(int ticks) {
	arduino.timer1(ticks);
	threadRunner.outerLoop();
}

void test_Serial() {
    cout << "TEST	: test_Serial() BEGIN" << endl;

	ASSERTEQUAL(0, Serial.available());
	Serial.clear();
	Serial.push((uint8_t)0x01);
	Serial.push((uint8_t)0x02);
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
	Serial.clear();
	Serial.push((uint8_t)0x01);
	Serial.push((uint8_t)0x02);
	si16.read();
	ASSERTEQUAL(0x0102, si16.intValue);
	ASSERTEQUAL(0, Serial.available());

	SerialInt32 si32;
	Serial.clear();
	Serial.push((uint8_t)0x01);
	Serial.push((uint8_t)0x02);
	Serial.push((uint8_t)0x03);
	Serial.push((uint8_t)0x04);
	si32.read();
	ASSERTEQUAL(0x0304, si32.lsInt);
	ASSERTEQUAL(0x0102, si32.msInt);
	ASSERTEQUAL(0x01020304, si32.longValue);
	ASSERTEQUAL(0, Serial.available());

	SerialVector8 sv8;
	Serial.clear();
	Serial.push((uint8_t)0x01);
	Serial.push((uint8_t)0xFF);
	Serial.push((uint8_t)0x7F);
	sv8.read();
	ASSERTEQUAL(1, sv8.x);
	ASSERTEQUAL(-1, sv8.y);
	ASSERTEQUAL(127, sv8.z);
	ASSERTEQUAL(0, Serial.available());

	SerialVector16 sv16;
	Serial.clear();
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
	Serial.clear();
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
	Serial.clear();
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

	test_command("[DIAG]", "[DIAG656 120 X1Y1Z1]\n");
	test_command("[V]", "[v1.0]\n");
	//stringstream sscmd;
	//sscmd << "[IDLE";
	//sscmd << "]";
	//test_command(sscmd.str(), "[XYZ00000000 00000000 00000000 X1Y1Z1 00000000 00000001 00000000 00000000 0000]\n");
	test_command("[GULS]", "[XYZ00000000 00000000 00000000 X1Y1Z1 00000000 00000001 00000000 00000000 0000]\n");
	test_command("[GXYZ]", "[XYZ00000000 00000000 00000000 X1Y1Z1 00000000 00000001 00000000 00000000 0000]\n");

	cout << "TEST	:=== test_Machine() OK " << endl;
}

void test_ArduinoJson() {
    cout << "TEST	: test_ArduinoJson() BEGIN" << endl;
	char json[] = "{\"sensor\":\"gps\",\"time\":1351824120,\"data\":[48.756080,2.302038]}";

	StaticJsonBuffer<500> jsonBuffer;

	JsonObject& root = jsonBuffer.parseObject(json);

	ASSERTEQUALS("gps", root["sensor"]);
	ASSERTEQUAL(1351824120L, (int32_t)root["time"]);
	ASSERTEQUAL(48.756080, root["data"][0]);
	ASSERTEQUAL(2.302038, root["data"][1]);

	JsonVariant& nothing = root["nothing"];
	ASSERT(!nothing.success());
	ASSERT(!nothing.is<double>());
	ASSERT(!nothing.is<long>());
	ASSERT(!nothing.is<const char *>());
	ASSERT(NULL == (const char *) root["time"]);

	// C string values MUST be invariant during JsonVariant lifetime
	char buf[] = {'r','e','d',0};
	ASSERT((root["color"] = buf).success());
	ASSERTEQUALS("red", root["color"]);
	buf[0] = 0;	// ----------- DANGER -----------
	ASSERTEQUALS("", root["color"]);

	// Keys are ordered as created and iterable as created
	JsonObject& obj = jsonBuffer.createObject();
	obj["a"] = 1;
	obj["c"] = 2;
	obj["b"] = 3;
	obj["e"] = 4;
	obj["d"] = 5;
	Serial.clear();
	obj.printTo(Serial);
	ASSERTEQUALS("{\"a\":1,\"c\":2,\"b\":3,\"e\":4,\"d\":5}", Serial.output().c_str());
	ASSERTEQUAL(5, obj.size());
	int i=0;
	for (JsonObject::iterator it=obj.begin(); it!=obj.end(); ++it, i++) {
		switch(i) {
		case 0: ASSERTEQUALS("a", it->key); break;
		case 1: ASSERTEQUALS("c", it->key); break;
		case 2: ASSERTEQUALS("b", it->key); break;
		case 3: ASSERTEQUALS("e", it->key); break;
		case 4: ASSERTEQUALS("d", it->key); break;
		}
	}

	cout << "TEST	:=== test_ArduinoJson() OK " << endl;
}

void test_JCommand() {
    cout << "TEST	: test_JCommand() BEGIN" << endl;

	JCommand cmd1;
	ASSERT(!cmd1.root().success());
	ASSERT(cmd1.parse("{\"sys\":\"\"}"));
	ASSERTEQUAL(STATUS_JSON_PARSED, cmd1.getStatus());
	ASSERT(cmd1.isValid());
	JsonVariant& sys = cmd1.root()["sys"];
	ASSERTEQUALS("", sys);
	ASSERT(!sys.is<double>());
	ASSERT(!sys.is<long>());
	ASSERT(sys.is<const char *>());
	
	JCommand cmd2;
	ASSERT(cmd2.parse("{\"x\":123,\"y\":2.3}"));
	ASSERTEQUAL(STATUS_JSON_PARSED, cmd2.getStatus());
	ASSERT(cmd2.isValid());
	ASSERTEQUALT(2.3, cmd2.root()["y"], 0.001);
	ASSERTEQUAL(123.0, cmd2.root()["x"]);
	JsonVariant& y = cmd2.root()["y"];
	ASSERTEQUALT(2.3, y, 0.001);
	ASSERT(y.success());
	ASSERT(y.is<double>());
	ASSERT(!y.is<long>());
	JsonVariant& x = cmd2.root()["x"];
	ASSERT(x.is<long>());
	ASSERT(!x.is<double>());

	const char *json1 = "{\"x\":-0.1";
	const char *json2 = "23}\n";
	Serial.push(json1);
	JCommand cmd3;
	ASSERT(!cmd3.parse());
	Serial.push(json2);
	ASSERT(cmd3.parse());
	ASSERTEQUAL(STATUS_JSON_PARSED, cmd3.getStatus());
	ASSERT(cmd3.isValid());
	ASSERT(cmd3.root().success());
	x = cmd3.root()["x"];
	ASSERTEQUAL(-0.123, x);

	Serial.clear();
	cmd3.root().printTo(Serial);
	ASSERTEQUALS("{\"x\":-0.123}", Serial.output().c_str());

	Serial.clear();
	cmd3.response().printTo(Serial);
	ASSERTEQUALS("{\"s\":-1,\"r\":{\"x\":-0.123}}", Serial.output().c_str());

	cout << "TEST	:=== test_JCommand() OK " << endl;
}

void test_Machine_process() {
    cout << "TEST	: test_Machine_process() BEGIN" << endl;

	Machine machine;

	Serial.clear();
	JCommand jcmd1;
	ASSERT(jcmd1.parse("{\"sys\":\"\"}"));
	machine.process(jcmd1);
	jcmd1.response().printTo(Serial);
	char sysbuf[500];
	snprintf(sysbuf, sizeof(sysbuf), "{\"s\":%d,\"r\":{\"sys\":{\"fb\":\"%s\",\"fv\":%.2f}}}",
		STATUS_COMPLETED, BUILD, VERSION_MAJOR*100 + VERSION_MINOR + VERSION_PATCH/100.0);
	ASSERTEQUALS(sysbuf, Serial.output().c_str());

	cout << "TEST	:=== test_Machine_process() OK " << endl;
}

int main(int argc, char *argv[]) {
    LOGINFO3("INFO	: FireStep test v%d.%d.%d",
		VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
    firelog_level(FIRELOG_TRACE);

    test_Serial();
	test_Thread();
	test_Machine();
	test_ArduinoJson();
	test_JCommand();
	test_Machine_process();

    cout << "TEST	: END OF TEST main()" << endl;
}

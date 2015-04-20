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
#include "JsonController.h"

byte lastByte;

using namespace firestep;
using namespace ArduinoJson;

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

	char buf[200];
	snprintf(buf, sizeof(buf), "[DIAG%ld 120 X1Y1Z1]\n", (long) sizeof(Controller));
	test_command("[DIAG]", buf);
	test_command("[V]", "[v1.0]\n");
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
	JsonObject& jobj = jsonBuffer.createObject();
	jobj["a"] = 1;
	jobj["c"] = 2;
	jobj["b"] = 3;
	jobj["e"] = 4;
	jobj["d"] = 5;
	Serial.clear();
	jobj.printTo(Serial);
	ASSERTEQUALS("{\"a\":1,\"c\":2,\"b\":3,\"e\":4,\"d\":5}", Serial.output().c_str());
	ASSERTEQUAL(5, jobj.size());
	int i=0;
	JsonVariant jv; // cannot combine with next line
	jv = jobj;
	JsonObject& jobj2 = static_cast<JsonObject&>(jv);
	for (JsonObject::iterator it=jobj2.begin(); it!=jobj2.end(); ++it, i++) {
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

void test_JsonCommand() {
    cout << "TEST	: test_JsonCommand() BEGIN" << endl;

	JsonCommand cmd1;
	ASSERT(!cmd1.root().success());
	ASSERT(cmd1.parse("{\"sys\":\"\"}"));
	ASSERTEQUAL(STATUS_JSON_PARSED, cmd1.getStatus());
	ASSERT(cmd1.isValid());
	JsonVariant& sys = cmd1.root()["sys"];
	ASSERTEQUALS("", sys);
	ASSERT(!sys.is<double>());
	ASSERT(!sys.is<long>());
	ASSERT(sys.is<const char *>());
	
	JsonCommand cmd2;
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
	JsonCommand cmd3;
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
	ASSERTEQUALS("{\"s\":1,\"r\":{\"x\":-0.123}}", Serial.output().c_str());

	cout << "TEST	:=== test_JsonCommand() OK " << endl;
}

void replaceChar(string &s, char cmatch, char creplace) {
	for (int i=0; i<s.size(); i++) { 
		if (s[i] == cmatch) {
			s[i] = creplace;
		}
	}
}

void testJSON_process(JsonController&jc, JsonCommand &jcmd, string replace, const char *jsonOut) {
	Serial.clear();
	string jo(jsonOut);
	for (int i=0; i<replace.size(); i+=2) {
		char cmatch = replace[i];
		char creplace = replace[i+1];
		replaceChar(jo, cmatch, creplace);
	}
	jc.process(jcmd);
	jcmd.response().printTo(Serial);
	ASSERTEQUALS(jo.c_str(), Serial.output().c_str());
}

JsonCommand testJSON(JsonController &jc, string replace, const char *jsonIn, const char* jsonOut) {
	string ji(jsonIn);
	for (int i=0; i<replace.size(); i+=2) {
		char cmatch = replace[i];
		char creplace = replace[i+1];
		replaceChar(ji, cmatch, creplace);
	}
	JsonCommand jcmd; 
	ASSERT(jcmd.parse(ji.c_str()));

	testJSON_process(jc, jcmd, replace, jsonOut);

	return jcmd;
}

void test_JsonController_motor(JsonController &jc, char motor) {
	string replace;
	replace.push_back('\''); replace.push_back('"');
	replace.push_back('?'); replace.push_back(motor);
	replace.push_back('!'); replace.push_back(motor-1);
	testJSON(jc, replace, "{'?':''}", "{'s':0,'r':{'?':{'ma':!,'sa':1.80,'mi':16,'po':0,'pm':0}}}");
	testJSON(jc, replace, "{'?ma':''}", "{'s':0,'r':{'?ma':!}}");
	testJSON(jc, replace, "{'?ma':4}", "{'s':0,'r':{'?ma':4}}");
	testJSON(jc, replace, "{'?ma':''}", "{'s':0,'r':{'?ma':4}}");
	testJSON(jc, replace, "{'?':{'ma':''}}", "{'s':0,'r':{'?':{'ma':4}}}");
	testJSON(jc, replace, "{'?':{'ma':!}}", "{'s':0,'r':{'?':{'ma':!}}}");
	testJSON(jc, replace, "{'?':{'ma':''}}", "{'s':0,'r':{'?':{'ma':!}}}");
	testJSON(jc, replace, "{'?sa':''}", "{'s':0,'r':{'?sa':1.80}}");
	testJSON(jc, replace, "{'?sa':0.90}", "{'s':0,'r':{'?sa':0.90}}");
	testJSON(jc, replace, "{'?sa':''}", "{'s':0,'r':{'?sa':0.90}}");
	testJSON(jc, replace, "{'?':{'sa':''}}", "{'s':0,'r':{'?':{'sa':0.90}}}");
	testJSON(jc, replace, "{'?':{'sa':1.80}}", "{'s':0,'r':{'?':{'sa':1.80}}}");
	testJSON(jc, replace, "{'?':{'sa':''}}", "{'s':0,'r':{'?':{'sa':1.80}}}");
	testJSON(jc, replace, "{'?mi':''}", "{'s':0,'r':{'?mi':16}}");
	testJSON(jc, replace, "{'?mi':33}", "{'s':0,'r':{'?mi':33}}");
	testJSON(jc, replace, "{'?mi':''}", "{'s':0,'r':{'?mi':33}}");
	testJSON(jc, replace, "{'?':{'mi':''}}", "{'s':0,'r':{'?':{'mi':33}}}");
	testJSON(jc, replace, "{'?':{'mi':16}}", "{'s':0,'r':{'?':{'mi':16}}}");
	testJSON(jc, replace, "{'?':{'mi':''}}", "{'s':0,'r':{'?':{'mi':16}}}");
	testJSON(jc, replace, "{'?po':''}", "{'s':0,'r':{'?po':0}}");
	testJSON(jc, replace, "{'?po':2}", "{'s':0,'r':{'?po':2}}");
	testJSON(jc, replace, "{'?po':''}", "{'s':0,'r':{'?po':2}}");
	testJSON(jc, replace, "{'?':{'po':''}}", "{'s':0,'r':{'?':{'po':2}}}");
	testJSON(jc, replace, "{'?':{'po':0}}", "{'s':0,'r':{'?':{'po':0}}}");
	testJSON(jc, replace, "{'?':{'po':''}}", "{'s':0,'r':{'?':{'po':0}}}");
	testJSON(jc, replace, "{'?pm':''}", "{'s':0,'r':{'?pm':0}}");
	testJSON(jc, replace, "{'?pm':2}", "{'s':0,'r':{'?pm':2}}");
	testJSON(jc, replace, "{'?pm':''}", "{'s':0,'r':{'?pm':2}}");
	testJSON(jc, replace, "{'?':{'pm':''}}", "{'s':0,'r':{'?':{'pm':2}}}");
	testJSON(jc, replace, "{'?':{'pm':0}}", "{'s':0,'r':{'?':{'pm':0}}}");
	testJSON(jc, replace, "{'?':{'pm':''}}", "{'s':0,'r':{'?':{'pm':0}}}");
	testJSON(jc, replace, "{'?':''}", "{'s':0,'r':{'?':{'ma':!,'sa':1.80,'mi':16,'po':0,'pm':0}}}");
}

void test_JsonController_axis(JsonController &jc, char axis) {
	string replace;
	replace.push_back('\''); replace.push_back('"');
	replace.push_back('?'); replace.push_back(axis);
	testJSON(jc, replace, "{'?tn':''}", "{'s':0,'r':{'?tn':0}}");		// default
	testJSON(jc, replace, "{'?tn':111}", "{'s':0,'r':{'?tn':111}}");	
	testJSON(jc, replace, "{'?tn':''}", "{'s':0,'r':{'?tn':111}}");
	testJSON(jc, replace, "{'?':{'tn':''}}", "{'s':0,'r':{'?':{'tn':111}}}");
	testJSON(jc, replace, "{'?':{'tn':0}}", "{'s':0,'r':{'?':{'tn':0}}}");
	testJSON(jc, replace, "{'?':{'tn':''}}", "{'s':0,'r':{'?':{'tn':0}}}");
	testJSON(jc, replace, "{'?tm':''}", "{'s':0,'r':{'?tm':10000}}");  	// default
	testJSON(jc, replace, "{'?tm':222}", "{'s':0,'r':{'?tm':222}}");	
	testJSON(jc, replace, "{'?tm':''}", "{'s':0,'r':{'?tm':222}}");	
	testJSON(jc, replace, "{'?':{'tm':''}}", "{'s':0,'r':{'?':{'tm':222}}}");
	testJSON(jc, replace, "{'?':{'tm':10000}}", "{'s':0,'r':{'?':{'tm':10000}}}");
	testJSON(jc, replace, "{'?':{'tm':''}}", "{'s':0,'r':{'?':{'tm':10000}}}");
	testJSON(jc, replace, "{'?am':''}", "{'s':0,'r':{'?am':1}}");  	// default
	testJSON(jc, replace, "{'?am':333}", "{'s':0,'r':{'?am':77}}");	// out of range
	testJSON(jc, replace, "{'?am':''}", "{'s':0,'r':{'?am':77}}");	
	testJSON(jc, replace, "{'?':{'am':''}}", "{'s':0,'r':{'?':{'am':77}}}");
	testJSON(jc, replace, "{'?':{'am':1}}", "{'s':0,'r':{'?':{'am':1}}}");
	testJSON(jc, replace, "{'?':{'am':''}}", "{'s':0,'r':{'?':{'am':1}}}");

	testJSON(jc, replace, 
		"{'x':''}", "{'s':0,'r':{'x':{'am':1,'pd':22,'pe':14,'pn':21,'po':0,'ps':23,'tm':10000,'tn':0}}}");
	testJSON(jc, replace, 
		"{'y':''}", "{'s':0,'r':{'y':{'am':1,'pd':3,'pe':14,'pn':2,'po':0,'ps':4,'tm':10000,'tn':0}}}");
	testJSON(jc, replace, 
		"{'z':''}", "{'s':0,'r':{'z':{'am':1,'pd':12,'pe':14,'pn':11,'po':0,'ps':13,'tm':10000,'tn':0}}}");
}

void test_JsonController_machinePosition(JsonController &jc) {
	string replace;
	replace.push_back('\''); replace.push_back('"');
	testJSON(jc, replace, "{'spo':''}", "{'s':0,'r':{'spo':{'x':0,'y':0,'z':0,'a':0,'b':0,'c':0}}}");
	testJSON(jc, replace, "{'spox':''}", "{'s':0,'r':{'spox':0}}");
	testJSON(jc, replace, "{'spox':32760}", "{'s':0,'r':{'spox':32760}}");
	testJSON(jc, replace, "{'spox':''}", "{'s':0,'r':{'spox':32760}}");
	testJSON(jc, replace, "{'spox':-32760}", "{'s':0,'r':{'spox':-32760}}");
	testJSON(jc, replace, "{'spox':''}", "{'s':0,'r':{'spox':-32760}}");
	testJSON(jc, replace, "{'spoy':''}", "{'s':0,'r':{'spoy':0}}");
	testJSON(jc, replace, "{'spoy':32761}", "{'s':0,'r':{'spoy':32761}}");
	testJSON(jc, replace, "{'spoy':''}", "{'s':0,'r':{'spoy':32761}}");
	testJSON(jc, replace, "{'spoy':-32761}", "{'s':0,'r':{'spoy':-32761}}");
	testJSON(jc, replace, "{'spoy':''}", "{'s':0,'r':{'spoy':-32761}}");
	testJSON(jc, replace, "{'spoz':''}", "{'s':0,'r':{'spoz':0}}");
	testJSON(jc, replace, "{'spoz':32762}", "{'s':0,'r':{'spoz':32762}}");
	testJSON(jc, replace, "{'spoz':''}", "{'s':0,'r':{'spoz':32762}}");
	testJSON(jc, replace, "{'spoz':-32762}", "{'s':0,'r':{'spoz':-32762}}");
	testJSON(jc, replace, "{'spoz':''}", "{'s':0,'r':{'spoz':-32762}}");
	testJSON(jc, replace, "{'spoa':''}", "{'s':0,'r':{'spoa':0}}");
	testJSON(jc, replace, "{'spoa':32763}", "{'s':0,'r':{'spoa':32763}}");
	testJSON(jc, replace, "{'spoa':''}", "{'s':0,'r':{'spoa':32763}}");
	testJSON(jc, replace, "{'spoa':-32763}", "{'s':0,'r':{'spoa':-32763}}");
	testJSON(jc, replace, "{'spoa':''}", "{'s':0,'r':{'spoa':-32763}}");
	testJSON(jc, replace, "{'spob':''}", "{'s':0,'r':{'spob':0}}");
	testJSON(jc, replace, "{'spob':32764}", "{'s':0,'r':{'spob':32764}}");
	testJSON(jc, replace, "{'spob':''}", "{'s':0,'r':{'spob':32764}}");
	testJSON(jc, replace, "{'spob':-32764}", "{'s':0,'r':{'spob':-32764}}");
	testJSON(jc, replace, "{'spob':''}", "{'s':0,'r':{'spob':-32764}}");
	testJSON(jc, replace, "{'spoc':''}", "{'s':0,'r':{'spoc':0}}");
	testJSON(jc, replace, "{'spoc':32765}", "{'s':0,'r':{'spoc':32765}}");
	testJSON(jc, replace, "{'spoc':''}", "{'s':0,'r':{'spoc':32765}}");
	testJSON(jc, replace, "{'spoc':-32765}", "{'s':0,'r':{'spoc':-32765}}");
	testJSON(jc, replace, "{'spoc':''}", "{'s':0,'r':{'spoc':-32765}}");
	testJSON(jc, replace, "{'spo':''}", 
		"{'s':0,'r':{'spo':{'x':-32760,'y':-32761,'z':-32762,'a':-32763,'b':-32764,'c':-32765}}}");
}

void test_JsonController_stroke(JsonController &jc) {
	string replace;
	replace.push_back('\''); replace.push_back('"');
	JsonCommand jcmd = testJSON(jc, replace, 
		"{'str':{'s1':[1,2,3],'s2':[4,5,6],'s3':[7,8,9],'s4':[-10,-11,-12]}}", 
		"{'s':2,'r':{'str':{'s1':[1,2,3],'s2':[4,5,6],'s3':[7,8,9],'s4':[-10,-11,-12]}}}");
	testJSON_process(jc, jcmd, replace, 
		"{'s':2,'r':{'str':{'s1':1,'s2':4,'s3':7,'s4':-10}}}");
	testJSON_process(jc, jcmd, replace, 
		"{'s':2,'r':{'str':{'s1':2,'s2':5,'s3':8,'s4':-11}}}");
	testJSON_process(jc, jcmd, replace, 
		"{'s':0,'r':{'str':{'s1':3,'s2':6,'s3':9,'s4':-12}}}");
	testJSON_process(jc, jcmd, replace, 
		"{'s':0,'r':{'str':{'s1':3,'s2':6,'s3':9,'s4':-12}}}");
}

void test_JsonController() {
    cout << "TEST	: test_JsonController() BEGIN" << endl;

	Machine machine;
	JsonController jc(machine);

	Serial.clear();
	JsonCommand jcmd;
	ASSERT(jcmd.parse("{\"sys\":\"\"}"));
	jc.process(jcmd);
	jcmd.response().printTo(Serial);
	char sysbuf[500];
	snprintf(sysbuf, sizeof(sysbuf), "{\"s\":%d,\"r\":{\"sys\":{\"fb\":\"%s\",\"fv\":%.2f}}}",
		STATUS_OK, BUILD, VERSION_MAJOR*100 + VERSION_MINOR + VERSION_PATCH/100.0);
	ASSERTEQUALS(sysbuf, Serial.output().c_str());

	test_JsonController_axis(jc, 'x');
	test_JsonController_axis(jc, 'y');
	test_JsonController_axis(jc, 'z');
	test_JsonController_axis(jc, 'a');
	test_JsonController_axis(jc, 'b');
	test_JsonController_axis(jc, 'c');

	test_JsonController_motor(jc, '1');
	test_JsonController_motor(jc, '2');
	test_JsonController_motor(jc, '3');
	test_JsonController_motor(jc, '4');

	test_JsonController_machinePosition(jc);

	test_JsonController_stroke(jc);

	cout << "TEST	:=== test_JsonController() OK " << endl;
}

int main(int argc, char *argv[]) {
    LOGINFO3("INFO	: FireStep test v%d.%d.%d",
		VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
    firelog_level(FIRELOG_TRACE);

    test_Serial();
	test_Thread();
	test_Machine();
	test_ArduinoJson();
	test_JsonCommand();
	test_JsonController();

    cout << "TEST	: END OF TEST main()" << endl;
}

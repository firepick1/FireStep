#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include "FireLog.h"
#include "FireUtils.hpp"
#include "version.h"
#include "Arduino.h"

#include "MachineThread.h"
#include "Display.h"

byte lastByte;

using namespace ph5;
using namespace firestep;
using namespace ArduinoJson;

#define ASSERTQUAD(expected,actual) ASSERTEQUALS( expected.toString().c_str(), actual.toString().c_str() );

class TestDisplay: public Display {
    public:
        char message[100];
        virtual void show() {
            snprintf(message, sizeof(message), "status:%d level:%d", status, level);
        }
        void clear() {
            memset(message, 0, sizeof(message));
        }
} testDisplay;

void test_ticks(int nTicks) {
    arduino.timer1(nTicks-1);
	ticks();
    threadRunner.outerLoop();
}

void test_Serial() {
    cout << "TEST	: test_Serial() =====" << endl;

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

    cout << "TEST	: test_Serial() OK " << endl;
}

void test_Thread() {
    cout << "TEST	: test_Thread() =====" << endl;
    arduino.clear();

    ThreadClock tc;
    Ticks tcLast = tc.ticks;
    ASSERTEQUAL(0, tc.ticks);
    ASSERTEQUAL(0, tc.age);
    ASSERTEQUAL(0, tc.generation);
    tc.age++;
    ASSERTEQUAL(1, tc.ticks - tcLast);
    tc.generation++;
    ASSERTEQUAL(65537, tc.ticks - tcLast);
    ASSERTEQUAL(64, TICK_MICROSECONDS);
    ASSERTEQUAL(15625, TICKS_PER_SECOND);

    threadRunner.setup();
    monitor.verbose = false;

	ASSERTEQUAL(4, sizeof(PH5TYPE));
    ASSERTEQUAL(16000, MS_CYCLES(1));
    ASSERTEQUAL(7812, (int32_t) MS_TICKS(500));
    ASSERTEQUAL(15625, MS_TICKS(1000));
    arduino.dump();
    //ASSERTEQUALS(" CLKPR:0 nThreads:1\n", Serial.output().c_str());
    ASSERTEQUAL(0x0000, TIMSK1); 	// Timer/Counter1 interrupt mask; no interrupts
    ASSERTEQUAL(0x0000, TCCR1A);	// Timer/Counter1 normal port operation
    ASSERTEQUAL(0x0005, TCCR1B);	// Timer/Counter1 active; prescale 1024
    ASSERTEQUAL(NOVALUE, SREGI); 	// Global interrupts enabled
	ASSERT(TIMER_ENABLED);
	cout << "TCNT1:" << (uint16_t) TCNT1 << endl;
    test_ticks(1);
	cout << "TCNT1:" << (uint16_t) TCNT1 << endl;
    Ticks lastClock = ticks();
	ASSERTEQUAL(lastClock+1, ticks());
	uint32_t lastTCNT1 = (uint32_t) (uint16_t) TCNT1;

	arduino.timer1(1);

	ASSERTEQUAL(lastTCNT1+1L, (uint32_t) (uint16_t) TCNT1);
	ASSERTEQUAL(lastClock+3, ticks());

    cout << "TEST	: test_Thread() OK " << endl;
}

void test_command(const char *cmd, const char* expected) {
    Serial.clear();
    Serial.push(cmd);
    ASSERTEQUAL(strlen(cmd), Serial.available());
    test_ticks(MS_TICKS(1));
    test_ticks(MS_TICKS(1));
    test_ticks(MS_TICKS(1));
    test_ticks(MS_TICKS(1));
    ASSERTEQUALS(expected, Serial.output().c_str());
}

void test_Machine() {
    cout << "TEST	: test_Machine() =====" << endl;

    arduino.clear();
    arduino.setPin(PC2_X_MIN_PIN, 0);
    arduino.setPin(PC2_Y_MIN_PIN, 0);
    arduino.setPin(PC2_Z_MIN_PIN, 0);
    Machine machine;
    machine.enable(true);
    ASSERTEQUAL(OUTPUT, arduino.getPinMode(PC2_X_STEP_PIN));
    ASSERTEQUAL(LOW, arduino.getPin(PC2_X_STEP_PIN));
    ASSERTEQUAL(OUTPUT, arduino.getPinMode(PC2_X_DIR_PIN));
    ASSERTEQUAL(LOW, arduino.getPin(PC2_X_DIR_PIN));
    ASSERTEQUAL(OUTPUT, arduino.getPinMode(PC2_X_ENABLE_PIN));
    ASSERTEQUAL(LOW, arduino.getPin(PC2_X_ENABLE_PIN));
    ASSERTEQUAL(INPUT, arduino.getPinMode(PC2_X_MIN_PIN));
    ASSERTEQUAL(OUTPUT, arduino.getPinMode(PC2_Y_STEP_PIN));
    ASSERTEQUAL(LOW, arduino.getPin(PC2_Y_STEP_PIN));
    ASSERTEQUAL(OUTPUT, arduino.getPinMode(PC2_Y_DIR_PIN));
    ASSERTEQUAL(LOW, arduino.getPin(PC2_Y_DIR_PIN));
    ASSERTEQUAL(OUTPUT, arduino.getPinMode(PC2_Y_ENABLE_PIN));
    ASSERTEQUAL(LOW, arduino.getPin(PC2_Y_ENABLE_PIN));
    ASSERTEQUAL(INPUT, arduino.getPinMode(PC2_Y_MIN_PIN));
    ASSERTEQUAL(OUTPUT, arduino.getPinMode(PC2_Z_STEP_PIN));
    ASSERTEQUAL(LOW, arduino.getPin(PC2_Z_STEP_PIN));
    ASSERTEQUAL(OUTPUT, arduino.getPinMode(PC2_Z_DIR_PIN));
    ASSERTEQUAL(LOW, arduino.getPin(PC2_Z_DIR_PIN));
    ASSERTEQUAL(OUTPUT, arduino.getPinMode(PC2_Z_ENABLE_PIN));
    ASSERTEQUAL(LOW, arduino.getPin(PC2_Z_ENABLE_PIN));
    ASSERTEQUAL(INPUT, arduino.getPinMode(PC2_Z_MIN_PIN));
    ASSERTEQUAL(1, arduino.pulses(PC2_X_STEP_PIN));
    ASSERTEQUAL(1, arduino.pulses(PC2_Y_STEP_PIN));
    ASSERTEQUAL(1, arduino.pulses(PC2_Z_STEP_PIN));

    ASSERT(machine.axis[0].isEnabled());
    ASSERT(machine.axis[1].isEnabled());
    ASSERT(machine.axis[2].isEnabled());
    ASSERTEQUAL(STATUS_OK, machine.step(Quad<StepCoord>(1, 1, 1, 0)));
    ASSERTEQUAL(2, arduino.pulses(PC2_X_STEP_PIN));
    ASSERTEQUAL(2, arduino.pulses(PC2_Y_STEP_PIN));
    ASSERTEQUAL(2, arduino.pulses(PC2_Z_STEP_PIN));

    ASSERTEQUAL(STATUS_OK, machine.step(Quad<StepCoord>(-1, -1, -1, 0)));
    ASSERTEQUAL(3, arduino.pulses(PC2_X_STEP_PIN));
    ASSERTEQUAL(3, arduino.pulses(PC2_Y_STEP_PIN));
    ASSERTEQUAL(3, arduino.pulses(PC2_Z_STEP_PIN));


    MachineThread machThread;
    machThread.setup();
    threadRunner.setup();
    monitor.verbose = false;

    arduino.dump();
    Serial.clear();
    ASSERTEQUAL(0x0000, TIMSK1); 	// Timer/Counter1 interrupt mask; no interrupts
    ASSERTEQUAL(0x0000, TCCR1A);	// Timer/Counter1 normal port operation
    ASSERTEQUAL(0x0005, TCCR1B);	// Timer/Counter1 active; no prescale
#ifdef THROTTLE_SPEED
    ASSERTEQUAL((1 << ADEN) | (1 << ADPS2),
                ADCSRA & ((1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0)));	// ADC 1MHz prescale
    ASSERTEQUAL(0x0000, ADCSRB);	// ADC Control/Status
    ASSERTEQUAL(0x0065, ADMUX);		// Timer/Counter1 active; no prescale
    ASSERTEQUAL(1, DIDR0 & (1 << ANALOG_SPEED_PIN) ? 1 : 0);	// digital pin disable
    ASSERTEQUAL(0, PRR & PRADC);		// Power Reduction Register; ADC enabled
#else
    ASSERTEQUAL(0,
                ADCSRA & ((1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0)));	// ADC 1MHz prescale
#endif
    ASSERTEQUAL(NOVALUE, SREGI); 	// Global interrupts enabled

    // ticks should increase with TCNT1
    Ticks lastClock = ticks();
    test_ticks(1);
    ASSERT(lastClock < ticks());
    lastClock = ticks();
    arduino.dump();

    cout << "TEST	: test_Machine() OK " << endl;
}

void test_BadJson(const char *json) {
    StaticJsonBuffer<JSON_OBJECT_SIZE(4)> jbTiny;
    char tinybuf[2000];
    snprintf(tinybuf, sizeof(tinybuf), "%s", json);
    JsonObject &jtiny = jbTiny.parseObject(tinybuf);
    if (jtiny.success()) {
        char output[2000];
        jtiny.printTo(output, sizeof(output));
        //cout << output << endl;
        ASSERTEQUALS(json, output);
        int kids = 0;
        for (JsonObject::iterator it = jtiny.begin();
                it != jtiny.end();
                ++it) {
            kids++;
        }
        ASSERT(kids > 0);
    }
}

void test_ArduinoJson() {
    cout << "TEST	: test_ArduinoJson() =====" << endl;
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
    char buf[] = {'r', 'e', 'd', 0};
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
    int i = 0;
    JsonVariant jv; // cannot combine with next line
    jv = jobj;
    JsonObject& jobj2 = static_cast<JsonObject&>(jv);
    for (JsonObject::iterator it = jobj2.begin(); it != jobj2.end(); ++it, i++) {
        switch (i) {
        case 0:
            ASSERTEQUALS("a", it->key);
            break;
        case 1:
            ASSERTEQUALS("c", it->key);
            break;
        case 2:
            ASSERTEQUALS("b", it->key);
            break;
        case 3:
            ASSERTEQUALS("e", it->key);
            break;
        case 4:
            ASSERTEQUALS("d", it->key);
            break;
        }
    }

    // Re-use jsonBuffer
    jsonBuffer.clear(); // existing jsonBuffer doesn't have enough memory before clear()
    char json2[] = "{\"sensor\":\"gps\",\"time\":1351824120,\"data\":[48.756080,2.302038]}";
    JsonObject& root2 = jsonBuffer.parseObject(json2);
    ASSERTEQUALS("gps", root2["sensor"]);
    ASSERTEQUAL(1351824120L, (int32_t)root2["time"]);
    ASSERTEQUAL(48.756080, root2["data"][0]);
    ASSERTEQUAL(2.302038, root2["data"][1]);

    test_BadJson( "{\"a\":1,\"b\":2}" );
    test_BadJson( "{\"a\":1,\"b\":2,\"c\":3}" );
    test_BadJson( "{\"a\":1,\"b\":2,\"c\":3,\"d\":4}" );
    test_BadJson( "{\"a\":1,\"b\":2,\"c\":3,\"d\":4,\"e\":5}" );
    test_BadJson( "{\"a\":1,\"b\":2,\"c\":3,\"d\":4,\"e\":5,\"f\":6}" );
    test_BadJson( "{\"a\":123}" );
    test_BadJson( "{\"a\":{\"b\":123}}" );
    test_BadJson( "{\"a\":{\"b\":{\"c\":123}}}" );
    test_BadJson( "{\"a\":{\"b\":{\"c\":{\"d\":123}}}}" );
    test_BadJson( "{\"a\":" );
    test_BadJson( "{\"a\":\"\":" );
    test_BadJson( "{\"a\":[1]}" );
    test_BadJson( "{\"a\":[1,2]}" );
    test_BadJson( "{\"a\":[1,2,3]}" );
    test_BadJson( "{\"a\":[1,2,3,4]}" );
    test_BadJson( "{\"a\":[1,2,3,4,5]}" );
    test_BadJson( "{\"a\":[1,2,3,4,5,6]}" );
    test_BadJson( "{\"a\":[1,2,3,4,5,6,7]}" );
    test_BadJson( "{\"a\":[1,2,3,4,5,6,7,8]}" );
    test_BadJson( "{\"a\":{\"b\":[1]}}" );
    test_BadJson( "{\"a\":{\"b\":[1,2]}}" );
    test_BadJson( "{\"a\":{\"b\":[1,2,3]}}" );
    test_BadJson( "{\"a\":{\"b\":[1,2,3,4]}}" );
    test_BadJson( "{\"a\":{\"b\":[1,2,3,4,5]}}" );
    test_BadJson( "{\"a\":{\"b\":[1,2,3,4,5,6]}}" );
    test_BadJson( "{\"a\":{\"b\":[1,2,3,4,5,6,7]}}" );
    test_BadJson( "{\"a\":{\"b\":[1,2,3,4,5,6,7,8]}}" );

    cout << "TEST	: test_ArduinoJson() OK " << endl;
}

void test_JsonCommand() {
    cout << "TEST	: test_JsonCommand() =====" << endl;

    JsonCommand cmd1;
    ASSERT(!cmd1.requestRoot().success());
    ASSERTEQUAL(STATUS_BUSY_PARSED, cmd1.parse("{\"sys\":\"\"}"));
    ASSERTEQUAL(STATUS_BUSY_PARSED, cmd1.getStatus());
    ASSERT(cmd1.isValid());
    JsonVariant& sys = cmd1.requestRoot()["sys"];
    ASSERTEQUALS("", sys);
    ASSERT(!sys.is<double>());
    ASSERT(!sys.is<long>());
    ASSERT(sys.is<const char *>());

    JsonCommand cmd2;
    ASSERTEQUAL(STATUS_BUSY_PARSED, cmd2.parse("{\"x\":123,\"y\":2.3}"));
    ASSERTEQUAL(STATUS_BUSY_PARSED, cmd2.getStatus());
    ASSERT(cmd2.isValid());
    ASSERTEQUALT(2.3, cmd2.requestRoot()["y"], 0.001);
    ASSERTEQUAL(123.0, cmd2.requestRoot()["x"]);
    JsonVariant& y = cmd2.requestRoot()["y"];
    ASSERTEQUALT(2.3, y, 0.001);
    ASSERT(y.success());
    ASSERT(y.is<double>());
    ASSERT(!y.is<long>());
    JsonVariant& x = cmd2.requestRoot()["x"];
    ASSERT(x.is<long>());
    ASSERT(!x.is<double>());

    const char *json1 = "{\"x\":-0.1";
    const char *json2 = "23}\n";
    Serial.push(json1);
    JsonCommand cmd3;
    ASSERTEQUAL(STATUS_WAIT_EOL, cmd3.parse());
    Serial.push(json2);
    ASSERTEQUAL(STATUS_BUSY_PARSED, cmd3.parse());
    ASSERTEQUAL(STATUS_BUSY_PARSED, cmd3.getStatus());
    ASSERT(cmd3.isValid());
    ASSERT(cmd3.requestRoot().success());
    x = cmd3.requestRoot()["x"];
    ASSERTEQUAL(-0.123, x);

    Serial.clear();
    cmd3.requestRoot().printTo(Serial);
    ASSERTEQUALS("{\"x\":-0.123}", Serial.output().c_str());

    Serial.clear();
    cmd3.response().printTo(Serial);
    ASSERTEQUALS("{\"s\":10,\"r\":{\"x\":-0.123}}", Serial.output().c_str());

    cout << "TEST	: test_JsonCommand() OK " << endl;
}

void replaceChar(string &s, char cmatch, char creplace) {
    for (int i = 0; i < s.size(); i++) {
        if (s[i] == cmatch) {
            s[i] = creplace;
        }
    }
}

void testJSON_process(Machine& machine, JsonController&jc, JsonCommand &jcmd, string replace,
                      const char *jsonOut, Status status = STATUS_OK) {
    Serial.clear();
    string jo(jsonOut);
    for (int i = 0; i < replace.size(); i += 2) {
        char cmatch = replace[i];
        char creplace = replace[i + 1];
        replaceChar(jo, cmatch, creplace);
    }
	ticks();
    Status actualStatus = jc.process(jcmd);
    ASSERTEQUAL(status, actualStatus);
	ASSERT(jcmd.requestAvailable() > sizeof(JsonVariant));
	ASSERT(jcmd.responseAvailable() > sizeof(JsonVariant));
    ASSERTEQUALS(jo.c_str(), Serial.output().c_str());
}

string jsonTemplate(const char *jsonIn, string replace = "'\"") {
    string ji(jsonIn);
    for (int i = 0; i < replace.size(); i += 2) {
        char cmatch = replace[i];
        char creplace = replace[i + 1];
        replaceChar(ji, cmatch, creplace);
    }
    return ji;
}
#define JT(s) (jsonTemplate(s).c_str())

JsonCommand testJSON(Machine& machine, JsonController &jc, string replace, const char *jsonIn,
                     const char* jsonOut, Status processStatus = STATUS_OK) {
    string ji(jsonTemplate(jsonIn, replace));
    JsonCommand jcmd;
    Status status = jcmd.parse(ji.c_str());
    ASSERTEQUAL(STATUS_BUSY_PARSED, status);
    char parseOut[MAX_JSON];
    jcmd.requestRoot().printTo(parseOut, sizeof(parseOut));
    ASSERTEQUALS(ji.c_str(), parseOut);

    testJSON_process(machine, jc, jcmd, replace, jsonOut, processStatus);

    return jcmd;
}

void test_JsonController_motor(Machine& machine, JsonController &jc, char motor) {
    string replace;
    replace.push_back('\'');
    replace.push_back('"');
    replace.push_back('?');
    replace.push_back(motor);
    replace.push_back('!');
    replace.push_back(motor - 1);
    testJSON(machine, jc, replace, "{'?':''}", "{'s':0,'r':{'?':{'ma':!}}}\n");
    testJSON(machine, jc, replace, "{'?ma':''}", "{'s':0,'r':{'?ma':!}}\n");
    testJSON(machine, jc, replace, "{'?ma':4}", "{'s':0,'r':{'?ma':4}}\n");
    testJSON(machine, jc, replace, "{'?ma':''}", "{'s':0,'r':{'?ma':4}}\n");
    testJSON(machine, jc, replace, "{'?':{'ma':''}}", "{'s':0,'r':{'?':{'ma':4}}}\n");
    testJSON(machine, jc, replace, "{'?':{'ma':!}}", "{'s':0,'r':{'?':{'ma':!}}}\n");
    testJSON(machine, jc, replace, "{'?':{'ma':''}}", "{'s':0,'r':{'?':{'ma':!}}}\n");
}

void test_JsonController_axis(Machine& machine, JsonController &jc, char axis) {
    string replace;
    replace.push_back('\'');
    replace.push_back('"');
    replace.push_back('?');
    replace.push_back(axis);
    testJSON(machine, jc, replace, "{'?tn':''}", "{'s':0,'r':{'?tn':0}}\n");		// default
    testJSON(machine, jc, replace, "{'?tn':111}", "{'s':0,'r':{'?tn':111}}\n");
    testJSON(machine, jc, replace, "{'?tn':''}", "{'s':0,'r':{'?tn':111}}\n");
    testJSON(machine, jc, replace, "{'?':{'tn':''}}", "{'s':0,'r':{'?':{'tn':111}}}\n");
    testJSON(machine, jc, replace, "{'?':{'tn':0}}", "{'s':0,'r':{'?':{'tn':0}}}\n");
    testJSON(machine, jc, replace, "{'?':{'tn':''}}", "{'s':0,'r':{'?':{'tn':0}}}\n");
    testJSON(machine, jc, replace, "{'?tm':''}", "{'s':0,'r':{'?tm':32000}}\n");  	// default
    testJSON(machine, jc, replace, "{'?tm':222}", "{'s':0,'r':{'?tm':222}}\n");
    testJSON(machine, jc, replace, "{'?tm':''}", "{'s':0,'r':{'?tm':222}}\n");
    testJSON(machine, jc, replace, "{'?':{'tm':''}}", "{'s':0,'r':{'?':{'tm':222}}}\n");
    testJSON(machine, jc, replace, "{'?':{'tm':32000}}", "{'s':0,'r':{'?':{'tm':32000}}}\n");
    testJSON(machine, jc, replace, "{'?':{'tm':''}}", "{'s':0,'r':{'?':{'tm':32000}}}\n");
    testJSON(machine, jc, replace, "{'?':{'mi':''}}", "{'s':0,'r':{'?':{'mi':16}}}\n");
    testJSON(machine, jc, replace, "{'?':{'mi':1}}", "{'s':0,'r':{'?':{'mi':1}}}\n");
    testJSON(machine, jc, replace, "{'?':{'mi':''}}", "{'s':0,'r':{'?':{'mi':1}}}\n");
    testJSON(machine, jc, replace, "{'?':{'mi':16}}", "{'s':0,'r':{'?':{'mi':16}}}\n");
    testJSON(machine, jc, replace, "{'?':{'dh':''}}", "{'s':0,'r':{'?':{'dh':true}}}\n");
    testJSON(machine, jc, replace, "{'?':{'dh':false}}", "{'s':0,'r':{'?':{'dh':false}}}\n");
    testJSON(machine, jc, replace, "{'?':{'dh':''}}", "{'s':0,'r':{'?':{'dh':false}}}\n");
    testJSON(machine, jc, replace, "{'?':{'dh':true}}", "{'s':0,'r':{'?':{'dh':true}}}\n");
    testJSON(machine, jc, replace, "{'?':{'sa':''}}", "{'s':0,'r':{'?':{'sa':1.80}}}\n");
    testJSON(machine, jc, replace, "{'?':{'sa':0.9}}", "{'s':0,'r':{'?':{'sa':0.90}}}\n");
    testJSON(machine, jc, replace, "{'?':{'sa':''}}", "{'s':0,'r':{'?':{'sa':0.90}}}\n");
    testJSON(machine, jc, replace, "{'?':{'sa':1.8}}", "{'s':0,'r':{'?':{'sa':1.80}}}\n");

    testJSON(machine, jc, replace, "{'x':''}",
             "{'s':0,'r':{'x':{'dh':true,'en':true,'ho':0,'is':0,'lb':16,'lm':false,'ln':false,"\
			 "'mi':16,'pd':55,'pe':38,'pm':255,'pn':3,'po':0,'ps':54,"\
			 "'sa':1.80,'sd':80,'tm':32000,'tn':0,'ud':0}}}\n");
    testJSON(machine, jc, replace, "{'y':''}",
             "{'s':0,'r':{'y':{'dh':true,'en':true,'ho':0,'is':0,'lb':16,'lm':false,'ln':false,"\
			 "'mi':16,'pd':61,'pe':56,'pm':255,'pn':14,'po':0,'ps':60,"\
			 "'sa':1.80,'sd':80,'tm':32000,'tn':0,'ud':0}}}\n");
    testJSON(machine, jc, replace, "{'z':''}",
             "{'s':0,'r':{'z':{'dh':true,'en':true,'ho':0,'is':0,'lb':16,'lm':false,'ln':false,"\
			 "'mi':16,'pd':48,'pe':62,'pm':255,'pn':18,'po':0,'ps':46,"\
			 "'sa':1.80,'sd':80,'tm':32000,'tn':0,'ud':0}}}\n");
}

void test_JsonController_machinePosition(Machine& machine, JsonController &jc) {
    string replace;
    replace.push_back('\'');
    replace.push_back('"');
    testJSON(machine, jc, replace, "{'mpo':''}", "{'s':0,'r':{'mpo':{'1':0,'2':0,'3':0,'4':0}}}\n");
    testJSON(machine, jc, replace, "{'mpo1':''}", "{'s':0,'r':{'mpo1':0}}\n");
    testJSON(machine, jc, replace, "{'mpo1':32760}", "{'s':0,'r':{'mpo1':32760}}\n");
    testJSON(machine, jc, replace, "{'mpo1':''}", "{'s':0,'r':{'mpo1':32760}}\n");
    testJSON(machine, jc, replace, "{'mpo1':-32760}", "{'s':0,'r':{'mpo1':-32760}}\n");
    testJSON(machine, jc, replace, "{'mpo1':''}", "{'s':0,'r':{'mpo1':-32760}}\n");
    testJSON(machine, jc, replace, "{'mpo2':''}", "{'s':0,'r':{'mpo2':0}}\n");
    testJSON(machine, jc, replace, "{'mpo2':32761}", "{'s':0,'r':{'mpo2':32761}}\n");
    testJSON(machine, jc, replace, "{'mpo2':''}", "{'s':0,'r':{'mpo2':32761}}\n");
    testJSON(machine, jc, replace, "{'mpo2':-32761}", "{'s':0,'r':{'mpo2':-32761}}\n");
    testJSON(machine, jc, replace, "{'mpo2':''}", "{'s':0,'r':{'mpo2':-32761}}\n");
    testJSON(machine, jc, replace, "{'mpo3':''}", "{'s':0,'r':{'mpo3':0}}\n");
    testJSON(machine, jc, replace, "{'mpo3':32762}", "{'s':0,'r':{'mpo3':32762}}\n");
    testJSON(machine, jc, replace, "{'mpo3':''}", "{'s':0,'r':{'mpo3':32762}}\n");
    testJSON(machine, jc, replace, "{'mpo3':-32762}", "{'s':0,'r':{'mpo3':-32762}}\n");
    testJSON(machine, jc, replace, "{'mpo3':''}", "{'s':0,'r':{'mpo3':-32762}}\n");
    testJSON(machine, jc, replace, "{'mpo4':''}", "{'s':0,'r':{'mpo4':0}}\n");
    testJSON(machine, jc, replace, "{'mpo4':32763}", "{'s':0,'r':{'mpo4':32763}}\n");
    testJSON(machine, jc, replace, "{'mpo4':''}", "{'s':0,'r':{'mpo4':32763}}\n");
    testJSON(machine, jc, replace, "{'mpo4':-32763}", "{'s':0,'r':{'mpo4':-32763}}\n");
    testJSON(machine, jc, replace, "{'mpo4':''}", "{'s':0,'r':{'mpo4':-32763}}\n");
    testJSON(machine, jc, replace, "{'mpo':''}",
             "{'s':0,'r':{'mpo':{'1':-32760,'2':-32761,'3':-32762,'4':-32763}}}\n");
}

MachineThread test_setup() {
    arduino.clear();
    threadRunner.clear();
    MachineThread mt;
    mt.machine.pDisplay = &testDisplay;
    testDisplay.clear();
    mt.setup();
    Serial.clear();
	delayMicsTotal = 0;
    arduino.setPin(mt.machine.axis[0].pinMin, 0);
    arduino.setPin(mt.machine.axis[1].pinMin, 0);
    arduino.setPin(mt.machine.axis[2].pinMin, 0);
    mt.loop();
    ASSERTEQUAL(LOW, arduino.getPin(PC2_X_DIR_PIN));
    ASSERTEQUAL(LOW, arduino.getPin(PC2_Y_DIR_PIN));
    ASSERTEQUAL(LOW, arduino.getPin(PC2_Z_DIR_PIN));
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);
    ASSERTQUAD(Quad<StepCoord>(0, 0, 0, 0), mt.machine.getMotorPosition());
    ASSERTEQUALS("FireStep 0.1.2\n", Serial.output().c_str());
	for (int i=0; i<MOTOR_COUNT; i++) {
		ASSERTEQUAL((size_t) &mt.machine.axis[i], (size_t) &mt.machine.getMotorAxis(i));
	}

    return mt;
}

void test_JsonController_tst() {
    MachineThread mt = test_setup();
    int32_t xdirpulses;
    int32_t ydirpulses;
    int32_t zdirpulses;
    int32_t xpulses;
    int32_t ypulses;
    int32_t zpulses;
    Ticks ticks;
    //uint32_t usDelay;

    threadClock.ticks++;
    //usDelay = arduino.get_usDelay();
    ticks = mt.controller.getLastProcessed();
    Serial.push(JT("{'tstrv':[1,2]}\n")); // tstrv: test revolutions steps
    mt.loop();	// command.parse
    ASSERTEQUAL(ticks, mt.controller.getLastProcessed());
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    ASSERTEQUAL(0, Serial.available()); // expected parse
    ASSERTEQUAL(DISPLAY_BUSY, mt.machine.pDisplay->getStatus());
    ASSERTEQUAL(LOW, arduino.getPin(PC2_X_DIR_PIN));
    ASSERTEQUAL(LOW, arduino.getPin(PC2_Y_DIR_PIN));
    //ASSERTEQUAL(usDelay, arduino.get_usDelay());
    xpulses = arduino.pulses(PC2_X_STEP_PIN);
    ypulses = arduino.pulses(PC2_Y_STEP_PIN);
    zpulses = arduino.pulses(PC2_Z_STEP_PIN);
    xdirpulses = arduino.pulses(PC2_X_DIR_PIN);
    ydirpulses = arduino.pulses(PC2_Y_DIR_PIN);
    zdirpulses = arduino.pulses(PC2_Z_DIR_PIN);

    ticks = ++threadClock.ticks;
    mt.loop();	// controller.process
    ASSERTEQUAL(ticks, mt.controller.getLastProcessed());
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    ASSERTEQUAL(DISPLAY_BUSY_MOVING, mt.machine.pDisplay->getStatus());
    ASSERTEQUAL(xpulses + 2 * 3200, arduino.pulses(PC2_X_STEP_PIN));
    ASSERTEQUAL(ypulses + 2 * 6400, arduino.pulses(PC2_Y_STEP_PIN));
    ASSERTEQUAL(zpulses, arduino.pulses(PC2_Z_STEP_PIN));
    ASSERTEQUAL(xdirpulses, arduino.pulses(PC2_X_DIR_PIN));
    ASSERTEQUAL(ydirpulses, arduino.pulses(PC2_Y_DIR_PIN));
    ASSERTEQUAL(zdirpulses, arduino.pulses(PC2_Z_DIR_PIN));
    ASSERTEQUAL(HIGH, arduino.getPin(PC2_X_DIR_PIN));
    ASSERTEQUAL(HIGH, arduino.getPin(PC2_Y_DIR_PIN));
    //ASSERTEQUAL(usDelay+2*6400L*80L, arduino.get_usDelay());
    ASSERTEQUALS("", Serial.output().c_str());

    threadClock.ticks++;
    Serial.push("\n"); // cancel current command
    mt.loop();
    ASSERTEQUAL(STATUS_WAIT_CANCELLED, mt.status);
    ASSERTEQUAL(DISPLAY_WAIT_CANCELLED, mt.machine.pDisplay->getStatus());
    ASSERTEQUALS(JT("{'s':-901,'r':{'tstrv':[1,2]}}\n"), Serial.output().c_str());

    threadClock.ticks++;
    mt.loop();
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);
    ASSERTEQUAL(DISPLAY_WAIT_IDLE, mt.machine.pDisplay->getStatus());

    ++threadClock.ticks;
    Serial.push(JT("{'tstsp':[1,100,1000]}\n")); // tstsp: test stepper pulse
    ticks = mt.controller.getLastProcessed();
    xpulses = arduino.pulses(PC2_X_STEP_PIN);
    ypulses = arduino.pulses(PC2_Y_STEP_PIN);
    zpulses = arduino.pulses(PC2_Z_STEP_PIN);
    mt.loop();	// parse
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    ASSERTEQUAL(DISPLAY_BUSY, mt.machine.pDisplay->getStatus());
    ASSERTEQUAL(0, Serial.available());

    threadClock.ticks++;
    ticks = threadClock.ticks;
    mt.loop();	// controller.process
    ASSERTEQUAL(ticks, mt.controller.getLastProcessed());
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUAL(DISPLAY_WAIT_IDLE, mt.machine.pDisplay->getStatus());
    ASSERTEQUAL(xpulses + 1, arduino.pulses(PC2_X_STEP_PIN));
    ASSERTEQUAL(ypulses + 100, arduino.pulses(PC2_Y_STEP_PIN));
    ASSERTEQUAL(zpulses + 1000, arduino.pulses(PC2_Z_STEP_PIN));
    ASSERTEQUALS(JT("{'s':0,'r':{'tstsp':[1,100,1000]}}\n"), Serial.output().c_str());

    threadClock.ticks++;
    mt.loop();
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);
    ASSERTEQUAL(DISPLAY_WAIT_IDLE, mt.machine.pDisplay->getStatus());
    ASSERTEQUAL(xpulses + 1, arduino.pulses(PC2_X_STEP_PIN));
    ASSERTEQUAL(ypulses + 100, arduino.pulses(PC2_Y_STEP_PIN));
    ASSERTEQUAL(zpulses + 1000, arduino.pulses(PC2_Z_STEP_PIN));
    ASSERTEQUALS("", Serial.output().c_str());

    threadClock.ticks++;
    //usDelay = arduino.get_usDelay();
    ticks = mt.controller.getLastProcessed();
    Serial.push(JT("{'tstrv':[-1,-2]}\n")); // tstrv: test revolutions steps
    mt.loop();	// command.parse
    ASSERTEQUAL(ticks, mt.controller.getLastProcessed());
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    ASSERTEQUAL(DISPLAY_BUSY, mt.machine.pDisplay->getStatus());
    ASSERTEQUAL(0, Serial.available()); // expected parse
    xpulses = arduino.pulses(PC2_X_STEP_PIN);
    ypulses = arduino.pulses(PC2_Y_STEP_PIN);
    zpulses = arduino.pulses(PC2_Z_STEP_PIN);

    ticks = ++threadClock.ticks;
    mt.loop();	// controller.process
    ASSERTEQUAL(ticks, mt.controller.getLastProcessed());
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    ASSERTEQUAL(DISPLAY_BUSY_MOVING, mt.machine.pDisplay->getStatus());
    ASSERTEQUAL(xpulses + 2 * 3200, arduino.pulses(PC2_X_STEP_PIN));
    ASSERTEQUAL(ypulses + 2 * 6400, arduino.pulses(PC2_Y_STEP_PIN));
    ASSERTEQUAL(zpulses, arduino.pulses(PC2_Z_STEP_PIN));
    //ASSERTEQUAL(usDelay+2*6400L*80L, arduino.get_usDelay());

    ticks = ++threadClock.ticks;
    mt.loop();	// controller.process
    ASSERTEQUAL(ticks, mt.controller.getLastProcessed());
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    ASSERTEQUAL(DISPLAY_BUSY_MOVING, mt.machine.pDisplay->getStatus());
    ASSERTEQUAL(xpulses + 4 * 3200L, arduino.pulses(PC2_X_STEP_PIN));
    ASSERTEQUAL(ypulses + 4 * 6400L, arduino.pulses(PC2_Y_STEP_PIN));
    ASSERTEQUAL(zpulses, arduino.pulses(PC2_Z_STEP_PIN));
    ASSERTEQUAL(xdirpulses + 2, arduino.pulses(PC2_X_DIR_PIN));
    ASSERTEQUAL(ydirpulses + 2, arduino.pulses(PC2_Y_DIR_PIN));
    ASSERTEQUAL(zdirpulses, arduino.pulses(PC2_Z_DIR_PIN));
    ASSERTEQUAL(HIGH, arduino.getPin(PC2_X_DIR_PIN));
    ASSERTEQUAL(HIGH, arduino.getPin(PC2_Y_DIR_PIN));
    //ASSERTEQUAL(usDelay+4*6400L*80L, arduino.get_usDelay());
}

void test_JsonController_stroke(Machine& machine, JsonController &jc) {
    string replace;
    replace.push_back('\'');
    replace.push_back('"');

    // Set machine to known position and state
	threadRunner.clear();
	TCNT1 = 00;
    arduino.setPin(machine.axis[0].pinMin, 0);
    arduino.setPin(machine.axis[1].pinMin, 0);
    arduino.setPin(machine.axis[2].pinMin, 0);
    machine.axis[0].enable(true);
    machine.axis[1].enable(true);
    machine.axis[2].enable(true);
#define STROKE_CONFIG "{"\
			"'xtm':32000,'xtn':0,'xpo':5,'xln':false,"\
			"'ytm':32000,'ytn':0,'ypo':5,'yln':false,"\
			"'ztm':32000,'ztn':0,'zpo':5,'zln':false,"\
			"'atm':32000,'atn':0,'apo':5,'aln':false,'aps':255}"
    string jconfig = STROKE_CONFIG;
    testJSON(machine, jc, replace, jconfig.c_str(), "{'s':0,'r':" STROKE_CONFIG "}\n");
    ASSERTQUAD(Quad<StepCoord>(5, 5, 5, 5), machine.getMotorPosition());
    Ticks tStart = 101;
	TCNT1 = 99;

    //////////////////// TEST dEndPos.isZero() ////////////////
    // parse and initialize stroke
    JsonCommand jcmd =
        testJSON(machine, jc, replace,
                 "{'systc':'','dvs':{'us':128,'1':[1,2],'2':[4,5],'3':[7,8]}}",
                 "", STATUS_BUSY_MOVING);
    ASSERTQUAD(Quad<StepCoord>(0, 0, 0, 0), machine.stroke.position());
    ASSERTQUAD(Quad<StepCoord>(5, 5, 5, 5), machine.getMotorPosition());
    ASSERTEQUAL(tStart, machine.stroke.tStart);
	ASSERTEQUAL(2, machine.stroke.get_dtTotal());
	float epsilon = 0.000001;
    ASSERTEQUALT(0.000128, machine.stroke.getTotalTime(), epsilon);
    ASSERTQUAD(Quad<StepCoord>(4, 13, 22, 0), machine.stroke.dEndPos);

	TCNT1--; // mock ticks() increments TCNT1, so decrement to simulate no change in ticks()

    // traverse first stroke segment
    ASSERTEQUAL(0, digitalRead(PC2_Z_MIN_PIN));
    testJSON_process(machine, jc, jcmd, replace, "", STATUS_BUSY_MOVING);
    ASSERTEQUALS("", Serial.output().c_str());
    ASSERTEQUAL(0, digitalRead(PC2_Z_MIN_PIN));
    ASSERTQUAD(Quad<StepCoord>(1, 4, 7, 0), machine.stroke.position());
    ASSERTQUAD(Quad<StepCoord>(6, 9, 12, 5), machine.getMotorPosition()); // axis a NOPIN inactive
    size_t reqAvail = jcmd.requestAvailable();
    size_t resAvail = jcmd.responseAvailable();

    // finalize stroke
    testJSON_process(machine, jc, jcmd, replace,
                     "{'s':0,'r':{'systc':103,'dvs':{'us':128,'1':4,'2':13,'3':22}}}\n",
                     STATUS_OK);
    ASSERTQUAD(Quad<StepCoord>(4, 13, 22, 0), machine.stroke.position());
    ASSERTQUAD(Quad<StepCoord>(9, 18, 27, 5), machine.getMotorPosition()); // axis a is NOPIN inactive
    ASSERTEQUAL(tStart + 3, threadClock.ticks);
    ASSERTEQUAL(reqAvail, jcmd.requestAvailable());
    ASSERTEQUAL(resAvail, jcmd.responseAvailable());

    ////////////////////// TEST !dEndPos.isZero() /////////////
	machine.setMotorPosition(Quad<StepCoord>(5, 5, 5, 5));
	tStart = ticks()+2;

    // parse and initialize stroke
    jcmd = testJSON(machine, jc, replace,
                    "{'systc':'','dvs':{'us':128,'dp':[10,20],'x':[1,2],'y':[4,5],'z':[7,8]}}",
                    "", STATUS_BUSY_MOVING);
    ASSERTQUAD(Quad<StepCoord>(0, 0, 0, 0), machine.stroke.position());
    ASSERTQUAD(Quad<StepCoord>(5, 5, 5, 5), machine.getMotorPosition());
    ASSERTEQUAL(tStart, machine.stroke.tStart);
    ASSERTEQUAL(2, machine.stroke.getTotalTime()*TICKS_PER_SECOND);
    ASSERTQUAD(Quad<StepCoord>(10, 20, 0, 0), machine.stroke.dEndPos);

	TCNT1--; // simulate unchanging ticks()

    // traverse first stroke segment
    ASSERTEQUAL(0, digitalRead(PC2_Z_MIN_PIN));
    testJSON_process(machine, jc, jcmd, replace, "", STATUS_BUSY_MOVING);
    ASSERTEQUALS("", Serial.output().c_str());
    ASSERTEQUAL(0, digitalRead(PC2_Z_MIN_PIN));
    ASSERTQUAD(Quad<StepCoord>(1, 4, 7, 0), machine.stroke.position());
    ASSERTQUAD(Quad<StepCoord>(6, 9, 12, 5), machine.getMotorPosition()); // axis a NOPIN inactive
    reqAvail = jcmd.requestAvailable();
    resAvail = jcmd.responseAvailable();

    // finalize stroke
    testJSON_process(machine, jc, jcmd, replace,
                     "{'s':0,'r':{'systc':109,'dvs':{'us':128,'dp':[10,20],'x':10,'y':20,'z':0}}}\n",
                     STATUS_OK);
    ASSERTQUAD(Quad<StepCoord>(10, 20, 0, 0), machine.stroke.position());
    ASSERTQUAD(Quad<StepCoord>(15, 25, 5, 5), machine.getMotorPosition()); // axis a is NOPIN inactive
    ASSERTEQUAL(tStart + 3, threadClock.ticks);
    ASSERTEQUAL(reqAvail, jcmd.requestAvailable());
    ASSERTEQUAL(resAvail, jcmd.responseAvailable());

    // Test largest valid stroke
    string segs = "[-127";
    for (int i = 1; i < 50; i++) {
        segs += ",-127";
    }
    segs += "]";
    string jlarge = "{'dvs':{";
    jlarge += "'1':";
    jlarge += segs;
    jlarge += "}}";
    string jlargein = JT(jlarge.c_str());
    JsonCommand jcmd2;
    ASSERTEQUAL(STATUS_JSON_PARSE_ERROR, jcmd2.parse(jlargein.c_str()));
}

void test_JsonController() {
    cout << "TEST	: test_JsonController() =====" << endl;

    Machine machine;
    machine.enable(true);
    JsonController jc(machine);
    arduino.setPin(PC2_X_MIN_PIN, false);
    arduino.setPin(PC2_Y_MIN_PIN, false);
    arduino.setPin(PC2_Z_MIN_PIN, false);

    Serial.clear();
    machine.pDisplay->setStatus(DISPLAY_WAIT_IDLE);
    JsonCommand jcmd;
    ASSERTEQUAL(STATUS_BUSY_PARSED, jcmd.parse("{\"sys\":\"\"}"));
    threadClock.ticks = 12345;
    jc.process(jcmd);
    char sysbuf[500];
    const char *fmt = "{'s':%d,'r':{'sys':{'fr':1000,'jp':false,'lh':false,'lp':0,'pc':2,'tc':12345,'v':%.2f}}}\n";
    snprintf(sysbuf, sizeof(sysbuf), JT(fmt),
             STATUS_OK, VERSION_MAJOR * 100 + VERSION_MINOR + VERSION_PATCH / 100.0);
    ASSERTEQUALS(sysbuf, Serial.output().c_str());

    test_JsonController_axis(machine, jc, 'x');
    test_JsonController_axis(machine, jc, 'y');
    test_JsonController_axis(machine, jc, 'z');
    test_JsonController_axis(machine, jc, 'a');
    test_JsonController_axis(machine, jc, 'b');
    test_JsonController_axis(machine, jc, 'c');

    test_JsonController_motor(machine, jc, '1');
    test_JsonController_motor(machine, jc, '2');
    test_JsonController_motor(machine, jc, '3');
    test_JsonController_motor(machine, jc, '4');

    test_JsonController_machinePosition(machine, jc);

    test_JsonController_stroke(machine, jc);

    test_JsonController_tst();

    cout << "TEST	: test_JsonController() OK " << endl;
}

void test_Quad() {
    cout << "TEST	: test_Quad() =====" << endl;

    Quad<int16_t> q1(1, 2, 3, 4);
    Quad<int16_t> q2(2, 4, 6, 8);
    ASSERT(q1 + q1 == q2);
    ASSERT(q1 + q1 != q1);
    ASSERT(q1 != q2);
    q1 *= 2;
    ASSERT(q1 == q2);
    Quad<int16_t> q3(-3, -1, 0, 4);
    ASSERTQUAD(Quad<int16_t>(-1, -1, 0, 1), q3.sgn());
    ASSERTQUAD(Quad<int16_t>(-3, -1, 0, 4), q3);
    q3 -= Quad<int16_t>(-1, -1, 0, 1);
    ASSERTQUAD(Quad<int16_t>(-2, 0, 0, 3), q3);

    cout << "TEST	: test_Quad() OK " << endl;
}

class MockStepper : public QuadStepper {
    public:
        Quad<StepCoord> dPos;
        void clear() {
            dPos.clear();
        }
        virtual Status step(const Quad<StepCoord> &pulse) {
            dPos += pulse;
#ifdef TEST_TRACE
            cout << "	: MockStepper"
                 << " dPos:" << dPos.toString()
                 << " pulse:" << pulse.toString() << endl;
#endif
            return STATUS_OK;
        }
};

void test_Stroke() {
    cout << "TEST	: test_Stroke() =====" << endl;

    Stroke stroke;
    MockStepper stepper;
    Ticks tStart = 100000;
    ASSERTEQUAL(STATUS_STROKE_START, stroke.traverse(ticks(), stepper));
    stroke.append( Quad<StepDV>(1, 10, -1, -10) );
    stroke.append( Quad<StepDV>(1, 10, -1, -10) );
    stroke.append( Quad<StepDV>(-1, -10, 1, 10) );
    stroke.dEndPos = Quad<StepCoord>(4, 40, -4, -40);
    ASSERTEQUAL(0, stroke.getTotalTime()*TICKS_PER_SECOND);
    ASSERTEQUAL(STATUS_STROKE_TIME, stroke.start(tStart));

    stroke.setTotalTime(17/(float) TICKS_PER_SECOND); 
    ASSERTEQUAL(17, stroke.getTotalTime()*TICKS_PER_SECOND);

    stroke.dEndPos.value[0] += 100;
    ASSERTEQUAL(STATUS_STROKE_END_ERROR, stroke.start(tStart));
    stroke.dEndPos.value[0] -= 100;
    ASSERTEQUAL(17, stroke.getTotalTime()*TICKS_PER_SECOND);

    ASSERTEQUAL(0, stroke.goalStartTicks(tStart - 1));
    ASSERTEQUAL(0, stroke.goalEndTicks(tStart - 1));
    ASSERTEQUAL(0, stroke.goalStartTicks(0));
    ASSERTEQUAL(0, stroke.goalEndTicks(0));

    ASSERTEQUAL(0, stroke.goalStartTicks(tStart));
    ASSERTEQUAL(5, stroke.goalEndTicks(tStart));
    ASSERTEQUAL(0, stroke.goalStartTicks(tStart + 1));
    ASSERTEQUAL(5, stroke.goalEndTicks(tStart + 1));
    ASSERTEQUAL(0, stroke.goalStartTicks(tStart + 4));
    ASSERTEQUAL(5, stroke.goalEndTicks(tStart + 4));

    ASSERTEQUAL(5, stroke.goalStartTicks(tStart + 5));
    ASSERTEQUAL(11, stroke.goalEndTicks(tStart + 5));
    ASSERTEQUAL(5, stroke.goalStartTicks(tStart + 6));
    ASSERTEQUAL(11, stroke.goalEndTicks(tStart + 6));
    ASSERTEQUAL(5, stroke.goalStartTicks(tStart + 10));
    ASSERTEQUAL(11, stroke.goalEndTicks(tStart + 10));

    ASSERTEQUAL(11, stroke.goalStartTicks(tStart + 11));
    ASSERTEQUAL(17, stroke.goalEndTicks(tStart + 11));
    ASSERTEQUAL(11, stroke.goalStartTicks(tStart + 12));
    ASSERTEQUAL(17, stroke.goalEndTicks(tStart + 12));
    ASSERTEQUAL(11, stroke.goalStartTicks(tStart + 17));
    ASSERTEQUAL(17, stroke.goalEndTicks(tStart + 17));

    ASSERTEQUAL(11, stroke.goalStartTicks(tStart + 18));
    ASSERTEQUAL(17, stroke.goalEndTicks(tStart + 18));
    ASSERTEQUAL(11, stroke.goalStartTicks(tStart + 1000));
    ASSERTEQUAL(17, stroke.goalEndTicks(tStart + 1000));

    ASSERTEQUAL(STATUS_OK, stroke.start(tStart));
    ASSERTEQUAL(0, (long) stroke.goalSegment(0));
    ASSERTEQUAL(0, (long) stroke.goalSegment(tStart - 1));
    ASSERTEQUAL(0, (long) stroke.goalSegment(tStart));
    ASSERTEQUAL(0, (long) stroke.goalSegment(tStart + 1));
    ASSERTEQUAL(0, (long) stroke.goalSegment(tStart + 4));
    ASSERTEQUAL(1, (long) stroke.goalSegment(tStart + 5));
    ASSERTEQUAL(1, (long) stroke.goalSegment(tStart + 6));
    ASSERTEQUAL(1, (long) stroke.goalSegment(tStart + 10));
    ASSERTEQUAL(2, (long) stroke.goalSegment(tStart + 11));
    ASSERTEQUAL(2, (long) stroke.goalSegment(tStart + 12));
    ASSERTEQUAL(2, (long) stroke.goalSegment(tStart + 17));
    ASSERTEQUAL(2, (long) stroke.goalSegment(tStart + 18));
    ASSERTEQUAL(2, (long) stroke.goalSegment(tStart + 1000));

    // Test goalPos() and traverse()
    for (int t = 0; t < 20; t++) {
        Quad<StepCoord> pos = stroke.goalPos(tStart + t);
    }
    ASSERTQUAD(Quad<StepCoord>(4, 40, -4, -40), stroke.dEndPos);
    ASSERTQUAD(Quad<StepCoord>(4, 40, -4, -40), stroke.goalPos(tStart + 17));
    ASSERTQUAD(Quad<StepCoord>(3, 35, -3, -35), stroke.goalPos(tStart + 14));
    ASSERTQUAD(Quad<StepCoord>(3, 30, -3, -30), stroke.goalPos(tStart + 11));
    ASSERTQUAD(Quad<StepCoord>(2, 26, -2, -26), stroke.goalPos(tStart + 10));
    ASSERTQUAD(Quad<StepCoord>(2, 20, -2, -20), stroke.goalPos(tStart + 8));
    ASSERTQUAD(Quad<StepCoord>(2, 20, -2, -20), stroke.goalPos(tStart + 8));
    ASSERTQUAD(Quad<StepCoord>(1, 10, -1, -10), stroke.goalPos(tStart + 5));
    ASSERTQUAD(Quad<StepCoord>(0, 0, 0, 0), stroke.goalPos(tStart));
    ASSERTQUAD(Quad<StepCoord>(0, 0, 0, 0), stroke.position());
	ASSERTEQUAL(17, stroke.get_dtTotal());
    for (Ticks t = tStart; t < tStart + 20; t++) {
        //cout << "stroke	: traverse(" << t << ")" << endl;
        if (STATUS_OK == stroke.traverse(t, stepper)) {
            ASSERTEQUAL(17, t - tStart);
            Quad<StepCoord> dPos = stepper.dPos;
            ASSERTEQUAL(STATUS_OK, stroke.traverse(t, stepper)); 	// should do nothing
            ASSERTEQUAL(STATUS_OK, stroke.traverse(t + 1, stepper)); 	// should do nothing
            ASSERT(dPos == stepper.dPos);
            break;
        } else {
            Quad<StepCoord> dPos = stepper.dPos;
            Status status = stroke.traverse(t, stepper); // should do nothing
            ASSERT(status == STATUS_BUSY_MOVING || status == STATUS_OK);
            ASSERTQUAD(dPos, stepper.dPos);
        }
    }

    // Test scale
    stroke.scale = 3;
    ASSERTEQUAL(STATUS_STROKE_END_ERROR, stroke.start(tStart));
    stroke.dEndPos *= stroke.scale;
    ASSERTEQUAL(STATUS_OK, stroke.start(tStart));
    ASSERTEQUAL(STATUS_OK, stroke.start(tStart));
    ASSERT(Quad<StepCoord>(12, 120, -12, -120) == stroke.dEndPos);
    ASSERT(Quad<StepCoord>(12, 120, -12, -120) == stroke.goalPos(tStart + 17));
    ASSERT(Quad<StepCoord>(10, 105, -10, -105) == stroke.goalPos(tStart + 14));
    ASSERT(Quad<StepCoord>(9, 90, -9, -90) == stroke.goalPos(tStart + 11));
    ASSERT(Quad<StepCoord>(6, 60, -6, -60) == stroke.goalPos(tStart + 8));
    ASSERT(Quad<StepCoord>(3, 30, -3, -30) == stroke.goalPos(tStart + 5));
    ASSERT(Quad<StepCoord>(0, 0, 0, 0) == stroke.goalPos(tStart));
    stepper.clear();
    for (Ticks t = tStart; t < tStart + 20; t++) {
        //cout << "stroke	: traverse(" << t << ")" << endl;
        if (STATUS_OK == stroke.traverse(t, stepper)) {
            ASSERTEQUAL(17, t - tStart);
            Quad<StepCoord> dPos = stepper.dPos;
            ASSERTEQUAL(STATUS_OK, stroke.traverse(t, stepper)); 	// should do nothing
            ASSERTEQUAL(STATUS_OK, stroke.traverse(t + 1, stepper)); 	// should do nothing
            ASSERT(dPos == stepper.dPos);
            break;
        } else {
            Quad<StepCoord> dPos = stepper.dPos;
            Status status = stroke.traverse(t, stepper); // should do nothing
            ASSERT(status == STATUS_BUSY_MOVING || status == STATUS_OK);
            ASSERT(dPos == stepper.dPos);
        }
    }

    // Test end position not modulo scale
    stroke.dEndPos.value[0] += 1;
    stroke.dEndPos.value[1] += 2;
    stroke.dEndPos.value[2] += 1;
    stroke.dEndPos.value[3] += 102;
    ASSERTEQUAL(STATUS_STROKE_END_ERROR, stroke.start(tStart));
    stroke.dEndPos.value[3] -= 100;
    ASSERTEQUAL(STATUS_OK, stroke.start(tStart));
    ASSERTEQUAL(17, stroke.getTotalTime()*TICKS_PER_SECOND);
    ASSERTQUAD(Quad<StepCoord>(0, 0, 0, 0), stroke.position());
    ASSERTQUAD(Quad<StepCoord>(13, 122, -11, -118), stroke.dEndPos);
    ASSERTQUAD(Quad<StepCoord>(13, 122, -11, -118), stroke.goalPos(tStart + 17));
    ASSERT(Quad<StepCoord>(10, 105, -10, -105) == stroke.goalPos(tStart + 14));
    ASSERT(Quad<StepCoord>(9, 90, -9, -90) == stroke.goalPos(tStart + 11));
    ASSERT(Quad<StepCoord>(6, 60, -6, -60) == stroke.goalPos(tStart + 8));
    ASSERT(Quad<StepCoord>(3, 30, -3, -30) == stroke.goalPos(tStart + 5));
    ASSERT(Quad<StepCoord>(0, 0, 0, 0) == stroke.goalPos(tStart));

    stepper.clear();
    for (Ticks t = tStart; t < tStart + 20; t++) {
        //cout << "stroke	: traverse(" << t << ")" << endl;
        if (STATUS_OK == stroke.traverse(t, stepper)) {
            ASSERTEQUAL(17, t - tStart);
            Quad<StepCoord> dPos = stepper.dPos;
            ASSERTEQUAL(STATUS_OK, stroke.traverse(t, stepper)); 	// should do nothing
            ASSERTEQUAL(STATUS_OK, stroke.traverse(t + 1, stepper)); 	// should do nothing
            ASSERT(dPos == stepper.dPos);
            break;
        } else {
            Quad<StepCoord> dPos = stepper.dPos;
            Status status = stroke.traverse(t, stepper); // should do nothing
            ASSERT(status == STATUS_BUSY_MOVING || status == STATUS_OK);
            ASSERT(dPos == stepper.dPos);
        }
    }

    cout << "TEST	: test_Stroke() OK " << endl;
}

void test_Machine_step() {
    cout << "TEST	: test_Machine_step() =====" << endl;

    Machine machine;
    for (int i = 0; i < 4; i++) {
        arduino.setPin(machine.axis[i].pinMin, 0);
    }
    machine.enable(true);

    machine.axis[0].travelMax = 5;
    machine.axis[1].travelMax = 4;
    machine.axis[2].travelMax = 3;
    machine.axis[3].travelMax = 2;
    ASSERTEQUAL(false, machine.axis[0].atMin);
    ASSERTEQUAL(false, machine.axis[1].atMin);
    ASSERTEQUAL(false, machine.axis[2].atMin);
    ASSERTEQUAL(false, machine.axis[3].atMin);
    ASSERTEQUAL(OUTPUT, arduino.getPinMode(machine.axis[0].pinEnable));
    ASSERTEQUAL(LOW, arduino.getPin(machine.axis[0].pinEnable));
    ASSERTEQUAL(OUTPUT, arduino.getPinMode(machine.axis[1].pinEnable));
    ASSERTEQUAL(LOW, arduino.getPin(machine.axis[1].pinEnable));
    ASSERTEQUAL(OUTPUT, arduino.getPinMode(machine.axis[2].pinEnable));
    ASSERTEQUAL(LOW, arduino.getPin(machine.axis[2].pinEnable));
    ASSERTEQUAL(true, machine.axis[0].isEnabled());
    ASSERTEQUAL(true, machine.axis[1].isEnabled());
    ASSERTEQUAL(true, machine.axis[2].isEnabled());
    ASSERTEQUAL(false, machine.axis[3].isEnabled());
    ASSERTEQUAL(false, machine.axis[4].isEnabled());
    ASSERTEQUAL(false, machine.axis[5].isEnabled());

    Status status;
    ASSERTQUAD(machine.getMotorPosition(), Quad<StepCoord>(0, 0, 0, 0));
    ASSERTEQUAL(STATUS_STEP_RANGE_ERROR, machine.step(Quad<StepCoord>(4, 3, 2, 1)));
    ASSERTEQUAL(STATUS_AXIS_DISABLED, machine.step(Quad<StepCoord>(1, 1, 1, 1)));
    ASSERTQUAD(machine.getMotorPosition(), Quad<StepCoord>(0, 0, 0, 0));

    // Test travelMax
    ASSERTEQUAL(STATUS_OK, machine.step(Quad<StepCoord>(1, 0, 0, 0)));
    ASSERTEQUAL(false, machine.axis[3].atMin);
    ASSERT(machine.getMotorPosition() == Quad<StepCoord>(1, 0, 0, 0));
    ASSERTEQUAL(STATUS_OK, machine.step(Quad<StepCoord>(1, 1, 0, 0)));
    ASSERTEQUAL(false, machine.axis[3].atMin);
    ASSERT(machine.getMotorPosition() == Quad<StepCoord>(2, 1, 0, 0));
    ASSERTEQUAL(STATUS_OK, machine.step(Quad<StepCoord>(1, 1, 1, 0)));
    ASSERTEQUAL(false, machine.axis[3].atMin);
    ASSERTQUAD(machine.getMotorPosition(), Quad<StepCoord>(3, 2, 1, 0));
    ASSERTEQUAL(STATUS_AXIS_DISABLED, machine.step(Quad<StepCoord>(1, 1, 1, 1)));
    ASSERTQUAD(machine.getMotorPosition(), Quad<StepCoord>(3, 2, 1, 0));
    ASSERTEQUAL(STATUS_OK, machine.step(Quad<StepCoord>(1, 1, 1, 0)));
    ASSERTEQUAL(false, machine.axis[3].atMin);
    ASSERTQUAD(Quad<StepCoord>(4, 3, 2, 0), machine.getMotorPosition());
    ASSERTEQUAL(STATUS_OK, machine.step(Quad<StepCoord>(1, 1, 1, 0)));
    ASSERTEQUAL(false, machine.axis[3].atMin);
    ASSERTQUAD(machine.getMotorPosition(), Quad<StepCoord>(5, 4, 3, 0));
    ASSERTEQUAL(false, machine.axis[0].atMin);
    ASSERTEQUAL(false, machine.axis[1].atMin);
    ASSERTEQUAL(false, machine.axis[2].atMin);
    ASSERTEQUAL(false, machine.axis[3].atMin);
    ASSERTEQUAL(STATUS_TRAVEL_MAX, machine.step(Quad<StepCoord>(1, 1, 1, 0)));
    ASSERTQUAD(Quad<StepCoord>(5, 4, 3, 0), machine.getMotorPosition());

    // Test travelMin
    ASSERTEQUAL(false, machine.axis[0].atMin);
    ASSERTEQUAL(false, machine.axis[1].atMin);
    ASSERTEQUAL(false, machine.axis[2].atMin);
    ASSERTEQUAL(false, machine.axis[3].atMin);
    ASSERTEQUAL(STATUS_OK, machine.step(Quad<StepCoord>(-1, -1, -1, 0)));
    ASSERTQUAD(Quad<StepCoord>(4, 3, 2, 0), machine.getMotorPosition());
    ASSERTEQUAL(STATUS_OK, machine.step(Quad<StepCoord>(-1, -1, -1, 0)));
    ASSERTQUAD(Quad<StepCoord>(3, 2, 1, 0), machine.getMotorPosition());
    ASSERTEQUAL(STATUS_OK, machine.step(Quad<StepCoord>(-1, -1, -1, 0)));
    ASSERTQUAD(Quad<StepCoord>(2, 1, 0, 0), machine.getMotorPosition());
    ASSERTEQUAL(STATUS_TRAVEL_MIN, machine.step(Quad<StepCoord>(-1, -1, -1, 0)));
    ASSERTQUAD(Quad<StepCoord>(2, 1, 0, 0), machine.getMotorPosition());

    // Test atMin
    arduino.setPin(machine.axis[0].pinMin, 1);
    machine.axis[0].travelMin = -10;
    machine.axis[1].travelMin = -10;
    machine.axis[2].travelMin = -10;
    ASSERTEQUAL(STATUS_LIMIT_MIN, machine.step(Quad<StepCoord>(-1, -1, -1, 0)));
    ASSERTQUAD(Quad<StepCoord>(2, 1, 0, 0), machine.getMotorPosition());

    cout << "TEST	: test_Machine_step() OK " << endl;
}

void test_PinConfig() {
    cout << "TEST	: test_PinConfig() =====" << endl;

    MachineThread mt = test_setup();
    Machine &machine = mt.machine;
	ASSERTEQUAL(NOVALUE, arduino.getPinMode(PC1_X_STEP_PIN));

    threadClock.ticks++;
    Serial.push(JT("{'syspc':1}\n"));
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);

    threadClock.ticks++;
    mt.loop();
    ASSERTEQUAL(STATUS_OK, mt.status);
	ASSERTEQUAL(PC1_X_STEP_PIN, machine.axis[0].pinStep);
	ASSERTEQUAL(OUTPUT, arduino.getPinMode(PC1_X_STEP_PIN));
	ASSERTEQUAL(PC1_Y_STEP_PIN, machine.axis[1].pinStep);
	ASSERTEQUAL(OUTPUT, arduino.getPinMode(PC1_Y_STEP_PIN));
	ASSERTEQUAL(PC1_Z_STEP_PIN, machine.axis[2].pinStep);
	ASSERTEQUAL(OUTPUT, arduino.getPinMode(PC1_Z_STEP_PIN));
	ASSERTEQUAL(PC1_X_DIR_PIN, machine.axis[0].pinDir);
	ASSERTEQUAL(PC1_Y_DIR_PIN, machine.axis[1].pinDir);
	ASSERTEQUAL(PC1_Z_DIR_PIN, machine.axis[2].pinDir);
	ASSERTEQUAL(PC1_X_ENABLE_PIN, machine.axis[0].pinEnable);
	ASSERTEQUAL(PC1_Y_ENABLE_PIN, machine.axis[1].pinEnable);
	ASSERTEQUAL(PC1_Z_ENABLE_PIN, machine.axis[2].pinEnable);
	ASSERTEQUAL(PC1_X_MIN_PIN, machine.axis[0].pinMin);
	ASSERTEQUAL(PC1_Y_MIN_PIN, machine.axis[1].pinMin);
	ASSERTEQUAL(PC1_Z_MIN_PIN, machine.axis[2].pinMin);
	ASSERTEQUAL(PC1_EMC01, machine.getPinConfig());

    cout << "TEST	: test_PinConfig() OK " << endl;
}

void test_Move() {
    cout << "TEST	: test_Move() =====" << endl;

    MachineThread mt = test_setup();
    Machine &machine = mt.machine;
    int32_t xpulses = arduino.pulses(PC2_X_STEP_PIN);
    int32_t ypulses = arduino.pulses(PC2_Y_STEP_PIN);
    int32_t zpulses = arduino.pulses(PC2_Z_STEP_PIN);
    int32_t usStart = delayMicsTotal;

    Status status = machine.moveTo(Quad<StepCoord>(1, 10, 100, 0), 1);
    ASSERTEQUAL(STATUS_OK, status);
    ASSERTEQUAL(xpulses + 1, arduino.pulses(PC2_X_STEP_PIN));
    ASSERTEQUAL(ypulses + 10, arduino.pulses(PC2_Y_STEP_PIN));
    ASSERTEQUAL(zpulses + 100, arduino.pulses(PC2_Z_STEP_PIN));
    ASSERTQUAD(Quad<StepCoord>(1, 10, 100, 0), machine.getMotorPosition());
    ASSERTEQUALT(usStart + 1000000, delayMicsTotal, 20000);

    usStart = delayMicsTotal;
    xpulses = arduino.pulses(PC2_X_STEP_PIN);
    ypulses = arduino.pulses(PC2_Y_STEP_PIN);
    zpulses = arduino.pulses(PC2_Z_STEP_PIN);
    machine.setMotorPosition(Quad<StepCoord>());

    threadClock.ticks++;
    Serial.push(JT("{'mov':{'1':1,'2':10,'3':100,'sr':1}}\n"));
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);

    threadClock.ticks++;
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);

    threadClock.ticks++;
    mt.loop();
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUAL(xpulses + 1, arduino.pulses(PC2_X_STEP_PIN));
    ASSERTEQUAL(ypulses + 10, arduino.pulses(PC2_Y_STEP_PIN));
    ASSERTEQUAL(zpulses + 100, arduino.pulses(PC2_Z_STEP_PIN));
    ASSERTQUAD(Quad<StepCoord>(1, 10, 100, 0), machine.getMotorPosition());
    ASSERTEQUALS(JT("{'s':0,'r':{'mov':{'1':1,'2':10,'3':100,'sr':1}}}\n"), Serial.output().c_str());
    ASSERTEQUALT(usStart + 1000000, delayMicsTotal, 20000);

    threadClock.ticks++;
    mt.loop();
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

    usStart = delayMicsTotal;
    xpulses = arduino.pulses(PC2_X_STEP_PIN);
    ypulses = arduino.pulses(PC2_Y_STEP_PIN);
    zpulses = arduino.pulses(PC2_Z_STEP_PIN);
    machine.setMotorPosition(Quad<StepCoord>());

    threadClock.ticks++;
    Serial.push(JT("{'mov':{'x':1,'y':10,'z':100}}\n"));
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);

    threadClock.ticks++;
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);

    threadClock.ticks++;
    mt.loop();
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUAL(xpulses + 1, arduino.pulses(PC2_X_STEP_PIN));
    ASSERTEQUAL(ypulses + 10, arduino.pulses(PC2_Y_STEP_PIN));
    ASSERTEQUAL(zpulses + 100, arduino.pulses(PC2_Z_STEP_PIN));
    ASSERTQUAD(Quad<StepCoord>(1, 10, 100, 0), machine.getMotorPosition());
    ASSERTEQUALS(JT("{'s':0,'r':{'mov':{'x':1,'y':10,'z':100}}}\n"), Serial.output().c_str());
    ASSERTEQUALT(usStart, delayMicsTotal, 20000);

    cout << "TEST	: test_Move() OK " << endl;
}

void test_dvs() {
    cout << "TEST	: test_dvs() =====" << endl;

    MachineThread mt = test_setup();
    Machine &machine = mt.machine;
    int32_t xpulses = arduino.pulses(PC2_X_STEP_PIN);
	machine.setMotorPosition(Quad<StepCoord>(100,100,100,100));

    Serial.push(JT("{'dvs':{'us':5000000,'x':[10,0,0,0,0]}}\n"));
    test_ticks(1); // parse
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    ASSERTEQUAL(xpulses, arduino.pulses(PC2_X_STEP_PIN));

    test_ticks(1); // initialize
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    ASSERTEQUAL(xpulses, arduino.pulses(PC2_X_STEP_PIN));
	ASSERTEQUAL(0, threadClock.ticks - machine.stroke.tStart);

    test_ticks(1); // start moving
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    ASSERTEQUAL(xpulses, arduino.pulses(PC2_X_STEP_PIN));

    test_ticks(MS_TICKS(100)); // moving
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    ASSERTEQUAL(1, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
	ASSERTEQUAL(MS_TICKS(100)+3, threadClock.ticks - machine.stroke.tStart);

    test_ticks(MS_TICKS(100)); // moving
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    ASSERTEQUAL(2, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
	ASSERTEQUAL(MS_TICKS(200)+3, threadClock.ticks - machine.stroke.tStart);

    test_ticks(MS_TICKS(100)); // moving
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    ASSERTEQUAL(3, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
	ASSERTEQUAL(MS_TICKS(300)+4, threadClock.ticks - machine.stroke.tStart);

    test_ticks(MS_TICKS(100)); // moving
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    ASSERTEQUAL(4, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
	ASSERTEQUAL(MS_TICKS(400)+4, threadClock.ticks - machine.stroke.tStart);

    test_ticks(MS_TICKS(500) - 4*MS_TICKS(100)); // moving
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    ASSERTEQUAL(5, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
	ASSERTEQUAL(MS_TICKS(500)+7, threadClock.ticks - machine.stroke.tStart);

    test_ticks(MS_TICKS(1000)-MS_TICKS(500)); // moving
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    ASSERTEQUAL(10, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
	ASSERTEQUAL(MS_TICKS(1000)+8, threadClock.ticks - machine.stroke.tStart);
	ASSERTQUAD(Quad<StepCoord>(110,100,100,100), machine.getMotorPosition());

    test_ticks(MS_TICKS(500)); // moving
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    ASSERTEQUAL(15, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
	ASSERTEQUAL(MS_TICKS(1500)+9, threadClock.ticks - machine.stroke.tStart);

    test_ticks(MS_TICKS(1000)-MS_TICKS(500)); // moving
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    ASSERTEQUAL(20, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
	ASSERTEQUAL(MS_TICKS(2000)+10, threadClock.ticks - machine.stroke.tStart);
	ASSERTQUAD(Quad<StepCoord>(120,100,100,100), machine.getMotorPosition());

    test_ticks(MS_TICKS(1000)); // moving
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    ASSERTEQUAL(30, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
	ASSERTEQUAL(MS_TICKS(3000)+11, threadClock.ticks - machine.stroke.tStart);
	ASSERTQUAD(Quad<StepCoord>(130,100,100,100), machine.getMotorPosition());

    test_ticks(MS_TICKS(500)); // moving
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    ASSERTEQUAL(35, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
	ASSERTEQUAL(MS_TICKS(3500)+12, threadClock.ticks - machine.stroke.tStart);

    test_ticks(MS_TICKS(600)); // moving
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    ASSERTEQUAL(41, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
	ASSERTEQUAL(MS_TICKS(4100)+13, threadClock.ticks - machine.stroke.tStart);
	ASSERTQUAD(Quad<StepCoord>(141,100,100,100), machine.getMotorPosition());

    test_ticks(MS_TICKS(600)); // moving
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
	ASSERTEQUAL(MS_TICKS(4700)+14, threadClock.ticks - machine.stroke.tStart);
	ASSERTQUAD(Quad<StepCoord>(147,100,100,100), machine.getMotorPosition());
    ASSERTEQUAL(47, arduino.pulses(PC2_X_STEP_PIN)-xpulses);

    test_ticks(MS_TICKS(1000)); // done
    ASSERTEQUAL(STATUS_OK, mt.status);
	ASSERTEQUAL(MS_TICKS(5700)+15, threadClock.ticks - machine.stroke.tStart);
	ASSERTEQUALS(JT("{'s':0,'r':{'dvs':{'us':5000000,'x':50}}}\n"), Serial.output().c_str());
    ASSERTEQUAL(50, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
	ASSERTQUAD(Quad<StepCoord>(150,100,100,100), machine.getMotorPosition());

    cout << "TEST	: test_dvs() OK " << endl;
}

void test_error(MachineThread &mt, const char * cmd, Status status, const char *output=NULL) {
    Serial.push(JT(cmd));
    test_ticks(1); // parse
	if (mt.status == STATUS_BUSY_PARSED) {
		test_ticks(1); // initialize
	}
	if (output) {
		ASSERTEQUALS(JT(output), Serial.output().c_str());
	}
	ASSERTEQUAL(status, mt.status);
}

void test_errors() {
    cout << "TEST	: test_errors() =====" << endl;

    MachineThread mt = test_setup();
    Machine &machine = mt.machine;

    test_error(mt, "{'abc':true}\n", STATUS_UNRECOGNIZED_NAME, "{'s':-402,'r':{'abc':true},'e':'abc'}\n");
    test_error(mt, "{bad-json}\n", STATUS_JSON_PARSE_ERROR, "{'s':-403}\n");
    test_error(mt, "bad-json\n", STATUS_JSON_PARSE_ERROR, "{'s':-403}\n");
    test_error(mt, "{'xud':50000}\n", STATUS_VALUE_RANGE, "{'s':-133,'r':{'xud':50000}}\n");

    cout << "TEST	: test_errors() OK " << endl;
}

void test_Idle() {
    cout << "TEST	: test_Idle() =====" << endl;

    MachineThread mt = test_setup();
    Machine &machine = mt.machine;
    int32_t xenpulses = arduino.pulses(PC2_X_ENABLE_PIN);

    threadClock.ticks++;
    mt.loop(); // parse
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);
    ASSERTEQUAL(xenpulses + 1, arduino.pulses(PC2_X_ENABLE_PIN));

    threadClock.ticks++;
    mt.loop(); // parse
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);
    ASSERTEQUAL(xenpulses + 2, arduino.pulses(PC2_X_ENABLE_PIN));

    threadClock.ticks++;
    mt.loop(); // parse
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);
    ASSERTEQUAL(xenpulses + 3, arduino.pulses(PC2_X_ENABLE_PIN));

    cout << "TEST	: test_Idle() OK " << endl;
}

void test_PrettyPrint() {
    cout << "TEST	: test_PrettyPrint() =====" << endl;

    MachineThread mt = test_setup();
    Machine &machine = mt.machine;

    Serial.push(JT("{'sysjp':true}\n"));
    threadClock.ticks++;
    mt.loop(); // parse
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    threadClock.ticks++;
    mt.loop(); // process
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUALS(JT("{\r\n  's': 0,\r\n  'r': {\r\n    'sysjp': true\r\n  }\r\n}\n"),
                 Serial.output().c_str());

    cout << "TEST	: test_PrettyPrint() OK " << endl;
}

void test_Home() {
    cout << "TEST	: test_Home() =====" << endl;

    MachineThread mt = test_setup();
    Machine &machine = mt.machine;
    int32_t xpulses = arduino.pulses(PC2_X_STEP_PIN);
    int32_t ypulses = arduino.pulses(PC2_Y_STEP_PIN);
    int32_t zpulses = arduino.pulses(PC2_Z_STEP_PIN);
    machine.axis[0].home = 5;
    machine.axis[1].home = 10;
    machine.axis[2].home = 15;
    machine.axis[3].home = 20;
    machine.setMotorPosition(Quad<StepCoord>(100, 100, 100, 100));

	// TEST LONG FORM
    threadClock.ticks++;
    Serial.push(JT("{'ho':{'x':'','z':20}}\n"));
    mt.loop();	// parse
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    ASSERTQUAD(Quad<StepCoord>(100, 100, 100, 100), mt.machine.getMotorPosition());

    threadClock.ticks++;
    mt.loop(); // initializing
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    ASSERT(machine.motorAxis[0]->homing);
    ASSERT(!machine.motorAxis[1]->homing);
    ASSERT(machine.motorAxis[2]->homing);
    ASSERT(!machine.motorAxis[3]->homing);
    ASSERTEQUAL(xpulses, arduino.pulses(PC2_X_STEP_PIN));
    ASSERTEQUAL(ypulses, arduino.pulses(PC2_Y_STEP_PIN));
    ASSERTEQUAL(zpulses, arduino.pulses(PC2_Z_STEP_PIN));
    ASSERTQUAD(Quad<StepCoord>(100, 100, 100, 100), mt.machine.getMotorPosition());
    ASSERTEQUALS("", Serial.output().c_str());

    threadClock.ticks++;
    mt.loop(); // moving
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    ASSERTEQUALS("", Serial.output().c_str());
    ASSERT(machine.motorAxis[0]->homing);
    ASSERT(!machine.motorAxis[1]->homing);
    ASSERT(machine.motorAxis[2]->homing);
    ASSERT(!machine.motorAxis[3]->homing);
    ASSERTEQUAL(xpulses + 32*1, arduino.pulses(PC2_X_STEP_PIN));
    ASSERTEQUAL(ypulses, arduino.pulses(PC2_Y_STEP_PIN));
    ASSERTEQUAL(zpulses + 32*1, arduino.pulses(PC2_Z_STEP_PIN));
    ASSERTQUAD(Quad<StepCoord>(5, 100, 20, 100), mt.machine.getMotorPosition());
    ASSERTEQUALS("", Serial.output().c_str());

    threadClock.ticks++;
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    ASSERT(machine.motorAxis[0]->homing);
    ASSERT(!machine.motorAxis[1]->homing);
    ASSERT(machine.motorAxis[2]->homing);
    ASSERT(!machine.motorAxis[3]->homing);
    ASSERTEQUAL(xpulses + 32*2, arduino.pulses(PC2_X_STEP_PIN));
    ASSERTEQUAL(ypulses, arduino.pulses(PC2_Y_STEP_PIN));
    ASSERTEQUAL(zpulses + 32*2, arduino.pulses(PC2_Z_STEP_PIN));
    ASSERTQUAD(Quad<StepCoord>(5, 100, 20, 100), mt.machine.getMotorPosition());
    ASSERTEQUAL(true, machine.axis[0].homing);
    ASSERTEQUAL(false, machine.axis[1].homing);
    ASSERTEQUAL(true, machine.axis[2].homing);
    ASSERTEQUAL(LOW, arduino.getPin(PC2_X_DIR_PIN));
    ASSERTEQUAL(LOW, arduino.getPin(PC2_Y_DIR_PIN));
    ASSERTEQUAL(LOW, arduino.getPin(PC2_Z_DIR_PIN));
    ASSERTEQUALS("", Serial.output().c_str());

    arduino.setPin(PC2_X_MIN_PIN, HIGH);
    threadClock.ticks++;
    mt.loop(); // hit first limit switch
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    ASSERTEQUAL(xpulses + 32*2 + MICROSTEPS_DEFAULT, arduino.pulses(PC2_X_STEP_PIN));
    ASSERTEQUAL(ypulses, arduino.pulses(PC2_Y_STEP_PIN));
    ASSERTEQUAL(zpulses + 32*3, arduino.pulses(PC2_Z_STEP_PIN));
    ASSERTQUAD(Quad<StepCoord>(5, 100, 20, 100), mt.machine.getMotorPosition());
    ASSERTEQUAL(false, machine.axis[0].homing);
    ASSERTEQUAL(false, machine.axis[1].homing);
    ASSERTEQUAL(true, machine.axis[2].homing);
    ASSERTEQUAL(HIGH, arduino.getPin(PC2_X_DIR_PIN)); // HIGH because we backed off
    ASSERTEQUAL(LOW, arduino.getPin(PC2_Y_DIR_PIN));
    ASSERTEQUAL(LOW, arduino.getPin(PC2_Z_DIR_PIN));
    ASSERTEQUALS("", Serial.output().c_str());

    arduino.setPin(PC2_Z_MIN_PIN, HIGH);
    threadClock.ticks++;
    mt.loop(); // hit final limit switch
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUAL(xpulses + 32*2 + MICROSTEPS_DEFAULT, arduino.pulses(PC2_X_STEP_PIN));
    ASSERTEQUAL(ypulses, arduino.pulses(PC2_Y_STEP_PIN));
    ASSERTEQUAL(zpulses + 32*3 + MICROSTEPS_DEFAULT, arduino.pulses(PC2_Z_STEP_PIN));
    ASSERTQUAD(Quad<StepCoord>(5, 100, 20, 100), mt.machine.getMotorPosition());
    ASSERTEQUAL(false, machine.axis[0].homing);
    ASSERTEQUAL(false, machine.axis[1].homing);
    ASSERTEQUAL(false, machine.axis[2].homing);
    ASSERTEQUAL(HIGH, arduino.getPin(PC2_X_DIR_PIN)); // HIGH because we backed off
    ASSERTEQUAL(LOW, arduino.getPin(PC2_Y_DIR_PIN));
    ASSERTEQUAL(HIGH, arduino.getPin(PC2_Z_DIR_PIN)); // HIGH because we backed off
    ASSERTEQUALS(JT("{'s':0,'r':{'ho':{'x':5,'z':20}}}\n"), Serial.output().c_str());

	// TEST ONE-AXIS SHORT FORM
    xpulses = arduino.pulses(PC2_X_STEP_PIN);
    machine.setMotorPosition(Quad<StepCoord>(100, 100, 100, 100));
    arduino.setPin(PC2_X_MIN_PIN, LOW);
    threadClock.ticks++;
    mt.loop(); // ready for next command
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

    threadClock.ticks++;
    Serial.push(JT("{'hox':''}\n"));
    mt.loop();	// parse
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);

    threadClock.ticks++;
    mt.loop(); // initializing
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    ASSERTEQUAL(xpulses, arduino.pulses(PC2_X_STEP_PIN));
    ASSERTEQUALS("", Serial.output().c_str());

    threadClock.ticks++;
    mt.loop(); // moving
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    ASSERTEQUALS("", Serial.output().c_str());

    arduino.setPin(PC2_X_MIN_PIN, HIGH);
    threadClock.ticks++;
    mt.loop(); // hit limit switch
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUAL(xpulses + 32*1 + MICROSTEPS_DEFAULT, arduino.pulses(PC2_X_STEP_PIN));
    ASSERTQUAD(Quad<StepCoord>(5, 100, 100, 100), mt.machine.getMotorPosition());
    ASSERTEQUALS(JT("{'s':0,'r':{'hox':5}}\n"), Serial.output().c_str());

	// TEST SHORT FORM
    xpulses = arduino.pulses(PC2_X_STEP_PIN);
	machine.axis[1].enabled = false;
	machine.axis[2].enabled = false;
    machine.setMotorPosition(Quad<StepCoord>(100, 100, 100, 100));
    arduino.setPin(PC2_X_MIN_PIN, LOW);
    threadClock.ticks++;
    mt.loop(); // ready for next command
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

    threadClock.ticks++;
    Serial.push(JT("{'ho':''}\n"));
    mt.loop();	// parse
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);

    threadClock.ticks++;
    mt.loop(); // initializing
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    ASSERTEQUAL(xpulses, arduino.pulses(PC2_X_STEP_PIN));
    ASSERTEQUALS("", Serial.output().c_str());

    threadClock.ticks++;
    mt.loop(); // moving
    ASSERTEQUALS("", Serial.output().c_str());
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);

    arduino.setPin(PC2_X_MIN_PIN, HIGH);
    threadClock.ticks++;
    mt.loop(); // hit limit switch
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUAL(xpulses + 32*1 + MICROSTEPS_DEFAULT, arduino.pulses(PC2_X_STEP_PIN));
    ASSERTQUAD(Quad<StepCoord>(5, 100, 100, 100), mt.machine.getMotorPosition());
    ASSERTEQUALS(JT("{'s':0,'r':{'ho':{'1':5,'2':100,'3':100,'4':100}}}\n"), Serial.output().c_str());

    cout << "TEST	: test_Home() OK " << endl;
}

void test_MachineThread() {
    cout << "TEST	: test_MachineThread() =====" << endl;

	threadRunner.clear();
    MachineThread mt;
    ASSERTQUAD(Quad<StepCoord>(0, 0, 0, 0), mt.machine.getMotorPosition());

    Serial.clear();
    Serial.push("{");
    TCNT1 = 100; ticks(); mt.loop();
    ASSERTEQUAL(STATUS_WAIT_EOL, mt.status);
    ASSERTEQUALS("", Serial.output().c_str());

    const char *jsonIn = "'systc':'','xen':true,'yen':true,'zen':true,'aen':true}\n";
    Serial.push(JT(jsonIn));
	arduino.timer1(1); ticks(); mt.loop();
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    ASSERTEQUALS("", Serial.output().c_str());

	arduino.timer1(1); ticks(); mt.loop();
    ASSERTEQUAL(STATUS_OK, mt.status);
    const char *jsonOut =
        "{'s':0,'r':{'systc':105,'xen':true,'yen':true,'zen':true,'aen':false}}\n";
    ASSERTEQUALS(JT(jsonOut), Serial.output().c_str());

	arduino.timer1(1); ticks(); mt.loop();
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

    ASSERTEQUALS("", Serial.output().c_str());
    jsonIn = "{'systc':'','dvs':{'us':123,'dp':[10,20],'1':[1,2],'2':[4,5],'3':[7,8]}}\n";
    Serial.push(JT(jsonIn));
	arduino.timer1(1); ticks(); mt.loop();
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    ASSERTEQUALS("", Serial.output().c_str());

	arduino.timer1(1); ticks(); mt.loop();
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    ASSERTEQUALS("", Serial.output().c_str());

    Serial.clear();
	arduino.timer1(1); ticks(); mt.loop();
    ASSERTEQUAL(STATUS_OK, mt.status);
    jsonOut =
        "{'s':0,'r':{'systc':114,'dvs':{'us':123,'dp':[10,20],'1':10,'2':20,'3':0}}}\n";
    ASSERTEQUALS(JT(jsonOut), Serial.output().c_str());
    ASSERTQUAD(Quad<StepCoord>(10, 20, 0, 0), mt.machine.getMotorPosition());

    cout << "TEST	: test_MachineThread() OK " << endl;
}

void test_DisplayPersistence(MachineThread &mt, DisplayStatus dispStatus, Status expectedStatus) {
    // Send partial serial command
    threadClock.ticks++;
    char jsonIn[128];
    snprintf(jsonIn, sizeof(jsonIn), JT("{'dpyds':%d}"), dispStatus);
    Serial.push(jsonIn);
    mt.loop();
    ASSERTEQUAL(STATUS_WAIT_EOL, mt.status);
    ASSERTEQUALS("status:11 level:127", testDisplay.message);

    // Send EOL to complete serial command
    threadClock.ticks++;
    Serial.push(JT("\n"));
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    ASSERTEQUALS("status:30 level:127", testDisplay.message);

    // Process command
    threadClock.ticks++;
    mt.loop();
    ASSERTEQUAL(expectedStatus, mt.status);
    char expectedDisplay[100];
    snprintf(expectedDisplay, sizeof(expectedDisplay), "status:%d level:127", dispStatus);
    ASSERTEQUALS(expectedDisplay, testDisplay.message);

    // Verify display persistence
    threadClock.ticks++;
    mt.loop();
    ASSERTEQUAL(expectedStatus, mt.status);
    ASSERTEQUALS(expectedDisplay, testDisplay.message);

}

void test_Display() {
    cout << "TEST	: test_Display() =====" << endl;

    pThreadList = NULL;
    threadRunner.setup();
    MachineThread mt;
    Serial.clear();
    testDisplay.clear();
    ASSERTEQUALS("", testDisplay.message);

    mt.machine.pDisplay = &testDisplay;
    mt.setup();
    ASSERTEQUAL(STATUS_BUSY_SETUP, mt.status);
    ASSERTEQUALS("status:30 level:127", testDisplay.message);

    mt.loop();
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);
    ASSERTEQUALS("status:10 level:127", testDisplay.message);

    test_DisplayPersistence(mt, DISPLAY_WAIT_OPERATOR, STATUS_WAIT_OPERATOR);
    test_DisplayPersistence(mt, DISPLAY_WAIT_CAMERA, STATUS_WAIT_CAMERA);
    test_DisplayPersistence(mt, DISPLAY_WAIT_ERROR, STATUS_WAIT_ERROR);

    testJSON(mt.machine, mt.controller, "'\"",
             "{'dpycr':10,'dpycg':20,'dpycb':30,'dpyds':12,'dpydl':255}",
             "{'s':0,'r':{'dpycr':10,'dpycg':20,'dpycb':30,'dpyds':12,'dpydl':255}}\n");
    testJSON(mt.machine, mt.controller, "'\"",
             "{'dpy':{'ds':30,'dl':255,'cr':1,'cg':2,'cb':3}}",
             "{'s':25,'r':{'dpy':{'ds':30,'dl':255,'cr':1,'cg':2,'cb':3}}}\n", STATUS_WAIT_BUSY);

    cout << "TEST	: test_Display() OK " << endl;
}

void test_ph5() {
    cout << "TEST	: test_ph5() =====" << endl;

    MachineThread mt = test_setup();
    Machine &machine = mt.machine;
	arduino.timer1(1);
	StrokeBuilder sb;
	ASSERTQUAD(Quad<StepCoord>(), machine.getMotorPosition());
	Status status = sb.buildLine(machine.stroke, Quad<StepCoord>(6400,3200,1600,0));
	ASSERTEQUAL(STATUS_OK, status);
	ASSERTEQUAL(50, machine.stroke.length);
	ASSERTEQUAL(0, machine.stroke.seg[0].value[0]);
	ASSERTEQUAL(1, machine.stroke.seg[1].value[0]);
	ASSERTEQUAL(1, machine.stroke.seg[2].value[0]);
	ASSERTEQUAL(4, machine.stroke.seg[3].value[0]);
	ASSERTEQUAL(5, machine.stroke.seg[4].value[0]);
	ASSERTEQUAL(8, machine.stroke.seg[5].value[0]);
	int32_t xpulses = arduino.pulses(PC2_X_STEP_PIN);
	StepCoord xpos = machine.axis[0].position;

	machine.stroke.start(ticks());

	int i = 0;
	status =  machine.stroke.traverse(ticks(), machine);
	ASSERTEQUAL(STATUS_BUSY_MOVING, status);
	status =  machine.stroke.traverse(ticks(), machine);
	ASSERTEQUAL(STATUS_BUSY_MOVING, status);
	do {
		status =  machine.stroke.traverse(ticks(), machine);
		StepCoord xposnew = machine.axis[0].position;
		ASSERT(xposnew >= xpos);
		xpos = xposnew;
		i++;
	} while (status == STATUS_BUSY_MOVING);
	ASSERTEQUAL(15623,i);
	ASSERTEQUAL(STATUS_OK, status);
	ASSERTQUAD(Quad<StepCoord>(6400, 3200, 1600, 0), machine.getMotorPosition());
    ASSERTEQUAL(6400, arduino.pulses(PC2_X_STEP_PIN)-xpulses);

	// TEST: short line
	xpulses = arduino.pulses(PC2_X_STEP_PIN);
	int32_t xdirpulses = arduino.pulses(PC2_X_DIR_PIN);
    ASSERTEQUAL(0, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    Serial.push(JT("{'tstph':{'pu':3200,'tv':'','sg':'','mv':'','lp':''}}\n")); 
    mt.loop();	// command.parse
	ASSERTEQUAL(true, machine.stroke.isDone());
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    ASSERTEQUAL(0, Serial.available()); // expected parse
    ASSERTEQUAL(0, arduino.pulses(PC2_X_STEP_PIN)-xpulses);

	mt.loop();	// command.process
	ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
	ASSERTEQUAL(true, machine.stroke.isDone());
    ASSERTEQUALS(JT(""), Serial.output().c_str());
    ASSERTEQUAL(0, arduino.pulses(PC2_X_DIR_PIN)-xdirpulses);	// never reversing
    ASSERTEQUAL(HIGH, arduino.getPin(PC2_X_DIR_PIN));	// advancing
    ASSERTEQUAL(3200, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
	ASSERTQUAD(Quad<StepCoord>(3200, 3200, 3200, 0), machine.getMotorPosition());

	mt.loop();	// command.process (second stroke)
	ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
	ASSERTEQUAL(true, machine.stroke.isDone());
    ASSERTEQUALS(JT(""), Serial.output().c_str());
    ASSERTEQUAL(0, arduino.pulses(PC2_X_DIR_PIN)-xdirpulses);	// never reversing
    ASSERTEQUAL(HIGH, arduino.getPin(PC2_X_DIR_PIN));	// advancing
    ASSERTEQUAL(6400, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
	// note that end coordinate is always the same even we've gone two strokes
	ASSERTQUAD(Quad<StepCoord>(3200, 3200, 3200, 0), machine.getMotorPosition());

	Serial.push("\n"); // terminate
	xpulses = arduino.pulses(PC2_X_STEP_PIN);
	mt.loop();	// command.process
	ASSERTEQUAL(STATUS_WAIT_CANCELLED, mt.status);
    ASSERTEQUALS(
		JT("{'s':-901,'r':{'tstph':{'pu':3200,'tv':0.70,'sg':41,'mv':12800,'lp':13072,'tt':0.84,'vp':7645.16}}}\n"), 
		Serial.output().c_str());
    ASSERTEQUAL(0, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
	mt.loop(); // idle

	// TEST: long line
	xpulses = arduino.pulses(PC2_X_STEP_PIN);
    Serial.push(JT("{'tstph':{'pu':12800,'tv':'','sg':'','mv':'','lp':''}}\n")); 
    mt.loop();	// command.parse
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);

	mt.loop();	// command.process
	ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
	ASSERTEQUAL(true, machine.stroke.isDone());
    ASSERTEQUALS(JT(""), Serial.output().c_str());
    ASSERTEQUAL(0, arduino.pulses(PC2_X_DIR_PIN)-xdirpulses);	// never reversing
    ASSERTEQUAL(HIGH, arduino.getPin(PC2_X_DIR_PIN));	// advancing
    ASSERTEQUAL(12800, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
	ASSERTQUAD(Quad<StepCoord>(12800, 12800, 12800, 0), machine.getMotorPosition());

	Serial.push("\n"); // terminate
	xpulses = arduino.pulses(PC2_X_STEP_PIN);
	mt.loop();	// command.process
	ASSERTEQUAL(STATUS_WAIT_CANCELLED, mt.status);
    ASSERTEQUALS(
		JT("{'s':-901,'r':{'tstph':{'pu':12800,'tv':0.70,'sg':79,'mv':12800,'lp':26562,'tt':1.70,'vp':12826.12}}}\n"), 
		Serial.output().c_str());
    ASSERTEQUAL(0, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
	mt.loop(); // idle

	// TEST: Fast acceleration
	xpulses = arduino.pulses(PC2_X_STEP_PIN);
    Serial.push(JT("{'tstph':{'pu':'','tv':0.01,'sg':'','mv':'','lp':''}}\n")); 
    mt.loop();	// command.parse
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);

	mt.loop();	// command.process
	ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
	ASSERTEQUAL(true, machine.stroke.isDone());
    ASSERTEQUALS(JT(""), Serial.output().c_str());
    ASSERTEQUAL(0, arduino.pulses(PC2_X_DIR_PIN)-xdirpulses);	// never reversing
    ASSERTEQUAL(HIGH, arduino.getPin(PC2_X_DIR_PIN));	// advancing
    ASSERTEQUAL(6400, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
	ASSERTQUAD(Quad<StepCoord>(6400, 6400, 6400, 0), machine.getMotorPosition());

	Serial.push("\n"); // terminate
	xpulses = arduino.pulses(PC2_X_STEP_PIN);
	mt.loop();	// command.process
	ASSERTEQUAL(STATUS_WAIT_CANCELLED, mt.status);
    ASSERTEQUALS(
		JT("{'s':-901,'r':{'tstph':{'pu':6400,'tv':0.01,'sg':64,'mv':12800,'lp':7968,'tt':0.51,'vp':12801.21}}}\n"), 
		Serial.output().c_str());
    ASSERTEQUAL(0, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
	mt.loop(); // idle

    cout << "TEST	: test_ph5() OK " << endl;
}

int main(int argc, char *argv[]) {
    LOGINFO3("INFO	: FireStep test v%d.%d.%d",
             VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
    firelog_level(FIRELOG_TRACE);

	// test first
	//test_ph5();

    test_Serial();
    test_Thread();
    test_Quad();
    test_Stroke();
    test_Machine_step();
    test_Machine();
    test_ArduinoJson();
    test_JsonCommand();
    test_JsonController();
    test_MachineThread();
    test_Display();
    test_Home();
    test_PrettyPrint();
    test_Idle();
    test_Move();
	test_PinConfig();
	test_dvs();
	test_errors();
	test_ph5();

    cout << "TEST	: END OF TEST main()" << endl;
}

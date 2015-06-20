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

void replaceChar(string &s, char cmatch, char creplace) {
    for (int i = 0; i < s.size(); i++) {
        if (s[i] == cmatch) {
            s[i] = creplace;
        }
    }
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
    ASSERTEQUAL(0x1, SREGI); 	// Global interrupts enabled
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
    ASSERTEQUAL(STATUS_OK, machine.step(Quad<StepDV>(1, 1, 1, 0)));
    ASSERTEQUAL(2, arduino.pulses(PC2_X_STEP_PIN));
    ASSERTEQUAL(2, arduino.pulses(PC2_Y_STEP_PIN));
    ASSERTEQUAL(2, arduino.pulses(PC2_Z_STEP_PIN));

    ASSERTEQUAL(STATUS_OK, machine.step(Quad<StepDV>(-1, -1, -1, 0)));
    ASSERTEQUAL(3, arduino.pulses(PC2_X_STEP_PIN));
    ASSERTEQUAL(3, arduino.pulses(PC2_Y_STEP_PIN));
    ASSERTEQUAL(3, arduino.pulses(PC2_Z_STEP_PIN));


    MachineThread machThread;
    machThread.setup(PC2_RAMPS_1_4);
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

	jsonBuffer.clear();
	char json3[] = "[1,'a',{\"color\":\"red\"}]";
    JsonObject& obj3 = jsonBuffer.parseObject(json3);
	ASSERTEQUAL(false, obj3.success());
    JsonArray& arr3 = jsonBuffer.parseArray(json3);
	ASSERTEQUAL(true, arr3.success());
	ASSERTEQUAL(1, arr3[0]);
	ASSERTEQUALS("a", arr3[1]);
	ASSERTEQUALS("red", arr3[2]["color"]);

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
    ASSERT(!cmd1.requestRoot().is<JsonArray&>());
    ASSERT(cmd1.requestRoot().is<JsonObject&>());
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

    Serial.clear();
	JsonCommand cmd4;
	Serial.push(JT("[{\"x\":1},{\"y\":2}]\n"));
    ASSERTEQUAL(STATUS_BUSY_PARSED, cmd4.parse());
    ASSERTEQUAL(STATUS_BUSY_PARSED, cmd4.getStatus());
    ASSERT(cmd4.requestRoot().success());
    ASSERT(cmd4.requestRoot().is<JsonArray&>());
    ASSERT(!cmd4.requestRoot().is<JsonObject&>());
    ASSERTEQUAL(2, cmd4.requestRoot().size());

    Serial.clear();
	JsonCommand cmd5;
	Serial.push(JT("{'sysee':'abcd'}\n"));
    ASSERTEQUAL(STATUS_BUSY_PARSED, cmd5.parse());
	char *s = cmd5.allocate(MAX_JSON);
	ASSERT(!s);
	s = cmd5.allocate(5);
	ASSERT(s);
	strcpy(s, "WXYZ");
	ASSERTEQUALS("abcd", cmd5.requestRoot()["sysee"]);
	ASSERTEQUALS("WXYZ", s);

    cout << "TEST	: test_JsonCommand() OK " << endl;
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
    testJSON(machine, jc, replace, "{'?':''}", "{'s':0,'r':{'?':{'ma':!}},'t':0.00}\n");
    testJSON(machine, jc, replace, "{'?ma':''}", "{'s':0,'r':{'?ma':!},'t':0.00}\n");
    testJSON(machine, jc, replace, "{'?ma':4}", "{'s':0,'r':{'?ma':4},'t':0.00}\n");
    testJSON(machine, jc, replace, "{'?ma':''}", "{'s':0,'r':{'?ma':4},'t':0.00}\n");
    testJSON(machine, jc, replace, "{'?':{'ma':''}}", "{'s':0,'r':{'?':{'ma':4}},'t':0.00}\n");
    testJSON(machine, jc, replace, "{'?':{'ma':!}}", "{'s':0,'r':{'?':{'ma':!}},'t':0.00}\n");
    testJSON(machine, jc, replace, "{'?':{'ma':''}}", "{'s':0,'r':{'?':{'ma':!}},'t':0.00}\n");
}

void test_JsonController_axis(Machine& machine, JsonController &jc, char axis) {
    string replace;
    replace.push_back('\'');
    replace.push_back('"');
    replace.push_back('?');
    replace.push_back(axis);
    testJSON(machine, jc, replace, "{'?tn':''}", "{'s':0,'r':{'?tn':-32000},'t':0.00}\n");
    testJSON(machine, jc, replace, "{'?tn':111}", "{'s':0,'r':{'?tn':111},'t':0.00}\n");
    testJSON(machine, jc, replace, "{'?tn':''}", "{'s':0,'r':{'?tn':111},'t':0.00}\n");
    testJSON(machine, jc, replace, "{'?':{'tn':''}}", "{'s':0,'r':{'?':{'tn':111}},'t':0.00}\n");
    testJSON(machine, jc, replace, "{'?':{'tn':-32000}}", "{'s':0,'r':{'?':{'tn':-32000}},'t':0.00}\n");
    testJSON(machine, jc, replace, "{'?':{'tn':''}}", "{'s':0,'r':{'?':{'tn':-32000}},'t':0.00}\n");
    testJSON(machine, jc, replace, "{'?tm':''}", "{'s':0,'r':{'?tm':32000},'t':0.00}\n");  	// default
    testJSON(machine, jc, replace, "{'?tm':222}", "{'s':0,'r':{'?tm':222},'t':0.00}\n");
    testJSON(machine, jc, replace, "{'?tm':''}", "{'s':0,'r':{'?tm':222},'t':0.00}\n");
    testJSON(machine, jc, replace, "{'?':{'tm':''}}", "{'s':0,'r':{'?':{'tm':222}},'t':0.00}\n");
    testJSON(machine, jc, replace, "{'?':{'tm':32000}}", "{'s':0,'r':{'?':{'tm':32000}},'t':0.00}\n");
    testJSON(machine, jc, replace, "{'?':{'tm':''}}", "{'s':0,'r':{'?':{'tm':32000}},'t':0.00}\n");
    testJSON(machine, jc, replace, "{'?':{'mi':''}}", "{'s':0,'r':{'?':{'mi':16}},'t':0.00}\n");
    testJSON(machine, jc, replace, "{'?':{'mi':1}}", "{'s':0,'r':{'?':{'mi':1}},'t':0.00}\n");
    testJSON(machine, jc, replace, "{'?':{'mi':''}}", "{'s':0,'r':{'?':{'mi':1}},'t':0.00}\n");
    testJSON(machine, jc, replace, "{'?':{'mi':16}}", "{'s':0,'r':{'?':{'mi':16}},'t':0.00}\n");
    testJSON(machine, jc, replace, "{'?':{'dh':''}}", "{'s':0,'r':{'?':{'dh':true}},'t':0.00}\n");
    testJSON(machine, jc, replace, "{'?':{'dh':false}}", "{'s':0,'r':{'?':{'dh':false}},'t':0.00}\n");
    testJSON(machine, jc, replace, "{'?':{'dh':''}}", "{'s':0,'r':{'?':{'dh':false}},'t':0.00}\n");
    testJSON(machine, jc, replace, "{'?':{'dh':true}}", "{'s':0,'r':{'?':{'dh':true}},'t':0.00}\n");
    testJSON(machine, jc, replace, "{'?':{'sa':''}}", "{'s':0,'r':{'?':{'sa':1.80}},'t':0.00}\n");
    testJSON(machine, jc, replace, "{'?':{'sa':0.9}}", "{'s':0,'r':{'?':{'sa':0.90}},'t':0.00}\n");
    testJSON(machine, jc, replace, "{'?':{'sa':''}}", "{'s':0,'r':{'?':{'sa':0.90}},'t':0.00}\n");
    testJSON(machine, jc, replace, "{'?':{'sa':1.8}}", "{'s':0,'r':{'?':{'sa':1.80}},'t':0.00}\n");

    testJSON(machine, jc, replace, "{'x':''}",
             "{'s':0,'r':{'x':{'dh':true,'en':true,'ho':0,'is':0,'lb':16,'lm':false,'ln':false,"\
			 "'mi':16,'pd':55,'pe':38,'pm':255,'pn':3,'po':0,'ps':54,"\
			 "'sa':1.80,'sd':80,'tm':32000,'tn':-32000,'ud':0}},'t':0.00}\n");
    testJSON(machine, jc, replace, "{'y':''}",
             "{'s':0,'r':{'y':{'dh':true,'en':true,'ho':0,'is':0,'lb':16,'lm':false,'ln':false,"\
			 "'mi':16,'pd':61,'pe':56,'pm':255,'pn':14,'po':0,'ps':60,"\
			 "'sa':1.80,'sd':80,'tm':32000,'tn':-32000,'ud':0}},'t':0.00}\n");
    testJSON(machine, jc, replace, "{'z':''}",
             "{'s':0,'r':{'z':{'dh':true,'en':true,'ho':0,'is':0,'lb':16,'lm':false,'ln':false,"\
			 "'mi':16,'pd':48,'pe':62,'pm':255,'pn':18,'po':0,'ps':46,"\
			 "'sa':1.80,'sd':80,'tm':32000,'tn':-32000,'ud':0}},'t':0.00}\n");
}

void test_JsonController_machinePosition(Machine& machine, JsonController &jc) {
    string replace;
    replace.push_back('\'');
    replace.push_back('"');
    testJSON(machine, jc, replace, 
		"{'mpo':''}", "{'s':0,'r':{'mpo':{'1':0,'2':0,'3':0,'4':0}},'t':0.00}\n");
    testJSON(machine, jc, replace, "{'mpo1':''}", "{'s':0,'r':{'mpo1':0},'t':0.00}\n");
    testJSON(machine, jc, replace, "{'mpo1':32760}", "{'s':0,'r':{'mpo1':32760},'t':0.00}\n");
    testJSON(machine, jc, replace, "{'mpo1':''}", "{'s':0,'r':{'mpo1':32760},'t':0.00}\n");
    testJSON(machine, jc, replace, "{'mpo1':-32760}", "{'s':0,'r':{'mpo1':-32760},'t':0.00}\n");
    testJSON(machine, jc, replace, "{'mpo1':''}", "{'s':0,'r':{'mpo1':-32760},'t':0.00}\n");
    testJSON(machine, jc, replace, "{'mpo2':''}", "{'s':0,'r':{'mpo2':0},'t':0.00}\n");
    testJSON(machine, jc, replace, "{'mpo2':32761}", "{'s':0,'r':{'mpo2':32761},'t':0.00}\n");
    testJSON(machine, jc, replace, "{'mpo2':''}", "{'s':0,'r':{'mpo2':32761},'t':0.00}\n");
    testJSON(machine, jc, replace, "{'mpo2':-32761}", "{'s':0,'r':{'mpo2':-32761},'t':0.00}\n");
    testJSON(machine, jc, replace, "{'mpo2':''}", "{'s':0,'r':{'mpo2':-32761},'t':0.00}\n");
    testJSON(machine, jc, replace, "{'mpo3':''}", "{'s':0,'r':{'mpo3':0},'t':0.00}\n");
    testJSON(machine, jc, replace, "{'mpo3':32762}", "{'s':0,'r':{'mpo3':32762},'t':0.00}\n");
    testJSON(machine, jc, replace, "{'mpo3':''}", "{'s':0,'r':{'mpo3':32762},'t':0.00}\n");
    testJSON(machine, jc, replace, "{'mpo3':-32762}", "{'s':0,'r':{'mpo3':-32762},'t':0.00}\n");
    testJSON(machine, jc, replace, "{'mpo3':''}", "{'s':0,'r':{'mpo3':-32762},'t':0.00}\n");
    testJSON(machine, jc, replace, "{'mpo4':''}", "{'s':0,'r':{'mpo4':0},'t':0.00}\n");
    testJSON(machine, jc, replace, "{'mpo4':32763}", "{'s':0,'r':{'mpo4':32763},'t':0.00}\n");
    testJSON(machine, jc, replace, "{'mpo4':''}", "{'s':0,'r':{'mpo4':32763},'t':0.00}\n");
    testJSON(machine, jc, replace, "{'mpo4':-32763}", "{'s':0,'r':{'mpo4':-32763},'t':0.00}\n");
    testJSON(machine, jc, replace, "{'mpo4':''}", "{'s':0,'r':{'mpo4':-32763},'t':0.00}\n");
    testJSON(machine, jc, replace, "{'mpo':''}",
             "{'s':0,'r':{'mpo':{'1':-32760,'2':-32761,'3':-32762,'4':-32763}},'t':0.00}\n");
}

MachineThread test_setup() {
    arduino.clear();
    threadRunner.clear();
    MachineThread mt;
    mt.machine.pDisplay = &testDisplay;
    testDisplay.clear();
    mt.setup(PC2_RAMPS_1_4);
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
    ASSERTEQUALS("FireStep 0.1.6\n", Serial.output().c_str());
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
    Serial.push(JT("{'tstrv':[1,2]}\n")); // tstrv: test revolutions steps
    mt.loop();	// command.parse
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

    mt.loop();	// controller.process
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

    Serial.push("\n"); // cancel current command
    mt.loop();
    ASSERTEQUAL(STATUS_WAIT_CANCELLED, mt.status);
    ASSERTEQUAL(DISPLAY_WAIT_CANCELLED, mt.machine.pDisplay->getStatus());
    ASSERTEQUALS(JT("{'s':-901,'r':{'tstrv':[1,2]},'t':0.50}\n"), Serial.output().c_str());

    mt.loop();
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);
    ASSERTEQUAL(DISPLAY_WAIT_IDLE, mt.machine.pDisplay->getStatus());

    Serial.push(JT("{'tstsp':[1,100,1000]}\n")); // tstsp: test stepper pulse
    xpulses = arduino.pulses(PC2_X_STEP_PIN);
    ypulses = arduino.pulses(PC2_Y_STEP_PIN);
    zpulses = arduino.pulses(PC2_Z_STEP_PIN);
    mt.loop();	// parse
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    ASSERTEQUAL(DISPLAY_BUSY, mt.machine.pDisplay->getStatus());
    ASSERTEQUAL(0, Serial.available());

    mt.loop();	// controller.process
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUAL(DISPLAY_WAIT_IDLE, mt.machine.pDisplay->getStatus());
    ASSERTEQUAL(xpulses + 1, arduino.pulses(PC2_X_STEP_PIN));
    ASSERTEQUAL(ypulses + 100, arduino.pulses(PC2_Y_STEP_PIN));
    ASSERTEQUAL(zpulses + 1000, arduino.pulses(PC2_Z_STEP_PIN));
    ASSERTEQUALS(JT("{'s':0,'r':{'tstsp':[1,100,1000]},'t':0.00}\n"), Serial.output().c_str());

    mt.loop();
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);
    ASSERTEQUAL(DISPLAY_WAIT_IDLE, mt.machine.pDisplay->getStatus());
    ASSERTEQUAL(xpulses + 1, arduino.pulses(PC2_X_STEP_PIN));
    ASSERTEQUAL(ypulses + 100, arduino.pulses(PC2_Y_STEP_PIN));
    ASSERTEQUAL(zpulses + 1000, arduino.pulses(PC2_Z_STEP_PIN));
    ASSERTEQUALS("", Serial.output().c_str());

    Serial.push(JT("{'tstrv':[-1,-2]}\n")); // tstrv: test revolutions steps
    mt.loop();	// command.parse
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    ASSERTEQUAL(DISPLAY_BUSY, mt.machine.pDisplay->getStatus());
    ASSERTEQUAL(0, Serial.available()); // expected parse
    xpulses = arduino.pulses(PC2_X_STEP_PIN);
    ypulses = arduino.pulses(PC2_Y_STEP_PIN);
    zpulses = arduino.pulses(PC2_Z_STEP_PIN);

    mt.loop();	// controller.process
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    ASSERTEQUAL(DISPLAY_BUSY_MOVING, mt.machine.pDisplay->getStatus());
    ASSERTEQUAL(xpulses + 2 * 3200, arduino.pulses(PC2_X_STEP_PIN));
    ASSERTEQUAL(ypulses + 2 * 6400, arduino.pulses(PC2_Y_STEP_PIN));
    ASSERTEQUAL(zpulses, arduino.pulses(PC2_Z_STEP_PIN));

    mt.loop();	// controller.process
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
    const char *fmt = "{'s':%d,'r':{'sys':{'ee':'','fr':1000,'jp':false,'lh':false,'lp':0,'pc':2,'tc':12345,'v':%.2f}},'t':0.00}\n";
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
        virtual Status stepDirection(const Quad<StepDV> &pulse) {
            return STATUS_OK;
        }
        virtual Status stepFast(Quad<StepDV> &pulse) {
			return step(pulse);
		}
        virtual Status step(const Quad<StepDV> &pulse) {
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
    ASSERTEQUAL(0, stroke.getTimePlanned()*TICKS_PER_SECOND);
    ASSERTEQUAL(STATUS_STROKE_TIME, stroke.start(tStart));

    stroke.setTimePlanned(17/(float) TICKS_PER_SECOND); 
    ASSERTEQUAL(17, stroke.getTimePlanned()*TICKS_PER_SECOND);

    stroke.dEndPos.value[0] += 1000;
    ASSERTEQUAL(STATUS_STROKE_END_ERROR, stroke.start(tStart));
    stroke.dEndPos.value[0] -= 1000;
    ASSERTEQUAL(17, stroke.getTimePlanned()*TICKS_PER_SECOND);

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
    stroke.scale = 30;
    ASSERTEQUAL(STATUS_STROKE_END_ERROR, stroke.start(tStart));
    stroke.scale = 3;
    ASSERTEQUAL(STATUS_OK, stroke.start(tStart));
    stroke.dEndPos *= stroke.scale;
    ASSERTEQUAL(STATUS_OK, stroke.start(tStart));
    ASSERTEQUAL(STATUS_OK, stroke.start(tStart));
    ASSERTQUAD(Quad<StepCoord>(12, 120, -12, -120), stroke.dEndPos);
    ASSERTQUAD(Quad<StepCoord>(12, 120, -12, -120), stroke.goalPos(tStart + 17));
    ASSERTQUAD(Quad<StepCoord>(9, 105, -9, -105), stroke.goalPos(tStart + 14));
    ASSERTQUAD(Quad<StepCoord>(9, 90, -9, -90), stroke.goalPos(tStart + 11));
    ASSERTQUAD(Quad<StepCoord>(6, 60, -6, -60), stroke.goalPos(tStart + 8));
    ASSERTQUAD(Quad<StepCoord>(3, 30, -3, -30), stroke.goalPos(tStart + 5));
    ASSERTQUAD(Quad<StepCoord>(0, 0, 0, 0), stroke.goalPos(tStart));
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
    stroke.dEndPos.value[3] += 1002;
    ASSERTEQUAL(STATUS_STROKE_END_ERROR, stroke.start(tStart));
    stroke.dEndPos.value[3] -= 1000;
    ASSERTEQUAL(STATUS_OK, stroke.start(tStart));
    ASSERTEQUAL(17, stroke.getTimePlanned()*TICKS_PER_SECOND);
    ASSERTQUAD(Quad<StepCoord>(0, 0, 0, 0), stroke.position());
    ASSERTQUAD(Quad<StepCoord>(13, 122, -11, -118), stroke.dEndPos);
    ASSERTQUAD(Quad<StepCoord>(13, 122, -11, -118), stroke.goalPos(tStart + 17));
    ASSERTQUAD(Quad<StepCoord>(9, 105, -9, -105), stroke.goalPos(tStart + 14));
    ASSERTQUAD(Quad<StepCoord>(9, 90, -9, -90), stroke.goalPos(tStart + 11));
    ASSERTQUAD(Quad<StepCoord>(6, 60, -6, -60), stroke.goalPos(tStart + 8));
    ASSERTQUAD(Quad<StepCoord>(3, 30, -3, -30), stroke.goalPos(tStart + 5));
    ASSERTQUAD(Quad<StepCoord>(0, 0, 0, 0), stroke.goalPos(tStart));

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

    machine.axis[0].travelMin = 0;
    machine.axis[1].travelMin = 0;
    machine.axis[2].travelMin = 0;
    machine.axis[3].travelMin = 0;
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
    ASSERTQUAD(machine.getMotorPosition(), Quad<StepDV>(0, 0, 0, 0));
    ASSERTEQUAL(STATUS_STEP_RANGE_ERROR, machine.step(Quad<StepDV>(4, 3, 2, 1)));
    ASSERTEQUAL(STATUS_AXIS_DISABLED, machine.step(Quad<StepDV>(1, 1, 1, 1)));
    ASSERTQUAD(machine.getMotorPosition(), Quad<StepCoord>(0, 0, 0, 0));

    // Test travelMax
    ASSERTEQUAL(STATUS_OK, machine.step(Quad<StepDV>(1, 0, 0, 0)));
    ASSERTEQUAL(false, machine.axis[3].atMin);
    ASSERT(machine.getMotorPosition() == Quad<StepCoord>(1, 0, 0, 0));
    ASSERTEQUAL(STATUS_OK, machine.step(Quad<StepDV>(1, 1, 0, 0)));
    ASSERTEQUAL(false, machine.axis[3].atMin);
    ASSERT(machine.getMotorPosition() == Quad<StepCoord>(2, 1, 0, 0));
    ASSERTEQUAL(STATUS_OK, machine.step(Quad<StepDV>(1, 1, 1, 0)));
    ASSERTEQUAL(false, machine.axis[3].atMin);
    ASSERTQUAD(machine.getMotorPosition(), Quad<StepCoord>(3, 2, 1, 0));
    ASSERTEQUAL(STATUS_AXIS_DISABLED, machine.step(Quad<StepDV>(1, 1, 1, 1)));
    ASSERTQUAD(machine.getMotorPosition(), Quad<StepCoord>(3, 2, 1, 0));
    ASSERTEQUAL(STATUS_OK, machine.step(Quad<StepDV>(1, 1, 1, 0)));
    ASSERTEQUAL(false, machine.axis[3].atMin);
    ASSERTQUAD(Quad<StepCoord>(4, 3, 2, 0), machine.getMotorPosition());
    ASSERTEQUAL(STATUS_OK, machine.step(Quad<StepDV>(1, 1, 1, 0)));
    ASSERTEQUAL(false, machine.axis[3].atMin);
    ASSERTQUAD(machine.getMotorPosition(), Quad<StepCoord>(5, 4, 3, 0));
    ASSERTEQUAL(false, machine.axis[0].atMin);
    ASSERTEQUAL(false, machine.axis[1].atMin);
    ASSERTEQUAL(false, machine.axis[2].atMin);
    ASSERTEQUAL(false, machine.axis[3].atMin);
    ASSERTEQUAL(STATUS_TRAVEL_MAX, machine.step(Quad<StepDV>(1, 1, 1, 0)));
    ASSERTQUAD(Quad<StepCoord>(5, 4, 3, 0), machine.getMotorPosition());

    // Test travelMin
    ASSERTEQUAL(false, machine.axis[0].atMin);
    ASSERTEQUAL(false, machine.axis[1].atMin);
    ASSERTEQUAL(false, machine.axis[2].atMin);
    ASSERTEQUAL(false, machine.axis[3].atMin);
    ASSERTEQUAL(STATUS_OK, machine.step(Quad<StepDV>(-1, -1, -1, 0)));
    ASSERTQUAD(Quad<StepCoord>(4, 3, 2, 0), machine.getMotorPosition());
    ASSERTEQUAL(STATUS_OK, machine.step(Quad<StepDV>(-1, -1, -1, 0)));
    ASSERTQUAD(Quad<StepCoord>(3, 2, 1, 0), machine.getMotorPosition());
    ASSERTEQUAL(STATUS_OK, machine.step(Quad<StepDV>(-1, -1, -1, 0)));
    ASSERTQUAD(Quad<StepCoord>(2, 1, 0, 0), machine.getMotorPosition());
    ASSERTEQUAL(STATUS_TRAVEL_MIN, machine.step(Quad<StepDV>(-1, -1, -1, 0)));
    ASSERTQUAD(Quad<StepCoord>(2, 1, 0, 0), machine.getMotorPosition());

    // Test atMin
    arduino.setPin(machine.axis[0].pinMin, 1);
    machine.axis[0].travelMin = -10;
    machine.axis[1].travelMin = -10;
    machine.axis[2].travelMin = -10;
    ASSERTEQUAL(STATUS_LIMIT_MIN, machine.step(Quad<StepDV>(-1, -1, -1, 0)));
    ASSERTQUAD(Quad<StepCoord>(2, 1, 0, 0), machine.getMotorPosition());

    cout << "TEST	: test_Machine_step() OK " << endl;
}

void test_PinConfig() {
    cout << "TEST	: test_PinConfig() =====" << endl;

    MachineThread mt = test_setup();
    Machine &machine = mt.machine;

    Serial.push(JT("{'syspc':1}\n"));
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);

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
	ASSERTEQUAL(PC1_EMC02, machine.getPinConfig());

    cout << "TEST	: test_PinConfig() OK " << endl;
}

void test_Move() {
    cout << "TEST	: test_Move() =====" << endl;

    MachineThread mt = test_setup();
    Machine &machine = mt.machine;
    int32_t xpulses;
    int32_t ypulses;
    int32_t zpulses;

	// mov to (1,10,100)
    xpulses = arduino.pulses(PC2_X_STEP_PIN);
    ypulses = arduino.pulses(PC2_Y_STEP_PIN);
    zpulses = arduino.pulses(PC2_Z_STEP_PIN);
    machine.setMotorPosition(Quad<StepCoord>());
    Serial.push(JT("{'mov':{'1':1,'2':10,'3':100}}\n"));
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop();
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUAL(xpulses + 1, arduino.pulses(PC2_X_STEP_PIN));
    ASSERTEQUAL(ypulses + 10, arduino.pulses(PC2_Y_STEP_PIN));
    ASSERTEQUAL(zpulses + 100, arduino.pulses(PC2_Z_STEP_PIN));
    ASSERTQUAD(Quad<StepCoord>(1, 10, 100, 0), machine.getMotorPosition());
    ASSERTEQUALS(JT("{'s':0,'r':"\
                    "{'mov':{'1':1,'2':10,'3':100,'lp':2310,'pp':1298.1,'sg':16,'ts':0.148,'tp':0.148}},"\
                    "'t':0.15}\n"),
                 Serial.output().c_str());
    mt.loop();
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

	// mov to same position
    Serial.push(JT("{'mov':{'1':1,'2':10,'3':100}}\n"));
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop();
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUAL(xpulses + 1, arduino.pulses(PC2_X_STEP_PIN));
    ASSERTEQUAL(ypulses + 10, arduino.pulses(PC2_Y_STEP_PIN));
    ASSERTEQUAL(zpulses + 100, arduino.pulses(PC2_Z_STEP_PIN));
    ASSERTQUAD(Quad<StepCoord>(1, 10, 100, 0), machine.getMotorPosition());
    ASSERTEQUALS(JT("{'s':0,'r':"\
                    "{'mov':{'1':1,'2':10,'3':100,'lp':0,'pp':0.0,'sg':0,'ts':0.000,'tp':0.000}},"\
                    "'t':0.00}\n"),
                 Serial.output().c_str());
    mt.loop();
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

	// mov nowhere but return position
    Serial.push(JT("{'mov':''}\n"));
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop();
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUAL(xpulses + 1, arduino.pulses(PC2_X_STEP_PIN));
    ASSERTEQUAL(ypulses + 10, arduino.pulses(PC2_Y_STEP_PIN));
    ASSERTEQUAL(zpulses + 100, arduino.pulses(PC2_Z_STEP_PIN));
    ASSERTQUAD(Quad<StepCoord>(1, 10, 100, 0), machine.getMotorPosition());
    ASSERTEQUALS(JT("{'s':0,'r':"\
                    "{'mov':{'lp':0,'mv':12800,'pp':0.0,'sg':0,'ts':0.000,'tp':0.000,'1':1,'2':10,'3':100}},"\
                    "'t':0.00}\n"),
                 Serial.output().c_str());
    mt.loop();
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

	// mov x 
    Serial.push(JT("{'mov':{'x':50,'z':300}}\n"));
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop();
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUAL(xpulses + 50, arduino.pulses(PC2_X_STEP_PIN));
    ASSERTEQUAL(ypulses + 10, arduino.pulses(PC2_Y_STEP_PIN));
    ASSERTEQUAL(zpulses + 300, arduino.pulses(PC2_Z_STEP_PIN));
    ASSERTQUAD(Quad<StepCoord>(50, 10, 300, 0), machine.getMotorPosition());
    ASSERTEQUALS(JT("{'s':0,'r':"\
                    "{'mov':{'x':50,'z':300,'lp':3268,'pp':1911.9,'sg':16,'ts':0.209,'tp':0.209}},"\
                    "'t':0.21}\n"),
                 Serial.output().c_str());
    mt.loop();
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

	// mov 10000,5000,9000 @ 16000 
    xpulses = arduino.pulses(PC2_X_STEP_PIN);
    ypulses = arduino.pulses(PC2_Y_STEP_PIN);
    zpulses = arduino.pulses(PC2_Z_STEP_PIN);
    machine.setMotorPosition(Quad<StepCoord>());
    Serial.push(JT("{'mov':{'x':10000,'y':5000,'z':9000,'mv':16000}}\n"));
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop();
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUAL(10000, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    ASSERTEQUAL(5000, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
    ASSERTEQUAL(9000, arduino.pulses(PC2_Z_STEP_PIN)-zpulses);
    ASSERTQUAD(Quad<StepCoord>(10000,5000,9000,0), machine.getMotorPosition());
    ASSERTEQUALS(JT("{'s':0,'r':"\
                    "{'mov':{'x':10000,'y':5000,'z':9000,'mv':16000,"\
					"'lp':20669,'pp':15118.5,'sg':50,'ts':1.323,'tp':1.323}},"\
                    "'t':1.32}\n"),
                 Serial.output().c_str());
    mt.loop();
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

	// mov short form
    xpulses = arduino.pulses(PC2_X_STEP_PIN);
    ypulses = arduino.pulses(PC2_Y_STEP_PIN);
    zpulses = arduino.pulses(PC2_Z_STEP_PIN);
    machine.setMotorPosition(Quad<StepCoord>());
    Serial.push(JT("{'movx':10000,'movy':5000}}\n"));
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop();
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUAL(10000, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    ASSERTEQUAL(5000, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
    ASSERTEQUAL(0, arduino.pulses(PC2_Z_STEP_PIN)-zpulses);
    ASSERTQUAD(Quad<StepCoord>(10000,5000,0,0), machine.getMotorPosition());
    ASSERTEQUALS(JT("{'s':0,'r':{'movx':10000,'movy':5000},'t':2.26}\n"),
                 Serial.output().c_str());
    mt.loop();
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

    cout << "TEST	: test_Move() OK " << endl;
}

void test_stroke_endpos() {
    cout << "TEST	: test_stroke_endpos() =====" << endl;

    MachineThread mt = test_setup();
    Machine &machine = mt.machine;
    int32_t xpulses;
    int32_t ypulses;
    int32_t zpulses;
	const char *json;

	// TEST: calculated end position
    xpulses = arduino.pulses(PC2_X_STEP_PIN);
    ypulses = arduino.pulses(PC2_Y_STEP_PIN);
    zpulses = arduino.pulses(PC2_Z_STEP_PIN);
    machine.setMotorPosition(Quad<StepCoord>(0, 0, 0, 0));
	json = "{'dvs':{'us':512,'1':[10,20],'2':[40,50],'3':[70,80]}}\n";
    Serial.push(JT(json));
    test_ticks(1); // parse
    ASSERTEQUAL(0, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    ASSERTEQUAL(0, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
    ASSERTEQUAL(0, arduino.pulses(PC2_Z_STEP_PIN)-zpulses);
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    test_ticks(1); // initialize
    ASSERTEQUAL(0, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    ASSERTEQUAL(0, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
    ASSERTEQUAL(0, arduino.pulses(PC2_Z_STEP_PIN)-zpulses);
	ASSERTQUAD(Quad<StepCoord>(40,130,220,0), machine.stroke.dEndPos);
	ASSERTQUAD(Quad<StepCoord>(0,0,0,0), machine.getMotorPosition());
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    test_ticks(1); // moving
    ASSERTEQUAL(7, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    ASSERTEQUAL(30, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
    ASSERTEQUAL(52, arduino.pulses(PC2_Z_STEP_PIN)-zpulses);
	ASSERTQUAD(Quad<StepCoord>(7,30,52,0), machine.getMotorPosition());
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    test_ticks(1); // moving
    ASSERTEQUAL(25, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    ASSERTEQUAL(85, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
    ASSERTEQUAL(145, arduino.pulses(PC2_Z_STEP_PIN)-zpulses);
	ASSERTQUAD(Quad<StepCoord>(25,85,145,0), machine.getMotorPosition());
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    test_ticks(1); // moving
    ASSERTEQUAL(40, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    ASSERTEQUAL(130, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
    ASSERTEQUAL(220, arduino.pulses(PC2_Z_STEP_PIN)-zpulses);
	ASSERTQUAD(Quad<StepCoord>(40,130,220,0), machine.getMotorPosition());
    ASSERTEQUAL(STATUS_OK, mt.status);

    test_ticks(1); // idle
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

	// TEST: given end position
    xpulses = arduino.pulses(PC2_X_STEP_PIN);
    ypulses = arduino.pulses(PC2_Y_STEP_PIN);
    zpulses = arduino.pulses(PC2_Z_STEP_PIN);
    machine.setMotorPosition(Quad<StepCoord>(0, 0, 0, 0));
	json = "{'dvs':{'us':512,'1':[10,20],'2':[40,50],'3':[70,80],"
		"'dp':[41,132,223]}}\n";
    Serial.push(JT(json));
    test_ticks(1); // parse
    ASSERTEQUAL(0, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    ASSERTEQUAL(0, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
    ASSERTEQUAL(0, arduino.pulses(PC2_Z_STEP_PIN)-zpulses);
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    test_ticks(1); // initialize
    ASSERTEQUAL(0, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    ASSERTEQUAL(0, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
    ASSERTEQUAL(0, arduino.pulses(PC2_Z_STEP_PIN)-zpulses);
	ASSERTQUAD(Quad<StepCoord>(41,132,223,0), machine.stroke.dEndPos);
	ASSERTQUAD(Quad<StepCoord>(0,0,0,0), machine.getMotorPosition());
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    test_ticks(1); // moving
    ASSERTEQUAL(7, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    ASSERTEQUAL(30, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
    ASSERTEQUAL(52, arduino.pulses(PC2_Z_STEP_PIN)-zpulses);
	ASSERTQUAD(Quad<StepCoord>(7,30,52,0), machine.getMotorPosition());
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    test_ticks(1); // moving
    ASSERTEQUAL(25, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    ASSERTEQUAL(85, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
    ASSERTEQUAL(145, arduino.pulses(PC2_Z_STEP_PIN)-zpulses);
	ASSERTQUAD(Quad<StepCoord>(25,85,145,0), machine.getMotorPosition());
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    test_ticks(1); // moving
    ASSERTEQUAL(41, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    ASSERTEQUAL(132, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
    ASSERTEQUAL(223, arduino.pulses(PC2_Z_STEP_PIN)-zpulses);
	ASSERTQUAD(Quad<StepCoord>(41,132,223,0), machine.getMotorPosition());
    ASSERTEQUAL(STATUS_OK, mt.status);

    cout << "TEST	: test_stroke_endpos() OK " << endl;
}

void test_pnp() {
    cout << "TEST	: test_pnp() =====" << endl;

    MachineThread mt = test_setup();
    Machine &machine = mt.machine;
    int32_t xpulses = arduino.pulses(PC2_X_STEP_PIN);
    int32_t ypulses = arduino.pulses(PC2_Y_STEP_PIN);
    int32_t zpulses = arduino.pulses(PC2_Z_STEP_PIN);
    machine.setMotorPosition(Quad<StepCoord>(9563, 5959, 12228, 100));

    const char *json =
        "{'dvs':{"
        "'1':'FF01FDFBFAF7F5F2F1EFEEF0EFF0F3F4FA00080E161818161311100D0D0C0A0A"
        "090808070606040405040304040303040304040300FCF6F1EEECEAEDF0F6FAFE',"
        "'2':'FF00FBF9F6F1F3F0F5F5FBFE030306070A0D1113181515110E0C090907060606"
        "0504040303030102000101000000FFFF01FE00FEFCF7F2EEEDEAECEDF2F6FBFE',"
        "'3':'0000FEFFFCFBF7F6F0EDE9E6E5E6E3E8ECF3FA050F13151515141211100F0D0D"
        "0C0A090A070608050507040606060607060708070501FDF8F3F0EFF1F2F7FAFF',"
        "'sc':2,'us':1873817,'dp':[1556,7742,-4881]}}\n";

    Serial.push(JT(json));
    test_ticks(1); // parse
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    ASSERTEQUAL(xpulses, arduino.pulses(PC2_X_STEP_PIN));

    test_ticks(MS_TICKS(1)); // initialize
    ASSERTEQUAL(0, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);

    test_ticks(MS_TICKS(100));
    ASSERTEQUAL(14, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    test_ticks(MS_TICKS(100));
    ASSERTEQUAL(154, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    test_ticks(MS_TICKS(100));
    ASSERTEQUAL(598, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    test_ticks(MS_TICKS(100));
    ASSERTEQUAL(1432, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    test_ticks(MS_TICKS(100));
    ASSERTEQUAL(2582, arduino.pulses(PC2_X_STEP_PIN)-xpulses);

    test_ticks(MS_TICKS(100));
    ASSERTEQUAL(3692, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    test_ticks(MS_TICKS(100));
    ASSERTEQUAL(4334, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    test_ticks(MS_TICKS(100));
    ASSERTEQUAL(4514, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    test_ticks(MS_TICKS(100));
    ASSERTEQUAL(4650, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    test_ticks(MS_TICKS(100));
    ASSERTEQUAL(5022, arduino.pulses(PC2_X_STEP_PIN)-xpulses);

    test_ticks(MS_TICKS(100));
    ASSERTEQUAL(5570, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    test_ticks(MS_TICKS(100));
    ASSERTEQUAL(6236, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    test_ticks(MS_TICKS(100));
    ASSERTEQUAL(6998, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    test_ticks(MS_TICKS(100));
    ASSERTEQUAL(7840, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    test_ticks(MS_TICKS(100));
    ASSERTEQUAL(8766, arduino.pulses(PC2_X_STEP_PIN)-xpulses);

    test_ticks(MS_TICKS(100));
    ASSERTEQUAL(9708, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    test_ticks(MS_TICKS(100));
    ASSERTEQUAL(10364, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    test_ticks(MS_TICKS(100));
    ASSERTEQUAL(10570, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    test_ticks(MS_TICKS(100));
    ASSERTEQUAL(10584, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    ASSERTEQUAL(STATUS_OK, mt.status);
    test_ticks(MS_TICKS(100));
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

    ASSERTEQUAL(10584, arduino.pulses(PC2_X_STEP_PIN) - xpulses);
    ASSERTEQUAL(12038, arduino.pulses(PC2_Y_STEP_PIN) - ypulses);
    ASSERTEQUAL(10423, arduino.pulses(PC2_Z_STEP_PIN) - zpulses);
    ASSERTEQUALS(JT("{'s':0,'r':{'dvs':{'1':1556,'2':7742,'3':-4881,"
                    "'sc':2,'us':1873817,'dp':[1556,7742,-4881]}},'t':1.90}\n"), 
					Serial.output().c_str());
    ASSERTQUAD(Quad<StepCoord>(11119, 13701, 7347, 100), machine.getMotorPosition());

    cout << "TEST	: test_pnp() OK " << endl;
}

void test_dvs() {
    cout << "TEST	: test_dvs() =====" << endl;

    MachineThread mt = test_setup();
    Machine &machine = mt.machine;
    int32_t xpulses = arduino.pulses(PC2_X_STEP_PIN);
    machine.setMotorPosition(Quad<StepCoord>(100, 100, 100, 100));

    Serial.push(JT("{'dvs':{'us':5000000,'x':[10,0,0,0,0]}}\n"));
    test_ticks(1); // parse
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    ASSERTEQUAL(xpulses, arduino.pulses(PC2_X_STEP_PIN));

    test_ticks(1); // initialize
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    ASSERTEQUAL(xpulses, arduino.pulses(PC2_X_STEP_PIN));
    ASSERTEQUAL(1, threadClock.ticks - machine.stroke.tStart);

    test_ticks(1); // start moving
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    ASSERTEQUAL(xpulses, arduino.pulses(PC2_X_STEP_PIN));

    test_ticks(MS_TICKS(100)); // moving
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    ASSERTEQUAL(1, arduino.pulses(PC2_X_STEP_PIN) - xpulses);
    ASSERTEQUAL(MS_TICKS(100) + 6, threadClock.ticks - machine.stroke.tStart);

    test_ticks(MS_TICKS(100)); // moving
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    ASSERTEQUAL(2, arduino.pulses(PC2_X_STEP_PIN) - xpulses);
    ASSERTEQUAL(MS_TICKS(200) + 7, threadClock.ticks - machine.stroke.tStart);

    test_ticks(MS_TICKS(100)); // moving
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    ASSERTEQUAL(3, arduino.pulses(PC2_X_STEP_PIN) - xpulses);
    ASSERTEQUAL(MS_TICKS(300) + 9, threadClock.ticks - machine.stroke.tStart);

    test_ticks(MS_TICKS(100)); // moving
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    ASSERTEQUAL(4, arduino.pulses(PC2_X_STEP_PIN) - xpulses);
    ASSERTEQUAL(MS_TICKS(400) + 10, threadClock.ticks - machine.stroke.tStart);

    test_ticks(MS_TICKS(500) - 4 * MS_TICKS(100)); // moving
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    ASSERTEQUAL(5, arduino.pulses(PC2_X_STEP_PIN) - xpulses);
    ASSERTEQUAL(MS_TICKS(500) + 14, threadClock.ticks - machine.stroke.tStart);

    test_ticks(MS_TICKS(1000) - MS_TICKS(500)); // moving
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    ASSERTEQUAL(10, arduino.pulses(PC2_X_STEP_PIN) - xpulses);
    ASSERTEQUAL(MS_TICKS(1000) + 16, threadClock.ticks - machine.stroke.tStart);
    ASSERTQUAD(Quad<StepCoord>(110, 100, 100, 100), machine.getMotorPosition());

    test_ticks(MS_TICKS(500)); // moving
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    ASSERTEQUAL(15, arduino.pulses(PC2_X_STEP_PIN) - xpulses);
    ASSERTEQUAL(MS_TICKS(1500) + 18, threadClock.ticks - machine.stroke.tStart);

    test_ticks(MS_TICKS(1000) - MS_TICKS(500)); // moving
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    ASSERTEQUAL(20, arduino.pulses(PC2_X_STEP_PIN) - xpulses);
    ASSERTEQUAL(MS_TICKS(2000) + 20, threadClock.ticks - machine.stroke.tStart);
    ASSERTQUAD(Quad<StepCoord>(120, 100, 100, 100), machine.getMotorPosition());

    test_ticks(MS_TICKS(1000)); // moving
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    ASSERTEQUAL(30, arduino.pulses(PC2_X_STEP_PIN) - xpulses);
    ASSERTEQUAL(MS_TICKS(3000) + 22, threadClock.ticks - machine.stroke.tStart);
    ASSERTQUAD(Quad<StepCoord>(130, 100, 100, 100), machine.getMotorPosition());

    test_ticks(MS_TICKS(500)); // moving
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    ASSERTEQUAL(35, arduino.pulses(PC2_X_STEP_PIN) - xpulses);
    ASSERTEQUAL(MS_TICKS(3500) + 24, threadClock.ticks - machine.stroke.tStart);

    test_ticks(MS_TICKS(600)); // moving
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    ASSERTEQUAL(41, arduino.pulses(PC2_X_STEP_PIN) - xpulses);
    ASSERTEQUAL(MS_TICKS(4100) + 26, threadClock.ticks - machine.stroke.tStart);
    ASSERTQUAD(Quad<StepCoord>(141, 100, 100, 100), machine.getMotorPosition());

    test_ticks(MS_TICKS(600)); // moving
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    ASSERTEQUAL(MS_TICKS(4700) + 28, threadClock.ticks - machine.stroke.tStart);
    ASSERTQUAD(Quad<StepCoord>(147, 100, 100, 100), machine.getMotorPosition());
    ASSERTEQUAL(47, arduino.pulses(PC2_X_STEP_PIN) - xpulses);

    test_ticks(MS_TICKS(1000)); // done
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUAL(MS_TICKS(5700) + 30, threadClock.ticks - machine.stroke.tStart);
    ASSERTEQUALS(JT("{'s':0,'r':{'dvs':{'us':5000000,'x':50}},'t':5.70}\n"), Serial.output().c_str());
    ASSERTEQUAL(50, arduino.pulses(PC2_X_STEP_PIN) - xpulses);
    ASSERTQUAD(Quad<StepCoord>(150, 100, 100, 100), machine.getMotorPosition());

    cout << "TEST	: test_dvs() OK " << endl;
}

void test_error(MachineThread &mt, const char * cmd, Status status, const char *output = NULL) {
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

    test_error(mt, "{'abc':true}\n", STATUS_UNRECOGNIZED_NAME,
               "{'s':-402,'r':{'abc':true},'e':'abc','t':0.00}\n");
    test_error(mt, "{bad-json}\n", STATUS_JSON_PARSE_ERROR, "{'s':-403}\n");
    test_error(mt, "bad-json\n", STATUS_JSON_PARSE_ERROR, "{'s':-403}\n");
    test_error(mt, "{'xud':50000}\n", STATUS_VALUE_RANGE, "{'s':-133,'r':{'xud':50000},'t':0.00}\n");

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
    ASSERTEQUALS(JT("{\r\n  's': 0,\r\n  'r': {\r\n    'sysjp': true\r\n  },\r\n  't': 0.00\r\n}\n"),
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
	ASSERT(!machine.axis[0].atMin);
	ASSERT(!machine.axis[1].atMin);
	ASSERT(!machine.axis[2].atMin);
	ASSERT(!machine.axis[3].atMin);

    // TEST LONG FORM
    threadClock.ticks++;
    Serial.push(JT("{'hom':{'x':'','z':20}}\n"));
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
    ASSERTEQUAL(xpulses + 32 * 1, arduino.pulses(PC2_X_STEP_PIN));
    ASSERTEQUAL(ypulses, arduino.pulses(PC2_Y_STEP_PIN));
    ASSERTEQUAL(zpulses + 32 * 1, arduino.pulses(PC2_Z_STEP_PIN));
    ASSERTQUAD(Quad<StepCoord>(5, 100, 20, 100), mt.machine.getMotorPosition());
    ASSERTEQUALS("", Serial.output().c_str());

    threadClock.ticks++;
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    ASSERT(machine.motorAxis[0]->homing);
    ASSERT(!machine.motorAxis[1]->homing);
    ASSERT(machine.motorAxis[2]->homing);
    ASSERT(!machine.motorAxis[3]->homing);
    ASSERTEQUAL(xpulses + 32 * 2, arduino.pulses(PC2_X_STEP_PIN));
    ASSERTEQUAL(ypulses, arduino.pulses(PC2_Y_STEP_PIN));
    ASSERTEQUAL(zpulses + 32 * 2, arduino.pulses(PC2_Z_STEP_PIN));
    ASSERTQUAD(Quad<StepCoord>(5, 100, 20, 100), mt.machine.getMotorPosition());
    ASSERTEQUAL(true, machine.axis[0].homing);
    ASSERTEQUAL(false, machine.axis[1].homing);
    ASSERTEQUAL(true, machine.axis[2].homing);
    ASSERTEQUAL(LOW, arduino.getPin(PC2_X_DIR_PIN));
    ASSERTEQUAL(LOW, arduino.getPin(PC2_Y_DIR_PIN));
    ASSERTEQUAL(LOW, arduino.getPin(PC2_Z_DIR_PIN));
    ASSERTEQUALS("", Serial.output().c_str());
	ASSERT(!machine.axis[0].atMin);
	ASSERT(!machine.axis[1].atMin);
	ASSERT(!machine.axis[2].atMin);
	ASSERT(!machine.axis[3].atMin);

    arduino.setPin(PC2_X_MIN_PIN, HIGH);
    threadClock.ticks++;
    mt.loop(); // hit first limit switch
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    ASSERTEQUAL(xpulses + 32 * 2 + MICROSTEPS_DEFAULT, arduino.pulses(PC2_X_STEP_PIN));
    ASSERTEQUAL(ypulses, arduino.pulses(PC2_Y_STEP_PIN));
    ASSERTEQUAL(zpulses + 32 * 3, arduino.pulses(PC2_Z_STEP_PIN));
    ASSERTQUAD(Quad<StepCoord>(5, 100, 20, 100), mt.machine.getMotorPosition());
    ASSERTEQUAL(false, machine.axis[0].homing);
    ASSERTEQUAL(false, machine.axis[1].homing);
    ASSERTEQUAL(true, machine.axis[2].homing);
    ASSERTEQUAL(HIGH, arduino.getPin(PC2_X_DIR_PIN)); // HIGH because we backed off
    ASSERTEQUAL(LOW, arduino.getPin(PC2_Y_DIR_PIN));
    ASSERTEQUAL(LOW, arduino.getPin(PC2_Z_DIR_PIN));
    ASSERTEQUALS("", Serial.output().c_str());
	ASSERT(machine.axis[0].atMin);
	ASSERT(!machine.axis[1].atMin);
	ASSERT(!machine.axis[2].atMin);
	ASSERT(!machine.axis[3].atMin);

    arduino.setPin(PC2_Z_MIN_PIN, HIGH);
    threadClock.ticks++;
    mt.loop(); // hit final limit switch
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUAL(xpulses + 32 * 2 + MICROSTEPS_DEFAULT, arduino.pulses(PC2_X_STEP_PIN));
    ASSERTEQUAL(ypulses, arduino.pulses(PC2_Y_STEP_PIN));
    ASSERTEQUAL(zpulses + 32 * 3 + MICROSTEPS_DEFAULT, arduino.pulses(PC2_Z_STEP_PIN));
    ASSERTQUAD(Quad<StepCoord>(5, 100, 20, 100), mt.machine.getMotorPosition());
    ASSERTEQUAL(false, machine.axis[0].homing);
    ASSERTEQUAL(false, machine.axis[1].homing);
    ASSERTEQUAL(false, machine.axis[2].homing);
    ASSERTEQUAL(HIGH, arduino.getPin(PC2_X_DIR_PIN)); // HIGH because we backed off
    ASSERTEQUAL(LOW, arduino.getPin(PC2_Y_DIR_PIN));
    ASSERTEQUAL(HIGH, arduino.getPin(PC2_Z_DIR_PIN)); // HIGH because we backed off
    ASSERTEQUALS(JT("{'s':0,'r':{'hom':{'x':5,'z':20}},'t':0.00}\n"), Serial.output().c_str());
	ASSERT(machine.axis[0].atMin);
	ASSERT(!machine.axis[1].atMin);
	ASSERT(machine.axis[2].atMin);
	ASSERT(!machine.axis[3].atMin);

    // TEST ONE-AXIS SHORT FORM
    xpulses = arduino.pulses(PC2_X_STEP_PIN);
    machine.setMotorPosition(Quad<StepCoord>(100, 100, 100, 100));
    arduino.setPin(PC2_X_MIN_PIN, LOW);
    xpulses = arduino.pulses(PC2_X_STEP_PIN);
    ypulses = arduino.pulses(PC2_Y_STEP_PIN);
    zpulses = arduino.pulses(PC2_Z_STEP_PIN);
    threadClock.ticks++;
    mt.loop(); // ready for next command
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

    threadClock.ticks++;
    Serial.push(JT("{'homy':''}\n"));
    mt.loop();	// parse
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);

    threadClock.ticks++;
    mt.loop(); // initializing
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    ASSERTEQUAL(0, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
    ASSERTEQUALS("", Serial.output().c_str());
    ASSERTEQUAL(false, machine.axis[0].homing);
    ASSERTEQUAL(true, machine.axis[1].homing);
    ASSERTEQUAL(false, machine.axis[2].homing);

    threadClock.ticks++;
    mt.loop(); // moving
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    ASSERTEQUALS("", Serial.output().c_str());

    arduino.setPin(PC2_Y_MIN_PIN, HIGH);
    threadClock.ticks++;
    mt.loop(); // hit limit switch
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUAL(32 * 1 + MICROSTEPS_DEFAULT, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
    ASSERTQUAD(Quad<StepCoord>(100, 10, 100, 100), mt.machine.getMotorPosition());
    ASSERTEQUALS(JT("{'s':0,'r':{'homy':10},'t':0.00}\n"), Serial.output().c_str());

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
    Serial.push(JT("{'hom':''}\n"));
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
    ASSERTEQUAL(xpulses + 32 * 1 + MICROSTEPS_DEFAULT, arduino.pulses(PC2_X_STEP_PIN));
    ASSERTQUAD(Quad<StepCoord>(5, 100, 100, 100), mt.machine.getMotorPosition());
    ASSERTEQUALS(JT("{'s':0,'r':{'hom':{'1':5,'2':100,'3':100,'4':100}},'t':0.00}\n"),
                 Serial.output().c_str());

    cout << "TEST	: test_Home() OK " << endl;
}

void test_MachineThread() {
    cout << "TEST	: test_MachineThread() =====" << endl;

    threadRunner.clear();
    MachineThread mt;
    ASSERTQUAD(Quad<StepCoord>(0, 0, 0, 0), mt.machine.getMotorPosition());

    Serial.clear();
    Serial.push("{");
    TCNT1 = 100;
    ticks();
    mt.loop();
    ASSERTEQUAL(STATUS_WAIT_EOL, mt.status);
    ASSERTEQUALS("", Serial.output().c_str());

    const char *jsonIn = "'systc':'','xen':true,'yen':true,'zen':true,'aen':true}\n";
    Serial.push(JT(jsonIn));
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    ASSERTEQUALS("", Serial.output().c_str());

    mt.loop();
    ASSERTEQUAL(STATUS_OK, mt.status);
    const char *jsonOut =
        "{'s':0,'r':{'systc':103,'xen':true,'yen':true,'zen':true,'aen':false},'t':0.00}\n";
    ASSERTEQUALS(JT(jsonOut), Serial.output().c_str());

    mt.loop();
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

    ASSERTEQUALS("", Serial.output().c_str());
    jsonIn = "{'systc':'','dvs':{'us':512,'dp':[100,200],'1':[10,20],'2':[40,50],'3':[7,8]}}\n";
    Serial.push(JT(jsonIn));
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    ASSERTEQUALS("", Serial.output().c_str());

    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    ASSERTEQUALS("", Serial.output().c_str());
    ASSERTQUAD(Quad<StepCoord>(0, 0, 0, 0), mt.machine.getMotorPosition());

    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    ASSERTEQUALS("", Serial.output().c_str());
    ASSERTQUAD(Quad<StepCoord>(5, 20, 3, 0), mt.machine.getMotorPosition());

    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    ASSERTEQUALS("", Serial.output().c_str());
    ASSERTQUAD(Quad<StepCoord>(10, 40, 7, 0), mt.machine.getMotorPosition());

    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    ASSERTEQUALS("", Serial.output().c_str());
    ASSERTQUAD(Quad<StepCoord>(25, 85, 14, 0), mt.machine.getMotorPosition());

    Serial.clear();
    mt.loop();
    ASSERTEQUAL(STATUS_OK, mt.status);
    jsonOut =
        "{'s':0,'r':{'systc':113,'dvs':{'us':512,'dp':[100,200],'1':100,'2':200,'3':0}},'t':0.00}\n";
    ASSERTEQUALS(JT(jsonOut), Serial.output().c_str());
    ASSERTQUAD(Quad<StepCoord>(100, 200, 0, 0), mt.machine.getMotorPosition());

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
    mt.setup(PC2_RAMPS_1_4);
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
             "{'s':0,'r':{'dpycr':10,'dpycg':20,'dpycb':30,'dpyds':12,'dpydl':255},'t':0.00}\n");
    testJSON(mt.machine, mt.controller, "'\"",
             "{'dpy':{'ds':30,'dl':255,'cr':1,'cg':2,'cb':3}}",
             "{'s':25,'r':{'dpy':{'ds':30,'dl':255,'cr':1,'cg':2,'cb':3}},'t':0.00}\n", STATUS_WAIT_BUSY);

    cout << "TEST	: test_Display() OK " << endl;
}

void test_ph5() {
    cout << "TEST	: test_ph5() =====" << endl;

    MachineThread mt = test_setup();
    Machine &machine = mt.machine;
    arduino.timer1(1);
    StrokeBuilder sb;
    ASSERTQUAD(Quad<StepCoord>(), machine.getMotorPosition());
    Status status = sb.buildLine(machine.stroke, Quad<StepCoord>(6400, 3200, 1600, 0));
    ASSERTEQUAL(STATUS_OK, status);
    ASSERTEQUAL(32, machine.stroke.length);
    ASSERTEQUAL(0, machine.stroke.seg[0].value[0]);
    ASSERTEQUAL(3, machine.stroke.seg[1].value[0]);
    ASSERTEQUAL(10, machine.stroke.seg[2].value[0]);
    ASSERTEQUAL(16, machine.stroke.seg[3].value[0]);
    ASSERTEQUAL(27, machine.stroke.seg[4].value[0]);
    ASSERTEQUAL(35, machine.stroke.seg[5].value[0]);
    ASSERTEQUAL(40, machine.stroke.seg[6].value[0]); // peak velocity
    ASSERTEQUAL(46, machine.stroke.seg[7].value[0]);
    ASSERTEQUAL(46, machine.stroke.seg[8].value[0]);
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
    ASSERTEQUAL(6400, arduino.pulses(PC2_X_STEP_PIN) - xpulses);
    ASSERTEQUAL(15623, i);
    ASSERTEQUAL(STATUS_OK, status);
    ASSERTQUAD(Quad<StepCoord>(6400, 3200, 1600, 0), machine.getMotorPosition());

    // TEST: short line
    xpulses = arduino.pulses(PC2_X_STEP_PIN);
    int32_t xdirpulses = arduino.pulses(PC2_X_DIR_PIN);
    ASSERTEQUAL(0, arduino.pulses(PC2_X_STEP_PIN) - xpulses);
    Serial.push(JT("{'tstph':{'pu':3200,'tv':'','sg':'','mv':'','lp':''}}\n"));
    mt.loop();	// command.parse
    ASSERTEQUAL(true, machine.stroke.isDone());
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    ASSERTEQUAL(0, Serial.available()); // expected parse
    ASSERTEQUAL(0, arduino.pulses(PC2_X_STEP_PIN) - xpulses);

    mt.loop();	// command.process
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    ASSERTEQUAL(true, machine.stroke.isDone());
    ASSERTEQUALS(JT(""), Serial.output().c_str());
    ASSERTEQUAL(1, arduino.pulses(PC2_X_DIR_PIN) - xdirpulses);	// reversing once
    ASSERTEQUAL(LOW, arduino.getPin(PC2_X_DIR_PIN));	// reversing
    ASSERTEQUAL(6400, arduino.pulses(PC2_X_STEP_PIN) - xpulses);
    ASSERTQUAD(Quad<StepCoord>(0, 0, 0, 0), machine.getMotorPosition());

    mt.loop();	// command.process (second stroke)
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    ASSERTEQUAL(true, machine.stroke.isDone());
    ASSERTEQUALS(JT(""), Serial.output().c_str());
    ASSERTEQUAL(2, arduino.pulses(PC2_X_DIR_PIN) - xdirpulses);
    ASSERTEQUAL(LOW, arduino.getPin(PC2_X_DIR_PIN));	// advancing
    ASSERTEQUAL(12800, arduino.pulses(PC2_X_STEP_PIN) - xpulses);
    ASSERTQUAD(Quad<StepCoord>(0, 0, 0, 0), machine.getMotorPosition());

    Serial.push("\n"); // terminate
    xpulses = arduino.pulses(PC2_X_STEP_PIN);
    mt.loop();	// command.process
    ASSERTEQUAL(STATUS_WAIT_CANCELLED, mt.status);
    ASSERTEQUALS(
        JT("{'s':-901,'r':{'tstph':{'pu':3200,'tv':0.70,'sg':16,'mv':12800,"\
           "'lp':26144,'pp':7611.1,'ts':0.837,'tp':0.837}},'t':3.35}\n"),
        Serial.output().c_str());
    ASSERTEQUAL(0, arduino.pulses(PC2_X_STEP_PIN) - xpulses);
    mt.loop(); // idle

    // TEST: Fast acceleration
    xpulses = arduino.pulses(PC2_X_STEP_PIN);
    xdirpulses = arduino.pulses(PC2_X_DIR_PIN);
    Serial.push(JT("{'tstph':{'pu':'','tv':0.01,'sg':'','mv':'','lp':''}}\n"));
    mt.loop();	// command.parse
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);

    mt.loop();	// command.process
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    ASSERTEQUAL(true, machine.stroke.isDone());
    ASSERTEQUALS(JT(""), Serial.output().c_str());
    ASSERTEQUAL(1, arduino.pulses(PC2_X_DIR_PIN) - xdirpulses);	// never reversing
    ASSERTEQUAL(LOW, arduino.getPin(PC2_X_DIR_PIN));	// advancing
    ASSERTEQUAL(12800, arduino.pulses(PC2_X_STEP_PIN) - xpulses);
    ASSERTQUAD(Quad<StepCoord>(0, 0, 0, 0), machine.getMotorPosition());

    Serial.push("\n"); // terminate
    xpulses = arduino.pulses(PC2_X_STEP_PIN);
    mt.loop();	// command.process
    ASSERTEQUAL(STATUS_WAIT_CANCELLED, mt.status);
    ASSERTEQUALS(
        JT("{'s':-901,'r':{'tstph':{'pu':6400,'tv':0.01,'sg':129,'mv':12800,"\
           "'lp':15936,'pp':12899.6,'ts':0.510,'tp':0.510}},'t':1.02}\n"),
        Serial.output().c_str());
    ASSERTEQUAL(0, arduino.pulses(PC2_X_STEP_PIN) - xpulses);
    mt.loop(); // idle

    // TEST: max velocity
    xpulses = arduino.pulses(PC2_X_STEP_PIN);
    xdirpulses = arduino.pulses(PC2_X_DIR_PIN);
    Serial.push(JT("{'tstph':{'pu':'','tv':0.1,'sg':'','mv':40000,'lp':''}}\n"));
    mt.loop();	// command.parse
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);

    mt.loop();	// command.process
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    ASSERTEQUAL(true, machine.stroke.isDone());
    ASSERTEQUALS(JT(""), Serial.output().c_str());
    ASSERTEQUAL(1, arduino.pulses(PC2_X_DIR_PIN) - xdirpulses);	// never reversing
    ASSERTEQUAL(LOW, arduino.getPin(PC2_X_DIR_PIN));	// advancing
    ASSERTEQUAL(12800, arduino.pulses(PC2_X_STEP_PIN) - xpulses);
    ASSERTQUAD(Quad<StepCoord>(0, 0, 0, 0), machine.getMotorPosition());

    Serial.push("\n"); // terminate
    xpulses = arduino.pulses(PC2_X_STEP_PIN);
    mt.loop();	// command.process
    ASSERTEQUAL(STATUS_WAIT_CANCELLED, mt.status);
    ASSERTEQUALS(
        JT("{'s':-901,'r':{'tstph':{'pu':6400,'tv':0.10,'sg':32,'mv':40000,"\
           "'lp':8124,'pp':39995.1,'ts':0.260,'tp':0.260}},'t':0.52}\n"),
        Serial.output().c_str());
    ASSERTEQUAL(0, arduino.pulses(PC2_X_STEP_PIN) - xpulses);
    mt.loop(); // idle

    // TEST: just one pulse
    xpulses = arduino.pulses(PC2_X_STEP_PIN);
    xdirpulses = arduino.pulses(PC2_X_DIR_PIN);
    Serial.push(JT("{'tstph':{'pu':1,'tv':0.1,'sg':'','mv':40000,'lp':''}}\n"));
    mt.loop();	// command.parse
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);

    mt.loop();	// command.process
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    ASSERTEQUAL(true, machine.stroke.isDone());
    ASSERTEQUALS(JT(""), Serial.output().c_str());
    ASSERTEQUAL(1, arduino.pulses(PC2_X_DIR_PIN) - xdirpulses);	// never reversing
    ASSERTEQUAL(LOW, arduino.getPin(PC2_X_DIR_PIN));	// advancing
    ASSERTEQUAL(2, arduino.pulses(PC2_X_STEP_PIN) - xpulses);
    ASSERTQUAD(Quad<StepCoord>(0, 0, 0, 0), machine.getMotorPosition());

    Serial.push("\n"); // terminate
    xpulses = arduino.pulses(PC2_X_STEP_PIN);
    mt.loop();	// command.process
    ASSERTEQUAL(STATUS_WAIT_CANCELLED, mt.status);
    ASSERTEQUALS(
        JT("{'s':-901,'r':{'tstph':{'pu':1,'tv':0.10,'sg':16,'mv':40000,"\
           "'lp':98,'pp':5000.0,'ts':0.003,'tp':0.003}},'t':0.01}\n"),
        Serial.output().c_str());
    ASSERTEQUAL(0, arduino.pulses(PC2_X_STEP_PIN) - xpulses);
    mt.loop(); // idle

    // TEST: long fast line
    xpulses = arduino.pulses(PC2_X_STEP_PIN);
    xdirpulses = arduino.pulses(PC2_X_DIR_PIN);
    Serial.push(JT("{'tstph':{'pu':32000,'tv':0.15,'sg':'','mv':16000,'lp':''}}\n"));
    mt.loop();	// command.parse
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);

    mt.loop();	// command.process
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    ASSERTEQUAL(true, machine.stroke.isDone());
    ASSERTEQUALS(JT(""), Serial.output().c_str());
    ASSERTEQUAL(1, arduino.pulses(PC2_X_DIR_PIN) - xdirpulses);	// never reversing
    ASSERTEQUAL(LOW, arduino.getPin(PC2_X_DIR_PIN));	// advancing
    ASSERTEQUAL(64000, arduino.pulses(PC2_X_STEP_PIN) - xpulses);
    ASSERTQUAD(Quad<StepCoord>(0, 0, 0, 0), machine.getMotorPosition());

    Serial.push("\n"); // terminate
    xpulses = arduino.pulses(PC2_X_STEP_PIN);
    mt.loop();	// command.process
    ASSERTEQUAL(STATUS_WAIT_CANCELLED, mt.status);
    ASSERTEQUALS(
        JT("{'s':-901,'r':{'tstph':{'pu':32000,'tv':0.15,'sg':129,'mv':16000,"\
           "'lp':67186,'pp':16019.9,'ts':2.150,'tp':2.150}},'t':4.30}\n"),
        Serial.output().c_str());
    ASSERTEQUAL(0, arduino.pulses(PC2_X_STEP_PIN) - xpulses);
    mt.loop(); // idle

    // TEST: long slow line
    xpulses = arduino.pulses(PC2_X_STEP_PIN);
    xdirpulses = arduino.pulses(PC2_X_DIR_PIN);
    Serial.push(JT("{'tstph':{'pu':32000,'tv':0.2,'sg':'','mv':4000,'lp':''}}\n"));
    mt.loop();	// command.parse
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);

    mt.loop();	// command.process
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    ASSERTEQUAL(true, machine.stroke.isDone());
    ASSERTEQUALS(JT(""), Serial.output().c_str());
    ASSERTEQUAL(1, arduino.pulses(PC2_X_DIR_PIN) - xdirpulses);	// never reversing
    ASSERTEQUAL(LOW, arduino.getPin(PC2_X_DIR_PIN));	// advancing
    ASSERTEQUAL(64000, arduino.pulses(PC2_X_STEP_PIN) - xpulses);
    ASSERTQUAD(Quad<StepCoord>(0, 0, 0, 0), machine.getMotorPosition());

    Serial.push("\n"); // terminate
    xpulses = arduino.pulses(PC2_X_STEP_PIN);
    mt.loop();	// command.process
    ASSERTEQUAL(STATUS_WAIT_CANCELLED, mt.status);
    ASSERTEQUALS(
        JT("{'s':-901,'r':{'tstph':{'pu':32000,'tv':0.20,'sg':129,'mv':4000,"\
           "'lp':256250,'pp':4011.6,'ts':8.200,'tp':8.200}},'t':16.40}\n"),
        Serial.output().c_str());
    ASSERTEQUAL(0, arduino.pulses(PC2_X_STEP_PIN) - xpulses);
    mt.loop(); // idle

    // TEST: short and fast
    xpulses = arduino.pulses(PC2_X_STEP_PIN);
    xdirpulses = arduino.pulses(PC2_X_DIR_PIN);
    Serial.push(JT("{'tstph':{'pu':1600,'tv':0.3,'mv':16000}}\n"));
    mt.loop();	// command.parse
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);

    mt.loop();	// command.process
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    ASSERTEQUAL(true, machine.stroke.isDone());
    ASSERTEQUALS(JT(""), Serial.output().c_str());
    ASSERTEQUAL(1, arduino.pulses(PC2_X_DIR_PIN) - xdirpulses);	// never reversing
    ASSERTEQUAL(LOW, arduino.getPin(PC2_X_DIR_PIN));	// advancing
    ASSERTEQUAL(3200, arduino.pulses(PC2_X_STEP_PIN) - xpulses);
    ASSERTQUAD(Quad<StepCoord>(0, 0, 0, 0), machine.getMotorPosition());

    Serial.push("\n"); // terminate
    xpulses = arduino.pulses(PC2_X_STEP_PIN);
    mt.loop();	// command.process
    ASSERTEQUAL(STATUS_WAIT_CANCELLED, mt.status);
    ASSERTEQUALS(
        JT("{'s':-901,'r':{'tstph':{'pu':1600,'tv':0.30,'mv':16000,'lp':10824,"\
           "'pp':9190.8,'sg':16,'ts':0.346,'tp':0.346}},'t':0.69}\n"),
        Serial.output().c_str());
    ASSERTEQUAL(0, arduino.pulses(PC2_X_STEP_PIN) - xpulses);
    mt.loop(); // idle

    cout << "TEST	: test_ph5() OK " << endl;
}

void test_command_array() {
    cout << "TEST	: test_command_arraytest_pnp() =====" << endl;

    MachineThread mt = test_setup();
    Machine &machine = mt.machine;
    machine.setMotorPosition(Quad<StepCoord>(1,2,3,4));

	// TEST two command array
    Serial.push(JT("[{'xpo':''},{'ypo':''}]\n"));
    test_ticks(1); // parse JsonCommand
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    test_ticks(1); // process first command
    ASSERTEQUALS(JT(""), Serial.output().c_str());
    ASSERTEQUAL(STATUS_BUSY_OK, mt.status);
	test_ticks(1); // process second command
    ASSERTEQUALS(JT(""), Serial.output().c_str());
    ASSERTEQUAL(STATUS_BUSY_OK, mt.status);
	test_ticks(1);
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUALS(JT("{'s':0,'r':{'ypo':2},'t':0.00}\n"), Serial.output().c_str());
    test_ticks(1); // done
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

	// TEST serial interrupt of command array
    Serial.push(JT("[{'xpo':''},{'ypo':''},{'zpo':''}]\n"));
    test_ticks(1); // parse JsonCommand
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    test_ticks(1); // process first command
    ASSERTEQUALS(JT(""), Serial.output().c_str());
    ASSERTEQUAL(STATUS_BUSY_OK, mt.status);
	Serial.push("\n");
	test_ticks(1);
    ASSERTEQUAL(STATUS_WAIT_CANCELLED, mt.status);
    ASSERTEQUALS(JT("{'s':-901,'r':{'xpo':1},'t':0.00}\n"), Serial.output().c_str());

    cout << "TEST	: test_command_arraytest_pnp() OK " << endl;
}

void test_eeprom() {
    cout << "TEST	: test_eeprom() =====" << endl;

    MachineThread mt = test_setup();
    Machine &machine = mt.machine;

    Serial.push(JT("{'sysee':''}\n"));
    test_ticks(1); // parse JsonCommand
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    test_ticks(1); // process command
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUALS(JT("{'s':0,'r':{'sysee':''},'t':0.00}\n"), Serial.output().c_str());
    test_ticks(1); // done
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

    Serial.push(JT("{'sysee':'{\\\"sysv\\\":\\\"\\\"}'}\n"));
    test_ticks(1); // parse JsonCommand
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    test_ticks(1); // process command
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUALS(JT("{'s':0,'r':{'sysee':'{\\\"sysv\\\":\\\"\\\"}'},'t':0.00}\n"), Serial.output().c_str());
    test_ticks(1); // done
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);
	uint8_t * addr = 0;
	ASSERTEQUAL('{', eeprom_read_byte(addr++));
	ASSERTEQUAL('"', eeprom_read_byte(addr++));

    cout << "TEST	: test_eeprom() OK " << endl;
}

int main(int argc, char *argv[]) {
    LOGINFO3("INFO	: FireStep test v%d.%d.%d",
             VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
    firelog_level(FIRELOG_TRACE);

    for (int i=0; i<argc; i++) {
        cout << "argv[" << i << "]:\"" << argv[i] << "\"" << endl;
    }

    // test first

    if (argc > 1 && strcmp("-1", argv[1]) == 0) {
		test_Home();
    } else {
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
        test_stroke_endpos();
		test_command_array();
        test_pnp();
		test_eeprom();
    }

    cout << "TEST	: END OF TEST main()" << endl;
}

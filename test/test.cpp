#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include "FireLog.h"
#include "FireUtils.hpp"
#include "version.h"
#include "Arduino.h"
#include "MachineThread.h"
#include "build.h"

byte lastByte;

using namespace firestep;
using namespace ArduinoJson;

#define ASSERTQUAD(expected,actual) ASSERTEQUALS( expected.toString().c_str(), actual.toString().c_str() );

void test_tick(int ticks) {
    arduino.timer1(ticks);
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
    tc.age++;
    ASSERTEQUAL(1, tc.ticks - tcLast);
    tc.generation++;
    ASSERTEQUAL(65537, tc.ticks - tcLast);
    ASSERTEQUAL(64, TICK_MICROSECONDS);

    threadRunner.setup(LED_PIN_RED, LED_PIN_GRN);
    monitor.verbose = false;

    ASSERTEQUAL(16000, MS_CYCLES(1));
    ASSERTEQUAL(15625, MS_TIMER_CYCLES(1000));
    arduino.dump();
    //ASSERTEQUALS(" CLKPR:0 nThreads:1\n", Serial.output().c_str());
    ASSERTEQUAL(OUTPUT, arduino.getPinMode(LED_PIN_RED));
    ASSERTEQUAL(LOW, digitalRead(LED_PIN_RED));
    ASSERTEQUAL(OUTPUT, arduino.getPinMode(LED_PIN_GRN));
    ASSERTEQUAL(LOW, digitalRead(LED_PIN_GRN));
    ASSERTEQUAL(0x0000, TIMSK1); 	// Timer/Counter1 interrupt mask; no interrupts
    ASSERTEQUAL(0x0000, TCCR1A);	// Timer/Counter1 normal port operation
    ASSERTEQUAL(0x0005, TCCR1B);	// Timer/Counter1 active; prescale 1024
    ASSERTEQUAL(NOVALUE, SREGI); 	// Global interrupts enabled
    test_tick(1);
    Ticks lastClock = ticks();
    test_tick(1);
    ASSERTEQUAL(1000000 / 15625, MicrosecondsSince(lastClock));

    cout << "TEST	: test_Thread() OK " << endl;
}

void test_command(const char *cmd, const char* expected) {
    Serial.clear();
    Serial.push(cmd);
    ASSERTEQUAL(strlen(cmd), Serial.available());
    test_tick(MS_TIMER_CYCLES(1));
    test_tick(MS_TIMER_CYCLES(1));
    test_tick(MS_TIMER_CYCLES(1));
    test_tick(MS_TIMER_CYCLES(1));
    ASSERTEQUALS(expected, Serial.output().c_str());
}

void test_Machine() {
    cout << "TEST	: test_Machine() =====" << endl;
    arduino.clear();

    MachineThread machThread;
    machThread.setup();
    threadRunner.setup(LED_PIN_RED, LED_PIN_GRN);
    monitor.verbose = false;

    arduino.dump();
    Serial.clear();
#ifdef LEGACY
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
#endif
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
    test_tick(1);
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
    ASSERTEQUAL(STATUS_JSON_PARSED, cmd1.parse("{\"sys\":\"\"}"));
    ASSERTEQUAL(STATUS_JSON_PARSED, cmd1.getStatus());
    ASSERT(cmd1.isValid());
    JsonVariant& sys = cmd1.requestRoot()["sys"];
    ASSERTEQUALS("", sys);
    ASSERT(!sys.is<double>());
    ASSERT(!sys.is<long>());
    ASSERT(sys.is<const char *>());

    JsonCommand cmd2;
    ASSERTEQUAL(STATUS_JSON_PARSED, cmd2.parse("{\"x\":123,\"y\":2.3}"));
    ASSERTEQUAL(STATUS_JSON_PARSED, cmd2.getStatus());
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
    ASSERTEQUAL(STATUS_SERIAL_EOL_WAIT, cmd3.parse());
    Serial.push(json2);
    ASSERTEQUAL(STATUS_JSON_PARSED, cmd3.parse());
    ASSERTEQUAL(STATUS_JSON_PARSED, cmd3.getStatus());
    ASSERT(cmd3.isValid());
    ASSERT(cmd3.requestRoot().success());
    x = cmd3.requestRoot()["x"];
    ASSERTEQUAL(-0.123, x);

    Serial.clear();
    cmd3.requestRoot().printTo(Serial);
    ASSERTEQUALS("{\"x\":-0.123}", Serial.output().c_str());

    Serial.clear();
    cmd3.response().printTo(Serial);
    ASSERTEQUALS("{\"s\":1,\"r\":{\"x\":-0.123}}", Serial.output().c_str());

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
	const char *jsonOut, Status status=STATUS_OK) 
{
    Serial.clear();
    string jo(jsonOut);
    for (int i = 0; i < replace.size(); i += 2) {
        char cmatch = replace[i];
        char creplace = replace[i + 1];
        replaceChar(jo, cmatch, creplace);
    }
    arduino.timer1(1);
    threadClock.ticks++;
    Status actualStatus = jc.process(machine, jcmd);
	ASSERTEQUAL(status, actualStatus);
    jcmd.response().printTo(Serial);
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

JsonCommand testJSON(Machine& machine, JsonController &jc, string replace, const char *jsonIn, 
	const char* jsonOut, Status processStatus=STATUS_OK) 
{
    string ji(jsonTemplate(jsonIn, replace));
    JsonCommand jcmd;
    Status status = jcmd.parse(ji.c_str());
    ASSERTEQUAL(STATUS_JSON_PARSED, status);
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
    testJSON(machine, jc, replace, "{'?':''}", "{'s':0,'r':{'?':{'ma':!}}}");
    testJSON(machine, jc, replace, "{'?ma':''}", "{'s':0,'r':{'?ma':!}}");
    testJSON(machine, jc, replace, "{'?ma':4}", "{'s':0,'r':{'?ma':4}}");
    testJSON(machine, jc, replace, "{'?ma':''}", "{'s':0,'r':{'?ma':4}}");
    testJSON(machine, jc, replace, "{'?':{'ma':''}}", "{'s':0,'r':{'?':{'ma':4}}}");
    testJSON(machine, jc, replace, "{'?':{'ma':!}}", "{'s':0,'r':{'?':{'ma':!}}}");
    testJSON(machine, jc, replace, "{'?':{'ma':''}}", "{'s':0,'r':{'?':{'ma':!}}}");
}

void test_JsonController_axis(Machine& machine, JsonController &jc, char axis) {
    string replace;
    replace.push_back('\'');
    replace.push_back('"');
    replace.push_back('?');
    replace.push_back(axis);
    testJSON(machine, jc, replace, "{'?tn':''}", "{'s':0,'r':{'?tn':0}}");		// default
    testJSON(machine, jc, replace, "{'?tn':111}", "{'s':0,'r':{'?tn':111}}");
    testJSON(machine, jc, replace, "{'?tn':''}", "{'s':0,'r':{'?tn':111}}");
    testJSON(machine, jc, replace, "{'?':{'tn':''}}", "{'s':0,'r':{'?':{'tn':111}}}");
    testJSON(machine, jc, replace, "{'?':{'tn':0}}", "{'s':0,'r':{'?':{'tn':0}}}");
    testJSON(machine, jc, replace, "{'?':{'tn':''}}", "{'s':0,'r':{'?':{'tn':0}}}");
    testJSON(machine, jc, replace, "{'?tm':''}", "{'s':0,'r':{'?tm':10000}}");  	// default
    testJSON(machine, jc, replace, "{'?tm':222}", "{'s':0,'r':{'?tm':222}}");
    testJSON(machine, jc, replace, "{'?tm':''}", "{'s':0,'r':{'?tm':222}}");
    testJSON(machine, jc, replace, "{'?':{'tm':''}}", "{'s':0,'r':{'?':{'tm':222}}}");
    testJSON(machine, jc, replace, "{'?':{'tm':10000}}", "{'s':0,'r':{'?':{'tm':10000}}}");
    testJSON(machine, jc, replace, "{'?':{'tm':''}}", "{'s':0,'r':{'?':{'tm':10000}}}");
    testJSON(machine, jc, replace, "{'?am':''}", "{'s':0,'r':{'?am':1}}");  	// default
    testJSON(machine, jc, replace, "{'?am':333}", "{'s':0,'r':{'?am':77}}");	// out of range
    testJSON(machine, jc, replace, "{'?am':''}", "{'s':0,'r':{'?am':77}}");
    testJSON(machine, jc, replace, "{'?':{'am':''}}", "{'s':0,'r':{'?':{'am':77}}}");
    testJSON(machine, jc, replace, "{'?':{'am':1}}", "{'s':0,'r':{'?':{'am':1}}}");
    testJSON(machine, jc, replace, "{'?':{'am':''}}", "{'s':0,'r':{'?':{'am':1}}}");
    testJSON(machine, jc, replace, "{'?':{'mi':''}}", "{'s':0,'r':{'?':{'mi':16}}}");
    testJSON(machine, jc, replace, "{'?':{'mi':1}}", "{'s':0,'r':{'?':{'mi':1}}}");
    testJSON(machine, jc, replace, "{'?':{'mi':''}}", "{'s':0,'r':{'?':{'mi':1}}}");
    testJSON(machine, jc, replace, "{'?':{'mi':16}}", "{'s':0,'r':{'?':{'mi':16}}}");
    testJSON(machine, jc, replace, "{'?':{'in':''}}", "{'s':0,'r':{'?':{'in':0}}}");
    testJSON(machine, jc, replace, "{'?':{'in':1}}", "{'s':0,'r':{'?':{'in':1}}}");
    testJSON(machine, jc, replace, "{'?':{'in':''}}", "{'s':0,'r':{'?':{'in':1}}}");
    testJSON(machine, jc, replace, "{'?':{'in':0}}", "{'s':0,'r':{'?':{'in':0}}}");
    testJSON(machine, jc, replace, "{'?':{'pm':''}}", "{'s':0,'r':{'?':{'pm':0}}}");
    testJSON(machine, jc, replace, "{'?':{'pm':3}}", "{'s':0,'r':{'?':{'pm':3}}}");
    testJSON(machine, jc, replace, "{'?':{'pm':''}}", "{'s':0,'r':{'?':{'pm':3}}}");
    testJSON(machine, jc, replace, "{'?':{'pm':0}}", "{'s':0,'r':{'?':{'pm':0}}}");
    testJSON(machine, jc, replace, "{'?':{'sa':''}}", "{'s':0,'r':{'?':{'sa':1.80}}}");
    testJSON(machine, jc, replace, "{'?':{'sa':0.9}}", "{'s':0,'r':{'?':{'sa':0.90}}}");
    testJSON(machine, jc, replace, "{'?':{'sa':''}}", "{'s':0,'r':{'?':{'sa':0.90}}}");
    testJSON(machine, jc, replace, "{'?':{'sa':1.8}}", "{'s':0,'r':{'?':{'sa':1.80}}}");

    testJSON(machine, jc, replace,
             "{'x':''}",
             "{'s':0,'r':{'x':{'am':1,'in':0,'ln':false,'mi':16,'pd':22,'pe':14,'pm':0,"\
             "'pn':21,'po':0,'ps':23,'sa':1.80,'tm':10000,'tn':0}}}");
    testJSON(machine, jc, replace,
             "{'y':''}",
             "{'s':0,'r':{'y':{'am':1,'in':0,'ln':false,'mi':16,'pd':3,'pe':14,'pm':0,"\
             "'pn':2,'po':0,'ps':4,'sa':1.80,'tm':10000,'tn':0}}}");
    testJSON(machine, jc, replace,
             "{'z':''}",
             "{'s':0,'r':{'z':{'am':1,'in':0,'ln':false,'mi':16,'pd':12,'pe':14,'pm':0,"\
             "'pn':11,'po':0,'ps':13,'sa':1.80,'tm':10000,'tn':0}}}");
}

void test_JsonController_machinePosition(Machine& machine, JsonController &jc) {
    string replace;
    replace.push_back('\'');
    replace.push_back('"');
    testJSON(machine, jc, replace, "{'spo':''}", "{'s':0,'r':{'spo':{'x':0,'y':0,'z':0,'a':0,'b':0,'c':0}}}");
    testJSON(machine, jc, replace, "{'spox':''}", "{'s':0,'r':{'spox':0}}");
    testJSON(machine, jc, replace, "{'spox':32760}", "{'s':0,'r':{'spox':32760}}");
    testJSON(machine, jc, replace, "{'spox':''}", "{'s':0,'r':{'spox':32760}}");
    testJSON(machine, jc, replace, "{'spox':-32760}", "{'s':0,'r':{'spox':-32760}}");
    testJSON(machine, jc, replace, "{'spox':''}", "{'s':0,'r':{'spox':-32760}}");
    testJSON(machine, jc, replace, "{'spoy':''}", "{'s':0,'r':{'spoy':0}}");
    testJSON(machine, jc, replace, "{'spoy':32761}", "{'s':0,'r':{'spoy':32761}}");
    testJSON(machine, jc, replace, "{'spoy':''}", "{'s':0,'r':{'spoy':32761}}");
    testJSON(machine, jc, replace, "{'spoy':-32761}", "{'s':0,'r':{'spoy':-32761}}");
    testJSON(machine, jc, replace, "{'spoy':''}", "{'s':0,'r':{'spoy':-32761}}");
    testJSON(machine, jc, replace, "{'spoz':''}", "{'s':0,'r':{'spoz':0}}");
    testJSON(machine, jc, replace, "{'spoz':32762}", "{'s':0,'r':{'spoz':32762}}");
    testJSON(machine, jc, replace, "{'spoz':''}", "{'s':0,'r':{'spoz':32762}}");
    testJSON(machine, jc, replace, "{'spoz':-32762}", "{'s':0,'r':{'spoz':-32762}}");
    testJSON(machine, jc, replace, "{'spoz':''}", "{'s':0,'r':{'spoz':-32762}}");
    testJSON(machine, jc, replace, "{'spoa':''}", "{'s':0,'r':{'spoa':0}}");
    testJSON(machine, jc, replace, "{'spoa':32763}", "{'s':0,'r':{'spoa':32763}}");
    testJSON(machine, jc, replace, "{'spoa':''}", "{'s':0,'r':{'spoa':32763}}");
    testJSON(machine, jc, replace, "{'spoa':-32763}", "{'s':0,'r':{'spoa':-32763}}");
    testJSON(machine, jc, replace, "{'spoa':''}", "{'s':0,'r':{'spoa':-32763}}");
    testJSON(machine, jc, replace, "{'spob':''}", "{'s':0,'r':{'spob':0}}");
    testJSON(machine, jc, replace, "{'spob':32764}", "{'s':0,'r':{'spob':32764}}");
    testJSON(machine, jc, replace, "{'spob':''}", "{'s':0,'r':{'spob':32764}}");
    testJSON(machine, jc, replace, "{'spob':-32764}", "{'s':0,'r':{'spob':-32764}}");
    testJSON(machine, jc, replace, "{'spob':''}", "{'s':0,'r':{'spob':-32764}}");
    testJSON(machine, jc, replace, "{'spoc':''}", "{'s':0,'r':{'spoc':0}}");
    testJSON(machine, jc, replace, "{'spoc':32765}", "{'s':0,'r':{'spoc':32765}}");
    testJSON(machine, jc, replace, "{'spoc':''}", "{'s':0,'r':{'spoc':32765}}");
    testJSON(machine, jc, replace, "{'spoc':-32765}", "{'s':0,'r':{'spoc':-32765}}");
    testJSON(machine, jc, replace, "{'spoc':''}", "{'s':0,'r':{'spoc':-32765}}");
    testJSON(machine, jc, replace, "{'spo':''}",
             "{'s':0,'r':{'spo':{'x':-32760,'y':-32761,'z':-32762,'a':-32763,'b':-32764,'c':-32765}}}");
}

void test_JsonController_stroke(Machine& machine, JsonController &jc) {
    string replace;
    replace.push_back('\'');
    replace.push_back('"');

    // Set machine to known position and state
	threadClock.ticks = 99;
    digitalWrite(machine.axis[0].pinMin, 0);
    digitalWrite(machine.axis[1].pinMin, 0);
    digitalWrite(machine.axis[2].pinMin, 0);
    digitalWrite(machine.axis[0].pinEnable, 1);
    digitalWrite(machine.axis[1].pinEnable, 1);
    digitalWrite(machine.axis[2].pinEnable, 1);
#define STROKE_CONFIG "{"\
			"'xtm':10000,'xtn':0,'xpo':5,'xln':false,"\
			"'ytm':10000,'ytn':0,'ypo':5,'yln':false,"\
			"'ztm':10000,'ztn':0,'zpo':5,'zln':false,"\
			"'atm':10000,'atn':0,'apo':5,'aln':false,'aps':255}"
    string jconfig = STROKE_CONFIG;
    testJSON(machine, jc, replace, jconfig.c_str(), "{'s':0,'r':" STROKE_CONFIG "}");
    ASSERTQUAD(Quad<StepCoord>(5, 5, 5, 5), machine.motorPosition());
    Ticks tStart = 101;
    ASSERTEQUAL(tStart, threadClock.ticks+1);

    // parse and initialize stroke
    JsonCommand jcmd =
        testJSON(machine, jc, replace,
                 "{'systc':'','dvs':{'us':123,'dp':[10,20],'s1':[1,2],'s2':[4,5],'s3':[7,8],'s4':[-10,-11]}}",
                 "{'s':2,'r':{'systc':101,'dvs':{'us':123,'dp':[10,20],'s1':0,'s2':0,'s3':0,'s4':0}}}",
                 STATUS_PROCESSING);
    ASSERTQUAD(Quad<StepCoord>(0, 0, 0, 0), machine.stroke.position());
    ASSERTQUAD(Quad<StepCoord>(5, 5, 5, 5), machine.motorPosition());
    ASSERTEQUAL(tStart, threadClock.ticks);
    ASSERTEQUAL(tStart, machine.stroke.tStart);
    ASSERTEQUAL(2, machine.stroke.dtTotal);
    ASSERTQUAD(Quad<StepCoord>(10, 20, 0, 0), machine.stroke.dEndPos);
	size_t reqAvail = jcmd.requestAvailable();
	size_t resAvail = jcmd.responseAvailable();

    // traverse first stroke segment
    ASSERTEQUAL(0, digitalRead(11));
    testJSON_process(machine, jc, jcmd, replace,
                     "{'s':2,'r':{'systc':102,'dvs':{'us':123,'dp':[10,20],'s1':1,'s2':4,'s3':7,'s4':-10}}}",
					 STATUS_PROCESSING);
    ASSERTEQUAL(0, digitalRead(11));
    ASSERTQUAD(Quad<StepCoord>(1, 4, 7, -10), machine.stroke.position());
    ASSERTQUAD(Quad<StepCoord>(6, 9, 12, 5), machine.motorPosition()); // axis a NOPIN inactive
	ASSERTEQUAL(reqAvail, jcmd.requestAvailable());
	ASSERTEQUAL(resAvail, jcmd.responseAvailable());

    // traverse final stroke segment
    testJSON_process(machine, jc, jcmd, replace,
                     "{'s':2,'r':{'systc':103,'dvs':{'us':123,'dp':[10,20],'s1':10,'s2':20,'s3':0,'s4':0}}}",
                     STATUS_PROCESSING);
    ASSERTQUAD(Quad<StepCoord>(10, 20, 0, 0), machine.stroke.position());
    ASSERTQUAD(Quad<StepCoord>(15, 25, 5, 5), machine.motorPosition()); // axis a is NOPIN inactive
	ASSERTEQUAL(reqAvail, jcmd.requestAvailable());
	ASSERTEQUAL(resAvail, jcmd.responseAvailable());

    // finalize stroke
    testJSON_process(machine, jc, jcmd, replace,
                     "{'s':0,'r':{'systc':104,'dvs':{'us':123,'dp':[10,20],'s1':10,'s2':20,'s3':0,'s4':0}}}",
					 STATUS_OK);
    ASSERTQUAD(Quad<StepCoord>(10, 20, 0, 0), machine.stroke.position());
    ASSERTQUAD(Quad<StepCoord>(15, 25, 5, 5), machine.motorPosition()); // axis a is NOPIN inactive
    ASSERTEQUAL(tStart + 3, threadClock.ticks);
	ASSERTEQUAL(reqAvail, jcmd.requestAvailable());
	ASSERTEQUAL(resAvail, jcmd.responseAvailable());

    // Test largest valid stroke
    char buf[5000];
    string segs = "[-127";
    for (int i = 1; i < SEGMENT_COUNT * 5; i++) {
        segs += ",-127";
    }
    segs += "]";
    string jlarge = "{'dvs':{";
    jlarge += "'s1':";
    jlarge += segs;
    jlarge += "}}";
    string jlargein = jsonTemplate(jlarge.c_str());
    JsonCommand jcmd2;
    ASSERTEQUAL(STATUS_JSON_PARSE_ERROR, jcmd2.parse(jlargein.c_str()));
}

void test_JsonController() {
    cout << "TEST	: test_JsonController() =====" << endl;

    Machine machine;
    JsonController jc;

    Serial.clear();
    JsonCommand jcmd;
    ASSERTEQUAL(STATUS_JSON_PARSED, jcmd.parse("{\"sys\":\"\"}"));
    threadClock.ticks = 12345;
    jc.process(machine, jcmd);
    jcmd.response().printTo(Serial);
    char sysbuf[500];
    snprintf(sysbuf, sizeof(sysbuf),
             "{\"s\":%d,\"r\":{\"sys\":{\"b\":\"%s\",\"fr\":1000,\"li\":false,\"tc\":12345,\"v\":%.2f}}}",
             STATUS_OK, BUILD, VERSION_MAJOR * 100 + VERSION_MINOR + VERSION_PATCH / 100.0);
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
    stroke.seg[stroke.length++] = Quad<StepDV>(1, 10, -1, -10);
    stroke.seg[stroke.length++] = Quad<StepDV>(1, 10, -1, -10);
    stroke.seg[stroke.length++] = Quad<StepDV>(-1, -10, 1, 10);
    stroke.dEndPos = Quad<StepCoord>(4, 40, -4, -40);
    ASSERTEQUAL(0, stroke.dtTotal);
    ASSERTEQUAL(STATUS_STROKE_PLANMICROS, stroke.start(tStart));
    stroke.planMicros = 17 * TICK_MICROSECONDS - 1; // should round up to 17
    stroke.dEndPos.value[0] += 100;
    ASSERTEQUAL(STATUS_STROKE_END_ERROR, stroke.start(tStart));
    stroke.dEndPos.value[0] -= 100;
    ASSERTEQUAL(17, stroke.dtTotal);

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
    for (Ticks t = tStart; t < tStart + 20; t++) {
        //cout << "stroke	: traverse(" << t << ")" << endl;
        if (STATUS_OK == stroke.traverse(t, stepper)) {
            ASSERTEQUAL(18, t - tStart);
            Quad<StepCoord> dPos = stepper.dPos;
            ASSERTEQUAL(STATUS_OK, stroke.traverse(t, stepper)); 	// should do nothing
            ASSERTEQUAL(STATUS_OK, stroke.traverse(t + 1, stepper)); 	// should do nothing
            ASSERT(dPos == stepper.dPos);
            break;
        } else {
            Quad<StepCoord> dPos = stepper.dPos;
            Status status = stroke.traverse(t, stepper); // should do nothing
            ASSERT(status == STATUS_PROCESSING || status == STATUS_OK);
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
            ASSERTEQUAL(18, t - tStart);
            Quad<StepCoord> dPos = stepper.dPos;
            ASSERTEQUAL(STATUS_OK, stroke.traverse(t, stepper)); 	// should do nothing
            ASSERTEQUAL(STATUS_OK, stroke.traverse(t + 1, stepper)); 	// should do nothing
            ASSERT(dPos == stepper.dPos);
            break;
        } else {
            Quad<StepCoord> dPos = stepper.dPos;
            Status status = stroke.traverse(t, stepper); // should do nothing
            ASSERT(status == STATUS_PROCESSING || status == STATUS_OK);
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
    ASSERTEQUAL(17, stroke.dtTotal);
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
            ASSERTEQUAL(18, t - tStart);
            Quad<StepCoord> dPos = stepper.dPos;
            ASSERTEQUAL(STATUS_OK, stroke.traverse(t, stepper)); 	// should do nothing
            ASSERTEQUAL(STATUS_OK, stroke.traverse(t + 1, stepper)); 	// should do nothing
            ASSERT(dPos == stepper.dPos);
            break;
        } else {
            Quad<StepCoord> dPos = stepper.dPos;
            Status status = stroke.traverse(t, stepper); // should do nothing
            ASSERT(status == STATUS_PROCESSING || status == STATUS_OK);
            ASSERT(dPos == stepper.dPos);
        }
    }

    cout << "TEST	: test_Stroke() OK " << endl;
}

void test_Machine_step() {
    cout << "TEST	: test_Machine_step() =====" << endl;

    Machine machine;
    digitalWrite(machine.axis[0].pinMin, 0);
    digitalWrite(machine.axis[0].pinEnable, 1);
    digitalWrite(machine.axis[1].pinMin, 0);
    digitalWrite(machine.axis[1].pinEnable, 1);
    digitalWrite(machine.axis[2].pinMin, 0);
    digitalWrite(machine.axis[2].pinEnable, 1);
    machine.axis[0].travelMax = 5;
    machine.axis[1].travelMax = 4;
    machine.axis[2].travelMax = 3;
    machine.axis[3].travelMax = 2;
    ASSERTEQUAL(false, machine.axis[0].atMin);
    ASSERTEQUAL(false, machine.axis[1].atMin);
    ASSERTEQUAL(false, machine.axis[2].atMin);
    ASSERTEQUAL(false, machine.axis[3].atMin);
    Status status;
    ASSERTQUAD(machine.motorPosition(), Quad<StepCoord>(0, 0, 0, 0));
    ASSERTEQUAL(STATUS_STEP_RANGE_ERROR, machine.step(Quad<StepCoord>(1, 2, 3, 4)));
    ASSERTQUAD(machine.motorPosition(), Quad<StepCoord>(0, 0, 0, 0));

    // Test travelMax
    ASSERTEQUAL(STATUS_OK, machine.step(Quad<StepCoord>(1, 0, 0, 0)));
    ASSERTEQUAL(false, machine.axis[3].atMin);
    ASSERT(machine.motorPosition() == Quad<StepCoord>(1, 0, 0, 0));
    ASSERTEQUAL(STATUS_OK, machine.step(Quad<StepCoord>(1, 1, 0, 0)));
    ASSERTEQUAL(false, machine.axis[3].atMin);
    ASSERT(machine.motorPosition() == Quad<StepCoord>(2, 1, 0, 0));
    ASSERTEQUAL(STATUS_OK, machine.step(Quad<StepCoord>(1, 1, 1, 0)));
    ASSERTEQUAL(false, machine.axis[3].atMin);
    ASSERT(machine.motorPosition() == Quad<StepCoord>(3, 2, 1, 0));
    ASSERTEQUAL(STATUS_OK, machine.step(Quad<StepCoord>(1, 1, 1, 1)));
    ASSERTEQUAL(false, machine.axis[3].atMin);
    ASSERTQUAD(Quad<StepCoord>(4, 3, 2, 0), machine.motorPosition());
    ASSERTEQUAL(STATUS_OK, machine.step(Quad<StepCoord>(1, 1, 1, 1)));
    ASSERTEQUAL(false, machine.axis[3].atMin);
    ASSERTQUAD(machine.motorPosition(), Quad<StepCoord>(5, 4, 3, 0));
    ASSERTEQUAL(false, machine.axis[0].atMin);
    ASSERTEQUAL(false, machine.axis[1].atMin);
    ASSERTEQUAL(false, machine.axis[2].atMin);
    ASSERTEQUAL(false, machine.axis[3].atMin);
    ASSERTEQUAL(STATUS_OK, machine.step(Quad<StepCoord>(1, 1, 1, 1)));
    ASSERTQUAD(Quad<StepCoord>(5, 4, 3, 0), machine.motorPosition());

    // Test travelMin
    ASSERTEQUAL(false, machine.axis[0].atMin);
    ASSERTEQUAL(false, machine.axis[1].atMin);
    ASSERTEQUAL(false, machine.axis[2].atMin);
    ASSERTEQUAL(false, machine.axis[3].atMin);
    ASSERTEQUAL(STATUS_OK, machine.step(Quad<StepCoord>(-1, -1, -1, -1)));
    ASSERTQUAD(Quad<StepCoord>(4, 3, 2, 0), machine.motorPosition());
    ASSERTEQUAL(STATUS_OK, machine.step(Quad<StepCoord>(-1, -1, -1, -1)));
    ASSERTQUAD(Quad<StepCoord>(3, 2, 1, 0), machine.motorPosition());
    ASSERTEQUAL(STATUS_OK, machine.step(Quad<StepCoord>(-1, -1, -1, -1)));
    ASSERTQUAD(Quad<StepCoord>(2, 1, 0, 0), machine.motorPosition());
    ASSERTEQUAL(STATUS_OK, machine.step(Quad<StepCoord>(-1, -1, -1, -1)));
    ASSERTQUAD(Quad<StepCoord>(1, 0, 0, 0), machine.motorPosition());
    ASSERTEQUAL(STATUS_OK, machine.step(Quad<StepCoord>(-1, -1, -1, -1)));
    ASSERTQUAD(Quad<StepCoord>(0, 0, 0, 0), machine.motorPosition());
    machine.axis[1].travelMin--;
    machine.axis[2].travelMin++;
    ASSERTEQUAL(STATUS_OK, machine.step(Quad<StepCoord>(-1, -1, -1, -1)));
    ASSERTQUAD(Quad<StepCoord>(0, -1, 0, 0), machine.motorPosition());

    // Test atMin
    ASSERTQUAD(Quad<StepCoord>(0, -1, 0, 0), machine.motorPosition());
    digitalWrite(machine.axis[0].pinMin, 1);
    machine.axis[0].travelMin = -10;
    machine.axis[1].travelMin = -10;
    machine.axis[2].travelMin = -10;
    machine.axis[3].travelMin = -10;
    ASSERTEQUAL(STATUS_OK, machine.step(Quad<StepCoord>(-1, -1, -1, -1)));
    ASSERTQUAD(Quad<StepCoord>(0, -2, -1, 0), machine.motorPosition());

    cout << "TEST	: test_Machine_step() OK " << endl;
}

void test_MachineThread() {
    cout << "TEST	: test_MachineThread() =====" << endl;

	MachineThread mt;
	ASSERTQUAD(Quad<StepCoord>(0,0,0,0), mt.machine.motorPosition());

	threadClock.ticks = 100;
	Serial.clear();
	Serial.push("{");
	mt.Heartbeat();
	ASSERTEQUAL(STATUS_SERIAL_EOL_WAIT, mt.status);
	ASSERTEQUALS("", Serial.output().c_str());

	threadClock.ticks++;
    const char *jsonIn = "'systc':'','dvs':{'us':123,'dp':[10,20],'s1':[1,2],'s2':[4,5],'s3':[7,8],'s4':[-10,-11]}}\n";
    string ji(jsonTemplate(jsonIn, "'\""));
	Serial.push(ji.c_str());
	mt.Heartbeat();
	ASSERTEQUAL(STATUS_JSON_PARSED, mt.status);
	ASSERTEQUALS("", Serial.output().c_str());

	threadClock.ticks++;
	mt.Heartbeat();
	ASSERTEQUAL(STATUS_PROCESSING, mt.status);
	ASSERTEQUALS("", Serial.output().c_str());

	threadClock.ticks++;
	mt.Heartbeat();
	ASSERTEQUAL(STATUS_PROCESSING, mt.status);
	ASSERTEQUALS("", Serial.output().c_str());

	threadClock.ticks++;
	ASSERTEQUAL(STATUS_PROCESSING, mt.status);
	mt.Heartbeat();
	ASSERTEQUALS("", Serial.output().c_str());

	threadClock.ticks++;
	ASSERTEQUAL(STATUS_PROCESSING, mt.status);
	mt.Heartbeat();
	ASSERTEQUALS("", Serial.output().c_str());

	threadClock.ticks++;
	ASSERTEQUAL(STATUS_OK, mt.status);
	mt.Heartbeat();
    const char *jsonOut = "{'s':0,'r':{'systc':105,'dvs':{'us':123,'dp':[10,20],'s1':10,'s2':20,'s3':0,'s4':0}}}";
	string jo(jsonTemplate(jsonOut, "'\""));
	ASSERTEQUALS(jo.c_str(), Serial.output().c_str());
	ASSERTQUAD(Quad<StepCoord>(10,20,0,0), mt.machine.motorPosition());

    cout << "TEST	: test_MachineThread() OK " << endl;
}

int main(int argc, char *argv[]) {
    LOGINFO3("INFO	: FireStep test v%d.%d.%d",
             VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
    firelog_level(FIRELOG_TRACE);

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

    cout << "TEST	: END OF TEST main()" << endl;
}

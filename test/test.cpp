#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include "FireLog.h"
#include "FireUtils.h"
#include "version.h"
#include "Arduino.h"

#include "MachineThread.h"
#include "Display.h"
#include "DeltaCalculator.h"
#include "ProgMem.h"

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
	int32_t hash1 = machine.hash();
	ASSERT(hash1);
    machine.setup(PC2_RAMPS_1_4);
	int32_t hash2 = machine.hash();
	ASSERT(hash1 != hash2);
    ASSERTEQUAL(OUTPUT, arduino.getPinMode(PC2_X_STEP_PIN));
    ASSERTEQUAL(LOW, arduino.getPin(PC2_X_STEP_PIN));
    ASSERTEQUAL(OUTPUT, arduino.getPinMode(PC2_X_DIR_PIN));
    ASSERTEQUAL(HIGH, arduino.getPin(PC2_X_DIR_PIN));
    ASSERTEQUAL(OUTPUT, arduino.getPinMode(PC2_X_ENABLE_PIN));
    ASSERTEQUAL(LOW, arduino.getPin(PC2_X_ENABLE_PIN));
    ASSERTEQUAL(INPUT, arduino.getPinMode(PC2_X_MIN_PIN));
    ASSERTEQUAL(OUTPUT, arduino.getPinMode(PC2_Y_STEP_PIN));
    ASSERTEQUAL(LOW, arduino.getPin(PC2_Y_STEP_PIN));
    ASSERTEQUAL(OUTPUT, arduino.getPinMode(PC2_Y_DIR_PIN));
    ASSERTEQUAL(HIGH, arduino.getPin(PC2_Y_DIR_PIN));
    ASSERTEQUAL(OUTPUT, arduino.getPinMode(PC2_Y_ENABLE_PIN));
    ASSERTEQUAL(LOW, arduino.getPin(PC2_Y_ENABLE_PIN));
    ASSERTEQUAL(INPUT, arduino.getPinMode(PC2_Y_MIN_PIN));
    ASSERTEQUAL(OUTPUT, arduino.getPinMode(PC2_Z_STEP_PIN));
    ASSERTEQUAL(LOW, arduino.getPin(PC2_Z_STEP_PIN));
    ASSERTEQUAL(OUTPUT, arduino.getPinMode(PC2_Z_DIR_PIN));
    ASSERTEQUAL(HIGH, arduino.getPin(PC2_Z_DIR_PIN));
    ASSERTEQUAL(OUTPUT, arduino.getPinMode(PC2_Z_ENABLE_PIN));
    ASSERTEQUAL(LOW, arduino.getPin(PC2_Z_ENABLE_PIN));
    ASSERTEQUAL(INPUT, arduino.getPinMode(PC2_Z_MIN_PIN));
    ASSERTEQUAL(1, arduino.pulses(PC2_X_STEP_PIN));
    ASSERTEQUAL(1, arduino.pulses(PC2_Y_STEP_PIN));
    ASSERTEQUAL(1, arduino.pulses(PC2_Z_STEP_PIN));
    ASSERT(machine.isCorePin(PC2_X_STEP_PIN));
    ASSERT(machine.isCorePin(PC2_Y_DIR_PIN));
    ASSERT(machine.isCorePin(PC2_Z_MIN_PIN));
    ASSERT(machine.isCorePin(PC2_Z_ENABLE_PIN));
    ASSERT(!machine.isCorePin(17));

    ASSERT(machine.axis[0].isEnabled());
    ASSERT(machine.axis[1].isEnabled());
    ASSERT(machine.axis[2].isEnabled());
	ASSERTEQUAL(hash2, machine.hash());
    ASSERTEQUAL(STATUS_OK, machine.step(Quad<StepDV>(1, 1, 1, 0)));
	ASSERTEQUAL(hash2, machine.hash());
    ASSERTEQUAL(2, arduino.pulses(PC2_X_STEP_PIN));
    ASSERTEQUAL(2, arduino.pulses(PC2_Y_STEP_PIN));
    ASSERTEQUAL(2, arduino.pulses(PC2_Z_STEP_PIN));

	ASSERTEQUAL(hash2, machine.hash());
    ASSERTEQUAL(STATUS_OK, machine.step(Quad<StepDV>(-1, -1, -1, 0)));
	ASSERTEQUAL(hash2, machine.hash());
    ASSERTEQUAL(3, arduino.pulses(PC2_X_STEP_PIN));
    ASSERTEQUAL(3, arduino.pulses(PC2_Y_STEP_PIN));
    ASSERTEQUAL(3, arduino.pulses(PC2_Z_STEP_PIN));

    MachineThread machThread;
	ASSERTEQUAL(hash2, machine.hash());
    machThread.setup(PC2_RAMPS_1_4);
	ASSERTEQUAL(hash2, machine.hash());
    threadRunner.setup();
	ASSERTEQUAL(hash2, machine.hash());
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

	ASSERTEQUAL(hash2, machine.hash());
	machine.axis[5].pinStep = 3;
	ASSERTEQUAL(hash2, machine.hash());

	// Configuration save
	char buf[255];
	char *out = machine.axis[0].saveConfig(buf, sizeof(buf));
	ASSERTEQUALS(JT("{'dh':1,'en':1,'ho':0,'is':0,'lb':200,'mi':16,'sa':1.8,'tm':32000,'tn':-32000,'ud':0}"), buf);
	ASSERTEQUAL((size_t)(void*)out, (size_t)(void*)buf+strlen(buf));
	out = machine.saveSysConfig(buf, sizeof(buf));
#define HASH1 "1130154469"
	ASSERTEQUALS(JT("{'ch':" HASH1 ",'pc':2,'to':0,'ah':0,'db':0,'hp':3,'jp':0,'lh':0,"
				 "'mv':12800,'om':0,'pb':2,'pi':11,'tv':0.70}"), 
				 buf);
	ASSERTEQUAL((size_t)(void*)out, (size_t)(void*)buf+strlen(buf));
	machine.bed.a = 0.00015;
	machine.bed.b = -0.00025;
	out = machine.saveDimConfig(buf, sizeof(buf));
	ASSERTEQUALS(JT("{'bx':0.0002,'by':-0.0003,'bz':0.00,"
				 "'e':131.64,'f':190.53,'gr1':"FPD_GEAR_RATIO_S",'gr2':"FPD_GEAR_RATIO_S",'gr3':"FPD_GEAR_RATIO_S","
				 "'ha':-67.20,'mi':16,'re':270.00,'rf':90.00,'spa':"FPD_SPE_ANGLE_S",'spr':0.00000,'st':200}"),
				 buf);
	ASSERTEQUAL((size_t)(void*)out, (size_t)(void*)buf+strlen(buf));

    // ticks should increase with TCNT1
    Ticks lastClock = ticks();
    test_ticks(1);
    ASSERT(lastClock < ticks());
    lastClock = ticks();
    arduino.dump();

	{
		Machine mach;
		mach.topology = MTO_FPD;
		mach.delta.setGearRatio(9.5);
		ASSERTEQUALT(-66.316, mach.setHomePulses(-5600), 0.001);
		ASSERTEQUAL(mach.getHomePulses(), mach.axis[0].home);
		mach.delta.setGearRatio(9.375);
		ASSERTEQUALT(-67.2, mach.setHomePulses(-5600), 0.001);
		ASSERTEQUAL(mach.getHomePulses(), mach.axis[0].home);
	}

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
    ASSERTEQUAL(STATUS_BUSY_PARSED, cmd1.parse("{\"sys\":\"\"}", STATUS_WAIT_IDLE));
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
    ASSERTEQUAL(STATUS_BUSY_PARSED, cmd2.parse("{\"x\":123,\"y\":2.3}", STATUS_WAIT_IDLE));
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
    ASSERTEQUAL(STATUS_WAIT_EOL, cmd3.parse(NULL, STATUS_WAIT_IDLE));
    Serial.push(json2);
    ASSERTEQUAL(STATUS_BUSY_PARSED, cmd3.parse(NULL, STATUS_WAIT_EOL));
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
    ASSERTEQUAL(STATUS_BUSY_PARSED, cmd4.parse(NULL, STATUS_WAIT_IDLE));
    ASSERTEQUAL(STATUS_BUSY_PARSED, cmd4.getStatus());
    ASSERT(cmd4.requestRoot().success());
    ASSERT(cmd4.requestRoot().is<JsonArray&>());
    ASSERT(!cmd4.requestRoot().is<JsonObject&>());
    ASSERTEQUAL(2, cmd4.requestRoot().size());

    cout << "TEST	: test_JsonCommand() OK " << endl;
}

void testJSON_process(MachineThread& mt, JsonCommand &jcmd, string replace,
                      const char *jsonOut, Status status = STATUS_OK) {
    Serial.clear();
    string jo(jsonOut);
    for (int i = 0; i < replace.size(); i += 2) {
        char cmatch = replace[i];
        char creplace = replace[i + 1];
        replaceChar(jo, cmatch, creplace);
    }
    ticks();
    Status actualStatus = mt.process(jcmd);
    ASSERTEQUAL(status, actualStatus);
    ASSERT(jcmd.requestAvailable() > sizeof(JsonVariant));
    ASSERT(jcmd.responseAvailable() > sizeof(JsonVariant));
    ASSERTEQUALS(jo.c_str(), Serial.output().c_str());
}

JsonCommand testJSON(MachineThread &mt, string replace, const char *jsonIn,
                     const char* jsonOut, Status processStatus = STATUS_OK) {
    string ji(jsonTemplate(jsonIn, replace));
    JsonCommand jcmd;
    Status status = jcmd.parse(ji.c_str(), STATUS_WAIT_IDLE);
    ASSERTEQUAL(STATUS_BUSY_PARSED, status);
    char parseOut[MAX_JSON];
    jcmd.requestRoot().printTo(parseOut, sizeof(parseOut));
    ASSERTEQUALS(ji.c_str(), parseOut);

    testJSON_process(mt, jcmd, replace, jsonOut, processStatus);

    return jcmd;
}

void test_JsonController_motor(MachineThread &mt, char motor) {
    string replace;
    replace.push_back('\'');
    replace.push_back('"');
    replace.push_back('?');
    replace.push_back(motor);
    replace.push_back('!');
    replace.push_back(motor - 1);
    testJSON(mt, replace, "{'?':''}", "{'s':0,'r':{'?':{'ma':!}},'t':0.000}\n");
    testJSON(mt, replace, "{'?ma':''}", "{'s':0,'r':{'?ma':!},'t':0.000}\n");
    testJSON(mt, replace, "{'?ma':4}", "{'s':0,'r':{'?ma':4},'t':0.000}\n");
    testJSON(mt, replace, "{'?ma':''}", "{'s':0,'r':{'?ma':4},'t':0.000}\n");
    testJSON(mt, replace, "{'?':{'ma':''}}", "{'s':0,'r':{'?':{'ma':4}},'t':0.000}\n");
    testJSON(mt, replace, "{'?':{'ma':!}}", "{'s':0,'r':{'?':{'ma':!}},'t':0.000}\n");
    testJSON(mt, replace, "{'?':{'ma':''}}", "{'s':0,'r':{'?':{'ma':!}},'t':0.000}\n");
}

void test_JsonController_axis(MachineThread &mt, char axis) {
    string replace;
    replace.push_back('\'');
    replace.push_back('"');
    replace.push_back('?');
    replace.push_back(axis);
    testJSON(mt, replace, "{'?tn':''}", "{'s':0,'r':{'?tn':-32000},'t':0.000}\n");
    testJSON(mt, replace, "{'?tn':111}", "{'s':0,'r':{'?tn':111},'t':0.000}\n");
    testJSON(mt, replace, "{'?tn':''}", "{'s':0,'r':{'?tn':111},'t':0.000}\n");
    testJSON(mt, replace, "{'?':{'tn':''}}", "{'s':0,'r':{'?':{'tn':111}},'t':0.000}\n");
    testJSON(mt, replace, "{'?':{'tn':-32000}}", "{'s':0,'r':{'?':{'tn':-32000}},'t':0.000}\n");
    testJSON(mt, replace, "{'?':{'tn':''}}", "{'s':0,'r':{'?':{'tn':-32000}},'t':0.000}\n");
    testJSON(mt, replace, "{'?tm':''}", "{'s':0,'r':{'?tm':32000},'t':0.000}\n");  	// default
    testJSON(mt, replace, "{'?tm':222}", "{'s':0,'r':{'?tm':222},'t':0.000}\n");
    testJSON(mt, replace, "{'?tm':''}", "{'s':0,'r':{'?tm':222},'t':0.000}\n");
    testJSON(mt, replace, "{'?':{'tm':''}}", "{'s':0,'r':{'?':{'tm':222}},'t':0.000}\n");
    testJSON(mt, replace, "{'?':{'tm':32000}}", "{'s':0,'r':{'?':{'tm':32000}},'t':0.000}\n");
    testJSON(mt, replace, "{'?':{'tm':''}}", "{'s':0,'r':{'?':{'tm':32000}},'t':0.000}\n");
    testJSON(mt, replace, "{'?':{'mi':''}}", "{'s':0,'r':{'?':{'mi':16}},'t':0.000}\n");
    testJSON(mt, replace, "{'?':{'mi':1}}", "{'s':0,'r':{'?':{'mi':1}},'t':0.000}\n");
    testJSON(mt, replace, "{'?':{'mi':''}}", "{'s':0,'r':{'?':{'mi':1}},'t':0.000}\n");
    testJSON(mt, replace, "{'?':{'mi':16}}", "{'s':0,'r':{'?':{'mi':16}},'t':0.000}\n");
    testJSON(mt, replace, "{'?':{'dh':''}}", "{'s':0,'r':{'?':{'dh':true}},'t':0.000}\n");
    testJSON(mt, replace, "{'?':{'dh':false}}", "{'s':0,'r':{'?':{'dh':false}},'t':0.000}\n");
    testJSON(mt, replace, "{'?':{'dh':''}}", "{'s':0,'r':{'?':{'dh':false}},'t':0.000}\n");
    testJSON(mt, replace, "{'?':{'dh':true}}", "{'s':0,'r':{'?':{'dh':true}},'t':0.000}\n");
    testJSON(mt, replace, "{'?':{'sa':''}}", "{'s':0,'r':{'?':{'sa':1.800}},'t':0.000}\n");
    testJSON(mt, replace, "{'?':{'sa':0.9}}", "{'s':0,'r':{'?':{'sa':0.900}},'t':0.000}\n");
    testJSON(mt, replace, "{'?':{'sa':''}}", "{'s':0,'r':{'?':{'sa':0.900}},'t':0.000}\n");
    testJSON(mt, replace, "{'?':{'sa':1.8}}", "{'s':0,'r':{'?':{'sa':1.800}},'t':0.000}\n");

    testJSON(mt, replace, "{'x':''}",
             "{'s':0,'r':{'x':{'dh':true,'en':false,'ho':0,'is':0,'lb':200,'lm':false,'ln':false,"\
             "'mi':16,'pd':55,'pe':38,'pm':255,'pn':3,'po':0,'ps':54,"\
             "'sa':1.800,'tm':32000,'tn':-32000,'ud':0}},'t':0.000}\n");
    testJSON(mt, replace, "{'y':''}",
             "{'s':0,'r':{'y':{'dh':true,'en':false,'ho':0,'is':0,'lb':200,'lm':false,'ln':false,"\
             "'mi':16,'pd':61,'pe':56,'pm':255,'pn':14,'po':0,'ps':60,"\
             "'sa':1.800,'tm':32000,'tn':-32000,'ud':0}},'t':0.000}\n");
    testJSON(mt, replace, "{'z':''}",
             "{'s':0,'r':{'z':{'dh':true,'en':false,'ho':0,'is':0,'lb':200,'lm':false,'ln':false,"\
             "'mi':16,'pd':48,'pe':62,'pm':255,'pn':18,'po':0,'ps':46,"\
             "'sa':1.800,'tm':32000,'tn':-32000,'ud':0}},'t':0.000}\n");
}

MachineThread test_setup(bool clearArduino=true) {
    if (clearArduino) {
        arduino.clear();
    }
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
    ASSERTEQUAL(HIGH, arduino.getPin(PC2_X_DIR_PIN));
    ASSERTEQUAL(HIGH, arduino.getPin(PC2_Y_DIR_PIN));
    ASSERTEQUAL(HIGH, arduino.getPin(PC2_Z_DIR_PIN));
    ASSERTEQUAL(LOW, arduino.getPin(PC2_X_ENABLE_PIN)); // enabled
    ASSERTEQUAL(LOW, arduino.getPin(PC2_Y_ENABLE_PIN)); // enabled
    ASSERTEQUAL(LOW, arduino.getPin(PC2_Z_ENABLE_PIN)); // enabled
    if (clearArduino) {
        ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);
		mt.loop();
        ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);
        char buf[100];
        snprintf(buf, sizeof(buf), "FireStep %d.%d.%d sysch:%ld\n", 
			VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, (long) mt.machine.hash());
        ASSERTEQUALS(buf, Serial.output().c_str());
    }
    ASSERTQUAD(Quad<StepCoord>(0, 0, 0, 0), mt.machine.getMotorPosition());
    for (int i=0; i<MOTOR_COUNT; i++) {
        ASSERTEQUAL((size_t) &mt.machine.axis[i], (size_t) &mt.machine.getMotorAxis(i));
    }

    return mt;
}

void test_mpo() {
    arduino.clear();
    threadRunner.clear();
    MachineThread mt;
    mt.machine.pDisplay = &testDisplay;
    mt.setup(PC1_EMC02);

    Machine & machine = mt.machine;
    testDisplay.clear();

    string replace;
    replace.push_back('\'');
    replace.push_back('"');
    testJSON(mt, replace,
             "{'mpo':''}", "{'s':0,'r':{'mpo':{'1':0,'2':0,'3':0,'4':0}},'t':0.000}\n");
    testJSON(mt, replace, "{'mpo1':''}", "{'s':0,'r':{'mpo1':0},'t':0.000}\n");
    testJSON(mt, replace, "{'mpo1':32760}", "{'s':0,'r':{'mpo1':32760},'t':0.000}\n");
    testJSON(mt, replace, "{'mpo1':''}", "{'s':0,'r':{'mpo1':32760},'t':0.000}\n");
    testJSON(mt, replace, "{'mpo1':-32760}", "{'s':0,'r':{'mpo1':-32760},'t':0.000}\n");
    testJSON(mt, replace, "{'mpo1':''}", "{'s':0,'r':{'mpo1':-32760},'t':0.000}\n");
    testJSON(mt, replace, "{'mpo2':''}", "{'s':0,'r':{'mpo2':0},'t':0.000}\n");
    testJSON(mt, replace, "{'mpo2':32761}", "{'s':0,'r':{'mpo2':32761},'t':0.000}\n");
    testJSON(mt, replace, "{'mpo2':''}", "{'s':0,'r':{'mpo2':32761},'t':0.000}\n");
    testJSON(mt, replace, "{'mpo2':-32761}", "{'s':0,'r':{'mpo2':-32761},'t':0.000}\n");
    testJSON(mt, replace, "{'mpo2':''}", "{'s':0,'r':{'mpo2':-32761},'t':0.000}\n");
    testJSON(mt, replace, "{'mpo3':''}", "{'s':0,'r':{'mpo3':0},'t':0.000}\n");
    testJSON(mt, replace, "{'mpo3':32762}", "{'s':0,'r':{'mpo3':32762},'t':0.000}\n");
    testJSON(mt, replace, "{'mpo3':''}", "{'s':0,'r':{'mpo3':32762},'t':0.000}\n");
    testJSON(mt, replace, "{'mpo3':-32762}", "{'s':0,'r':{'mpo3':-32762},'t':0.000}\n");
    testJSON(mt, replace, "{'mpo3':''}", "{'s':0,'r':{'mpo3':-32762},'t':0.000}\n");
    testJSON(mt, replace, "{'mpo4':''}", "{'s':0,'r':{'mpo4':0},'t':0.000}\n");
    testJSON(mt, replace, "{'mpo4':32763}", "{'s':0,'r':{'mpo4':32763},'t':0.000}\n");
    testJSON(mt, replace, "{'mpo4':''}", "{'s':0,'r':{'mpo4':32763},'t':0.000}\n");
    testJSON(mt, replace, "{'mpo4':-32763}", "{'s':0,'r':{'mpo4':-32763},'t':0.000}\n");
    testJSON(mt, replace, "{'mpo4':''}", "{'s':0,'r':{'mpo4':-32763},'t':0.000}\n");
    testJSON(mt, replace, "{'mpo':''}",
             "{'s':0,'r':{'mpo':{'1':-32760,'2':-32761,'3':-32762,'4':-32763}},'t':0.000}\n");
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
    ASSERTEQUAL(HIGH, arduino.getPin(PC2_X_DIR_PIN));
    ASSERTEQUAL(HIGH, arduino.getPin(PC2_Y_DIR_PIN));
    //ASSERTEQUAL(usDelay, arduino.get_usDelay());
    xpulses = arduino.pulses(PC2_X_STEP_PIN);
    ypulses = arduino.pulses(PC2_Y_STEP_PIN);
    zpulses = arduino.pulses(PC2_Z_STEP_PIN);
    xdirpulses = arduino.pulses(PC2_X_DIR_PIN);
    ydirpulses = arduino.pulses(PC2_Y_DIR_PIN);
    zdirpulses = arduino.pulses(PC2_Z_DIR_PIN);

    mt.loop();	// pController->process
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
    ASSERTEQUALS(JT("{'s':-901,'r':{'tstrv':[1,2]},'t':0.500}\n"), Serial.output().c_str());

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

    mt.loop();	// pController->process
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUAL(DISPLAY_WAIT_IDLE, mt.machine.pDisplay->getStatus());
    ASSERTEQUAL(xpulses + 1, arduino.pulses(PC2_X_STEP_PIN));
    ASSERTEQUAL(ypulses + 100, arduino.pulses(PC2_Y_STEP_PIN));
    ASSERTEQUAL(zpulses + 1000, arduino.pulses(PC2_Z_STEP_PIN));
    ASSERTEQUALS(JT("{'s':0,'r':{'tstsp':[1,100,1000]},'t':0.000}\n"), Serial.output().c_str());

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

    mt.loop();	// pController->process
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    ASSERTEQUAL(DISPLAY_BUSY_MOVING, mt.machine.pDisplay->getStatus());
    ASSERTEQUAL(xpulses + 2 * 3200, arduino.pulses(PC2_X_STEP_PIN));
    ASSERTEQUAL(ypulses + 2 * 6400, arduino.pulses(PC2_Y_STEP_PIN));
    ASSERTEQUAL(zpulses, arduino.pulses(PC2_Z_STEP_PIN));

    mt.loop();	// pController->process
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

	MachineThread mt;
    Machine & machine(mt.machine);
    machine.setPinConfig(PC2_RAMPS_1_4);
    JsonController &jc(*mt.pController);
    arduino.setPin(PC2_X_MIN_PIN, false);
    arduino.setPin(PC2_Y_MIN_PIN, false);
    arduino.setPin(PC2_Z_MIN_PIN, false);

    Serial.clear();
    machine.pDisplay->setStatus(DISPLAY_WAIT_IDLE);
    JsonCommand jcmd;
    ASSERTEQUAL(STATUS_BUSY_PARSED, jcmd.parse("{\"sys\":\"\"}", STATUS_WAIT_IDLE));
    threadClock.ticks = 12345;
    mt.process(jcmd);
    char sysbuf[500];
    const char *fmt = "{'s':%d,'r':{'sys':"\
                      "{'ah':false,'as':false,'ch':1130154407,'eu':false,'fr':1000,'hp':3,'jp':false,'lh':false,"\
                      "'lp':0,'mv':12800,'om':0,'pb':2,'pc':2,'pi':11,'sd':800,'tc':12345,"\
                      "'to':0,'tv':0.700,'v':%.3f}"\
                      "},'t':0.000}\n";
    snprintf(sysbuf, sizeof(sysbuf), JT(fmt),
             STATUS_OK, VERSION_MAJOR * 100 + VERSION_MINOR + VERSION_PATCH / 100.0);
    ASSERTEQUALS(sysbuf, Serial.output().c_str());

    test_JsonController_axis(mt, 'x');
    test_JsonController_axis(mt, 'y');
    test_JsonController_axis(mt, 'z');
    test_JsonController_axis(mt, 'a');
    test_JsonController_axis(mt, 'b');
    test_JsonController_axis(mt, 'c');

    test_JsonController_motor(mt, '1');
    test_JsonController_motor(mt, '2');
    test_JsonController_motor(mt, '3');
    test_JsonController_motor(mt, '4');

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
    machine.setup(PC2_RAMPS_1_4);
    for (int i = 0; i < 4; i++) {
        arduino.setPin(machine.axis[i].pinMin, 0);
    }
    ASSERTEQUAL(OUTPUT, arduino.getPinMode(machine.axis[0].pinEnable));
    ASSERTEQUAL(LOW, arduino.getPin(machine.axis[0].pinEnable));
    ASSERTEQUAL(OUTPUT, arduino.getPinMode(machine.axis[1].pinEnable));
    ASSERTEQUAL(LOW, arduino.getPin(machine.axis[1].pinEnable));
    ASSERTEQUAL(OUTPUT, arduino.getPinMode(machine.axis[2].pinEnable));
    ASSERTEQUAL(LOW, arduino.getPin(machine.axis[2].pinEnable));
    ASSERTEQUAL(true, machine.axis[0].isEnabled());
    ASSERTEQUAL(true, machine.axis[1].isEnabled());
    ASSERTEQUAL(true, machine.axis[2].isEnabled());
    ASSERTEQUAL(true, machine.axis[3].isEnabled());
    ASSERTEQUAL(true, machine.axis[4].isEnabled());
    ASSERTEQUAL(false, machine.axis[5].isEnabled());

    Status status;
    machine.axis[3].enable(false);
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
    ASSERTEQUAL(100, arduino.pulses(PC2_Z_STEP_PIN)-zpulses);
    ASSERTEQUAL(10, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
    ASSERTEQUAL(1, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    ASSERTQUAD(Quad<StepCoord>(1, 10, 100, 0), machine.getMotorPosition());
    ASSERTEQUALS(JT("{'s':0,'r':"\
                    "{'mov':{'1':1.000,'2':10.000,'3':100.000}},"\
                    "'t':0.148}\n"),
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
                    "{'mov':{'1':1.000,'2':10.000,'3':100.000}},"\
                    "'t':0.000}\n"),
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
                    "{'mov':{'lp':0,'mv':12800,'pp':0.0,'sg':0,'tp':0.000,'ts':0.000,"\
                    "'1':1.000,'2':10.000,'3':100.000,'4':0.000}},"\
                    "'t':0.000}\n"),
                 Serial.output().c_str());
    mt.loop();
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

    // mov x
    Serial.push(JT("{'mov':{'x':50,'z':300}}\n"));
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop();
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUAL(50, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    ASSERTEQUAL(10, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
    ASSERTEQUAL(300, arduino.pulses(PC2_Z_STEP_PIN)-zpulses);
    ASSERTQUAD(Quad<StepCoord>(50, 10, 300, 0), machine.getMotorPosition());
    ASSERTEQUALS(JT("{'s':0,'r':"\
                    "{'mov':{'x':50.000,'z':300.000}},"\
                    "'t':0.209}\n"),
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
                    "{'mov':{'x':10000.000,'y':5000.000,'z':9000.000,'mv':16000}},"\
                    "'t':1.323}\n"),
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
    ASSERTEQUALS(JT("{'s':0,'r':{'movx':10000.000,'movy':5000.000},'t':2.258}\n"),
                 Serial.output().c_str());
    mt.loop();
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

    cout << "TEST	: test_Move() OK " << endl;
}

void test_sys() {
    cout << "TEST	: test_sys() =====" << endl;

    MachineThread mt = test_setup();
    Machine &machine = mt.machine;

    ASSERTEQUAL(800, machine.searchDelay);
    ASSERTEQUAL(MTO_RAW, machine.topology);
    Serial.push(JT("{'systo':1,'syssd':400}\n"));
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop();
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUAL(400, machine.searchDelay);
	ASSERTEQUAL(11, machine.pinStatus);
    ASSERTEQUAL(MTO_FPD, machine.topology);
    ASSERTEQUALS(JT("{'s':0,'r':{'systo':1,'syssd':400},'t':0.000}\n"),
                 Serial.output().c_str());
    mt.loop();
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

	// syspi: custom pin status, probe
    Serial.push(JT("{'syspi':57,'syspb':3}\n"));
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop();
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUAL(400, machine.searchDelay);
	ASSERTEQUAL(57, machine.pinStatus);
    ASSERTEQUAL(MTO_FPD, machine.topology);
    ASSERTEQUALS(JT("{'s':0,'r':{'syspi':57,'syspb':3},'t':0.000}\n"),
                 Serial.output().c_str());
    mt.loop();
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);
    Serial.push(JT("{'sys':''}\n"));
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop();
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUAL(400, machine.searchDelay);
	ASSERTEQUAL(57, machine.pinStatus);
	ASSERTEQUAL(3, machine.op.probe.pinProbe);
    ASSERTEQUAL(MTO_FPD, machine.topology);
    ASSERTEQUALS(JT("{'s':0,'r':"
					"{'sys':{'ah':false,'as':false,'ch':-2836355,'eu':false,'fr':1000,"
					"'hp':3,'jp':false,'lh':false,'lp':0,'mv':12800,'om':0,"
					"'pb':3,'pc':2,'pi':57,'sd':400,'tc':5,'to':1,'tv':0.700,'v':2.040}"
					"},'t':0.000}\n"),
                 Serial.output().c_str());
    mt.loop();
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

    cout << "TEST	: test_sys() OK " << endl;
}

MachineThread test_MTO_FPD_setup() {
    cout << "TEST	: test_MTO_FPD_setup() =====" << endl;

    MachineThread mt = test_setup();
    Machine &machine = mt.machine;
    int32_t xpulses = arduino.pulses(PC2_X_STEP_PIN);
    int32_t ypulses = arduino.pulses(PC2_Y_STEP_PIN);
    int32_t zpulses = arduino.pulses(PC2_Z_STEP_PIN);

	// switch topologies at limit switch
    machine.setMotorPosition(Quad<StepCoord>()); 

    Serial.push(JT("{'systo':1}\n"));
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop();
    ASSERTEQUAL(STATUS_OK, mt.status);

	// switching topologies does not move anything
    ASSERTEQUAL(0, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    ASSERTEQUAL(0, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
    ASSERTEQUAL(0, arduino.pulses(PC2_Z_STEP_PIN)-zpulses);

	// but our first 3 axis coordinates have changed
    ASSERTQUAD(Quad<StepCoord>(-5659,-5659,-5659,0), machine.getMotorPosition());

	// since our home position has changed
    ASSERTEQUAL(-5659, machine.axis[0].home);
    ASSERTEQUAL(-5659, machine.axis[1].home);
    ASSERTEQUAL(-5659, machine.axis[2].home);

	// DeltaCalculator should be in sync with FPD axes
	StepCoord pulses = machine.delta.getHomePulses();
	ASSERTEQUAL(machine.axis[0].home, pulses);
	ASSERTEQUAL(machine.axis[1].home, pulses);
	ASSERTEQUAL(machine.axis[2].home, pulses);

	// Verify Cartesian coordinates
    XYZ3D xyz = mt.fpdController.getXYZ3D();
    ASSERTEQUALT(0, xyz.x, 0.01);
    ASSERTEQUALT(0, xyz.y, 0.01);
    ASSERTEQUALT(65.8913, xyz.z, 0.01);

	// Verify that the FPDController is active
	ASSERTEQUALS("MTO_FPD", mt.pController->name());
    ASSERTEQUALS(JT("{'s':0,'r':{'systo':1},'t':0.000}\n"),
                 Serial.output().c_str());

	// Prepare for next command	
    mt.loop();
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

	// Set coordinate to Cartesian origin and verify
    machine.setMotorPosition(Quad<StepCoord>());
    ASSERTQUAD(Quad<StepCoord>(0,0,0,0), machine.getMotorPosition());
    xyz = mt.fpdController.getXYZ3D();
    ASSERTEQUALT(0, xyz.x, 0.01);
    ASSERTEQUALT(0, xyz.y, 0.01);
    ASSERTEQUALT(0, xyz.z, 0.01);

    cout << "TEST	: test_MTO_FPD_setup() OK " << endl;

	return mt;
}

void test_loadDeltaCalculator(Machine & machine) {
	StepCoord pulses = machine.delta.getHomePulses();
	ASSERTEQUAL(machine.axis[0].home, pulses);
	ASSERTEQUAL(machine.axis[1].home, pulses);
	ASSERTEQUAL(machine.axis[2].home, pulses);
}

void test_MTO_FPD_mov() {
    cout << "TEST	: test_MTO_FPD_mov() =====" << endl;

    MachineThread mt = test_MTO_FPD_setup();
    Machine &machine = mt.machine;
    int32_t xpulses;
    int32_t ypulses;
    int32_t zpulses;
	int32_t e0pulses;

    // movx short form
    xpulses = arduino.pulses(PC2_X_STEP_PIN);
    ypulses = arduino.pulses(PC2_Y_STEP_PIN);
    zpulses = arduino.pulses(PC2_Z_STEP_PIN);
    machine.setMotorPosition(Quad<StepCoord>());
    Serial.push(JT("{'movx':1}\n"));
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop();
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUAL(0, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    ASSERTEQUAL(20, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
    ASSERTEQUAL(20, arduino.pulses(PC2_Z_STEP_PIN)-zpulses);
    ASSERTQUAD(Quad<StepCoord>(0,-20,20,0), machine.getMotorPosition());
    ASSERTEQUALS(JT("{'s':0,'r':{'movx':1.000},'t':0.066}\n"),
                 Serial.output().c_str());
	test_loadDeltaCalculator( machine);
    mt.loop();
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

    // movy short form
    xpulses = arduino.pulses(PC2_X_STEP_PIN);
    ypulses = arduino.pulses(PC2_Y_STEP_PIN);
    zpulses = arduino.pulses(PC2_Z_STEP_PIN);
    machine.setMotorPosition(Quad<StepCoord>());
    Serial.push(JT("{'movy':1}\n"));
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop();
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUAL(23, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    ASSERTEQUAL(11, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
    ASSERTEQUAL(11, arduino.pulses(PC2_Z_STEP_PIN)-zpulses);
    ASSERTQUAD(Quad<StepCoord>(23,-11,-11,0), machine.getMotorPosition());
    ASSERTEQUALS(JT("{'s':0,'r':{'movy':1.000},'t':0.071}\n"),
                 Serial.output().c_str());
	test_loadDeltaCalculator( machine);
    mt.loop();
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

    // movz short form
    xpulses = arduino.pulses(PC2_X_STEP_PIN);
    ypulses = arduino.pulses(PC2_Y_STEP_PIN);
    zpulses = arduino.pulses(PC2_Z_STEP_PIN);
    machine.setMotorPosition(Quad<StepCoord>());
    Serial.push(JT("{'movz':1}\n"));
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop();
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUAL(54, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    ASSERTEQUAL(54, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
    ASSERTEQUAL(54, arduino.pulses(PC2_Z_STEP_PIN)-zpulses);
    ASSERTQUAD(Quad<StepCoord>(-54,-54,-54,0), machine.getMotorPosition());
    ASSERTEQUALS(JT("{'s':0,'r':{'movz':1.000},'t':0.109}\n"),
                 Serial.output().c_str());
	test_loadDeltaCalculator( machine);
    mt.loop();
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

    // mov long form
    xpulses = arduino.pulses(PC2_X_STEP_PIN);
    ypulses = arduino.pulses(PC2_Y_STEP_PIN);
    zpulses = arduino.pulses(PC2_Z_STEP_PIN);
    Serial.push(JT("{'mov':{'x':1,'y':1,'z':1}}\n"));
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop();
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUAL(24, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    ASSERTEQUAL(31, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
    ASSERTEQUAL(9, arduino.pulses(PC2_Z_STEP_PIN)-zpulses);
    ASSERTQUAD(Quad<StepCoord>(-30,-85,-45,0), machine.getMotorPosition());
    ASSERTEQUALS(JT("{'s':0,'r':{'mov':{'x':1.000,'y':1.000,'z':1.000}},'t':0.082}\n"),
                 Serial.output().c_str());
	test_loadDeltaCalculator( machine);
    mt.loop();
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

    // mpo long form read
    machine.setMotorPosition(Quad<StepCoord>(0,-20,20,4));
    Serial.push(JT("{'mpo':''}\n"));
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop();
    XYZ3D xyz = mt.fpdController.getXYZ3D();
    ASSERTEQUALT(0.9980, xyz.x, 0.0001);
    ASSERTEQUALT(0.0000, xyz.y, 0.0001);
    ASSERTEQUALT(0.0020, xyz.z, 0.0001);
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUALS(JT("{'s':0,'r':{'mpo':{'1':0,'2':-20,'3':20,'4':4,'x':0.998,'y':-0.000,'z':0.002}},'t':0.000}\n"),
                 Serial.output().c_str());
	test_loadDeltaCalculator( machine);
    mt.loop();
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

    // movzr 
    xpulses = arduino.pulses(PC2_X_STEP_PIN);
    ypulses = arduino.pulses(PC2_Y_STEP_PIN);
    zpulses = arduino.pulses(PC2_Z_STEP_PIN);
    machine.setMotorPosition(Quad<StepCoord>());
    Serial.push(JT("[{'msg':'hello'},{'movzr':10}]\n"));
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    ASSERTEQUALS(JT("hello\n"),
                 Serial.output().c_str());
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop();
    //ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
	//arduino.timer1(MS_TICKS(1000));
	//mt.loop();
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUALS(JT("{'s':0,'r':{'movzr':10.000},'t':0.347}\n"),
                 Serial.output().c_str());
    ASSERTEQUAL(551, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    ASSERTEQUAL(551, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
    ASSERTEQUAL(551, arduino.pulses(PC2_Z_STEP_PIN)-zpulses);
    ASSERTQUAD(Quad<StepCoord>(-551,-551,-551,0), machine.getMotorPosition());
	xyz = mt.fpdController.getXYZ3D();
	ASSERTEQUALT(0, xyz.x, 0.01);
    ASSERTEQUALT(0, xyz.y, 0.01);
    ASSERTEQUALT(10, xyz.z, 0.01);
	test_loadDeltaCalculator( machine);
    mt.loop();
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

    // mov:{a,r}
    arduino.setPin(PC2_X_MIN_PIN, LOW);
    arduino.setPin(PC2_Y_MIN_PIN, LOW);
    arduino.setPin(PC2_Z_MIN_PIN, LOW);
    machine.setMotorPosition(Quad<StepCoord>());
    Serial.push(JT("{'mov':{'angle':30,'d':10,'zr':-1}}\n"));
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
	arduino.timer1(MS_TICKS(1000));
	mt.loop();
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUALS(JT("{'s':0,'r':{'mov':{'angle':30,'d':10,'zr':-1.000}},'t':1.198}\n"),
                 Serial.output().c_str());
    ASSERTQUAD(Quad<StepCoord>(179,-166,179), machine.getMotorPosition());
	xyz = mt.fpdController.getXYZ3D();
	ASSERTEQUALT(8.65175, xyz.x, 0.01);
    ASSERTEQUALT(5, xyz.y, 0.01);
    ASSERTEQUALT(-1, xyz.z, 0.01);
	test_loadDeltaCalculator( machine);
    mt.loop();
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

    // movzb
    arduino.setPin(PC2_X_MIN_PIN, LOW);
    arduino.setPin(PC2_Y_MIN_PIN, LOW);
    arduino.setPin(PC2_Z_MIN_PIN, LOW);
    machine.setMotorPosition(Quad<StepCoord>());
	machine.bed.a = 0.01;
	machine.bed.b = -0.02;
	machine.bed.c = 0;
	machine.loadDeltaCalculator();
	PH5TYPE zbOrigin = -1.9;
	ASSERTEQUALT(zbOrigin, machine.bed.calcZ(10,100),0.001);
    Serial.push(JT("{'mov':{'x':10,'y':100,'zb':-1},'mpox':'','mpoy':'','mpoz':''}\n"));
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
	arduino.timer1(MS_TICKS(1000));
	mt.loop();
    ASSERTEQUAL(STATUS_OK, mt.status);
	xyz = mt.fpdController.getXYZ3D();
	ASSERTEQUALT(10.0002, xyz.x, 0.001);
    ASSERTEQUALT(99.9999, xyz.y, 0.001);
    ASSERTEQUALT(-1.00852, xyz.z-zbOrigin, 0.01);
    ASSERTEQUALS(JT("{'s':0,'r':{'mov':{'x':10.000,'y':100.000,'zb':-1.000},"
					"'mpox':10.000,'mpoy':100.000,'mpoz':-2.909},'t':1.846}\n"),
                 Serial.output().c_str());
    ASSERTQUAD(Quad<StepCoord>(3269,-107,290,0), machine.getMotorPosition());
	test_loadDeltaCalculator( machine);
    mt.loop();
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

    // movz out of range 
    xpulses = arduino.pulses(PC2_X_STEP_PIN);
    ypulses = arduino.pulses(PC2_Y_STEP_PIN);
    zpulses = arduino.pulses(PC2_Z_STEP_PIN);
    machine.setMotorPosition(Quad<StepCoord>());
    Serial.push(JT("{'movz':-200}\n"));
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop();
    ASSERTEQUAL(STATUS_KINEMATIC_XYZ, mt.status);
    ASSERTEQUAL(0, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    ASSERTEQUAL(0, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
    ASSERTEQUAL(0, arduino.pulses(PC2_Z_STEP_PIN)-zpulses);
    ASSERTQUAD(Quad<StepCoord>(0,0,0,0), machine.getMotorPosition());
    ASSERTEQUALS(JT("{'s':-140,'r':{'movz':-200.000},'t':0.000}\n"),
                 Serial.output().c_str());
	test_loadDeltaCalculator( machine);
    mt.loop();
    ASSERTEQUAL(STATUS_KINEMATIC_XYZ, mt.status);
	mt.status = STATUS_WAIT_IDLE;
    mt.loop();
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

    // mova1 move axis to angle
    xpulses = arduino.pulses(PC2_X_STEP_PIN);
    ypulses = arduino.pulses(PC2_Y_STEP_PIN);
    zpulses = arduino.pulses(PC2_Z_STEP_PIN);
    machine.setMotorPosition(Quad<StepCoord>());
    Serial.push(JT("{'mova1':10}\n"));
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop();
    for (int i=0; mt.status == STATUS_BUSY_CALIBRATING && i<1000; i++) {
        mt.loop();
    }
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUAL(842, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    ASSERTEQUAL(0, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
    ASSERTEQUAL(0, arduino.pulses(PC2_Z_STEP_PIN)-zpulses);
    ASSERTEQUAL(2, machine.op.probe.pinProbe);
    ASSERTQUAD(Quad<StepCoord>(842,0,0,0), machine.getMotorPosition());
    ASSERTEQUALS(JT("{'s':0,'r':{'mova1':10},'t':0.054}\n"),
                 Serial.output().c_str());
    mt.loop();
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

    // mova2 move axis to angle
    xpulses = arduino.pulses(PC2_X_STEP_PIN);
    ypulses = arduino.pulses(PC2_Y_STEP_PIN);
    zpulses = arduino.pulses(PC2_Z_STEP_PIN);
    machine.setMotorPosition(Quad<StepCoord>());
    Serial.push(JT("{'mov':{'a2':-10}}\n"));
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop();
    for (int i=0; mt.status==STATUS_BUSY_CALIBRATING && i<1000; i++) {
        mt.loop();
    }
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUAL(0, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    ASSERTEQUAL(841, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
    ASSERTEQUAL(0, arduino.pulses(PC2_Z_STEP_PIN)-zpulses);
    ASSERTQUAD(Quad<StepCoord>(0,-841,0,0), machine.getMotorPosition());
    ASSERTEQUALS(JT("{'s':0,'r':{'mov':{'a2':-10}},'t':0.054}\n"),
                 Serial.output().c_str());
    mt.loop();
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

    // mova3 move axis to angle
    xpulses = arduino.pulses(PC2_X_STEP_PIN);
    ypulses = arduino.pulses(PC2_Y_STEP_PIN);
    zpulses = arduino.pulses(PC2_Z_STEP_PIN);
    machine.setMotorPosition(Quad<StepCoord>());
    Serial.push(JT("{'mov':{'a3':-10.0}}\n"));
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop();
    for (int i=0; mt.status==STATUS_BUSY_CALIBRATING && i<1000; i++) {
        mt.loop();
    }
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUAL(0, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    ASSERTEQUAL(0, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
    ASSERTEQUAL(841, arduino.pulses(PC2_Z_STEP_PIN)-zpulses);
    ASSERTQUAD(Quad<StepCoord>(0,0,-841,0), machine.getMotorPosition());
    ASSERTEQUALS(JT("{'s':0,'r':{'mov':{'a3':-10.0}},'t':0.054}\n"),
                 Serial.output().c_str());
    mt.loop();
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

    // mova move fourth axis (nozzle)
    xpulses = arduino.pulses(PC2_X_STEP_PIN);
    ypulses = arduino.pulses(PC2_Y_STEP_PIN);
    zpulses = arduino.pulses(PC2_Z_STEP_PIN);
	e0pulses = arduino.pulses(PC2_E0_STEP_PIN);
    machine.setMotorPosition(Quad<StepCoord>());
    Serial.push(JT("{'mova':100}}\n"));
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop();
    for (int i=0; mt.status==STATUS_BUSY_CALIBRATING && i<1000; i++) {
        mt.loop();
    }
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUAL(0, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    ASSERTEQUAL(0, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
    ASSERTEQUAL(0, arduino.pulses(PC2_Z_STEP_PIN)-zpulses);
    ASSERTEQUAL(100, arduino.pulses(PC2_E0_STEP_PIN)-e0pulses);
    ASSERTQUAD(Quad<StepCoord>(0,0,0,100), machine.getMotorPosition());
    ASSERTEQUALS(JT("{'s':0,'r':{'mova':100.000},'t':0.148}\n"),
                 Serial.output().c_str());
    mt.loop();
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

    // mova move fourth axis (nozzle)
    xpulses = arduino.pulses(PC2_X_STEP_PIN);
    ypulses = arduino.pulses(PC2_Y_STEP_PIN);
    zpulses = arduino.pulses(PC2_Z_STEP_PIN);
	e0pulses = arduino.pulses(PC2_E0_STEP_PIN);
    machine.setMotorPosition(Quad<StepCoord>());
    Serial.push(JT("{'mov':{'a':50}}}\n"));
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop();
    for (int i=0; mt.status==STATUS_BUSY_CALIBRATING && i<1000; i++) {
        mt.loop();
    }
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUAL(0, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    ASSERTEQUAL(0, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
    ASSERTEQUAL(0, arduino.pulses(PC2_Z_STEP_PIN)-zpulses);
    ASSERTEQUAL(50, arduino.pulses(PC2_E0_STEP_PIN)-e0pulses);
    ASSERTQUAD(Quad<StepCoord>(0,0,0,50), machine.getMotorPosition());
    ASSERTEQUALS(JT("{'s':0,'r':{'mov':{'a':50.000}},'t':0.105}\n"),
                 Serial.output().c_str());
    mt.loop();
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

    cout << "TEST	: test_MTO_FPD_mov() OK " << endl;
}

void test_gearRatio() {
    cout << "TEST	: test_gearRatio() =====" << endl;

	DeltaCalculator dc;
    dc.useEffectorOrigin();
	XYZ3D xyz10(0,0,-10);
	Step3D pz10 = dc.calcPulses(xyz10);
	ASSERTEQUAL(true, pz10.isValid());
	ASSERTEQUAL(525, pz10.p1);
	ASSERTEQUAL(525, pz10.p2);
	ASSERTEQUAL(525, pz10.p3);
	dc.setGearRatio(dc.getGearRatio(DELTA_AXIS_1)*1.1, DELTA_AXIS_1);
	pz10 = dc.calcPulses(xyz10);
	ASSERTEQUAL(true, pz10.isValid());
	ASSERTEQUALT(10.4213, dc.getGearRatio(DELTA_AXIS_1), 0.0001);
	ASSERTEQUAL(577, pz10.p1);
	ASSERTEQUAL(525, pz10.p2);
	ASSERTEQUAL(525, pz10.p3);
	dc.setGearRatio(dc.getGearRatio(DELTA_AXIS_2)*1.1, DELTA_AXIS_2);
	pz10 = dc.calcPulses(xyz10);
	ASSERTEQUAL(true, pz10.isValid());
	ASSERTEQUAL(577, pz10.p1);
	ASSERTEQUAL(577, pz10.p2);
	ASSERTEQUAL(525, pz10.p3);
	dc.setGearRatio(dc.getGearRatio(DELTA_AXIS_3)*1.1, DELTA_AXIS_3);
	pz10 = dc.calcPulses(xyz10);
	ASSERTEQUAL(true, pz10.isValid());
	ASSERTEQUAL(577, pz10.p1);
	ASSERTEQUAL(577, pz10.p2);
	ASSERTEQUAL(577, pz10.p3);
	dc.setGearRatio(dc.getGearRatio(DELTA_AXIS_1)*0.9091, DELTA_AXIS_1);
	pz10 = dc.calcPulses(xyz10);
	ASSERTEQUAL(true, pz10.isValid());
	ASSERTEQUAL(525, pz10.p1);
	ASSERTEQUAL(577, pz10.p2);
	ASSERTEQUAL(577, pz10.p3);
	dc.setGearRatio(dc.getGearRatio(DELTA_AXIS_2)*0.9091, DELTA_AXIS_2);
	pz10 = dc.calcPulses(xyz10);
	ASSERTEQUAL(true, pz10.isValid());
	ASSERTEQUAL(525, pz10.p1);
	ASSERTEQUAL(525, pz10.p2);
	ASSERTEQUAL(577, pz10.p3);
	dc.setGearRatio(dc.getGearRatio(DELTA_AXIS_3)*0.9091, DELTA_AXIS_3);
	pz10 = dc.calcPulses(xyz10);
	ASSERTEQUAL(true, pz10.isValid());
	ASSERTEQUAL(525, pz10.p1);
	ASSERTEQUAL(525, pz10.p2);
	ASSERTEQUAL(525, pz10.p3);

    MachineThread mt = test_MTO_FPD_setup();
    Machine &machine = mt.machine;
    int32_t xpulses;
    int32_t ypulses;
    int32_t zpulses;

	// change axis 1 and mov
    xpulses = arduino.pulses(PC2_X_STEP_PIN);
    ypulses = arduino.pulses(PC2_Y_STEP_PIN);
    zpulses = arduino.pulses(PC2_Z_STEP_PIN);
    Serial.push(JT("{'dim':{'gr1':10.4745},'movz':-30,'mpo':''}\n"));
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop();
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUAL(1694, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    ASSERTEQUAL(1532, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
    ASSERTEQUAL(1532, arduino.pulses(PC2_Z_STEP_PIN)-zpulses);
    ASSERTQUAD(Quad<StepCoord>(1694, 1532, 1532, 0), mt.machine.getMotorPosition());
    ASSERTEQUALS(JT("{'s':0,'r':{'dim':{'gr1':10.474},'movz':-30.000,"
					"'mpo':{'1':1694,'2':1532,'3':1532,'4':0,'x':0.000,'y':0.006,'z':-29.993}}"
					",'t':0.609}\n"),
                 Serial.output().c_str());
	ASSERTEQUALT(10.4745, machine.delta.getGearRatio(DELTA_AXIS_1), 0.0001);
    mt.loop();
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

	// return to origin
    xpulses = arduino.pulses(PC2_X_STEP_PIN);
    ypulses = arduino.pulses(PC2_Y_STEP_PIN);
    zpulses = arduino.pulses(PC2_Z_STEP_PIN);
    Serial.push(JT("{'dimgr1':'', 'movz':0,'mpo':''}\n"));
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop();
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUAL(1694, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    ASSERTEQUAL(1532, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
    ASSERTEQUAL(1532, arduino.pulses(PC2_Z_STEP_PIN)-zpulses);
    ASSERTQUAD(Quad<StepCoord>(0, 0, 0, 0), mt.machine.getMotorPosition());
    ASSERTEQUALS(JT("{'s':0,'r':{'dimgr1':10.474,'movz':0.000,"
					"'mpo':{'1':0,'2':0,'3':0,'4':0,'x':0.000,'y':-0.000,'z':0.000}}"
					",'t':0.609}\n"),
                 Serial.output().c_str());
	ASSERTEQUALT(10.4745, machine.delta.getGearRatio(DELTA_AXIS_1), 0.0001);
    mt.loop();
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

    cout << "TEST	: test_gearRatio() OK " << endl;
}

void test_MTO_FPD_prb() {
    cout << "TEST	: test_MTO_FPD_prb() =====" << endl;

    MachineThread mt = test_MTO_FPD_setup();
    Machine &machine = mt.machine;
    int32_t xpulses;
    int32_t ypulses;
    int32_t zpulses;

    // prbz
    xpulses = arduino.pulses(PC2_X_STEP_PIN);
    ypulses = arduino.pulses(PC2_Y_STEP_PIN);
    zpulses = arduino.pulses(PC2_Z_STEP_PIN);
    int32_t e0pulses = arduino.pulses(PC2_E0_STEP_PIN);
    machine.setMotorPosition(Quad<StepCoord>(100, 100, 100, 100));
    arduino.setPin(PC2_PROBE_PIN, LOW);
    Serial.push(JT("{'prbz':''}\n"));
    test_ticks(1);	// parse
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    ASSERTQUAD(Quad<StepCoord>(100, 100, 100, 100), mt.machine.getMotorPosition());
    ASSERTEQUAL(0, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    ASSERTEQUAL(0, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
    ASSERTEQUAL(0, arduino.pulses(PC2_Z_STEP_PIN)-zpulses);
    for (int i=0; i<1000; i++) {
        test_ticks(MS_TICKS(6));	// calibrating
        ASSERTEQUAL(STATUS_BUSY_CALIBRATING, mt.status);
        ASSERT(machine.op.probe.probing);
    }
    ASSERTEQUAL(PDS_Z, machine.op.probe.dataSource);
    ASSERTQUAD(Quad<StepCoord>(1096, 1096, 1096, 100), mt.machine.getMotorPosition());
    ASSERTEQUAL(0, arduino.pulses(PC2_E0_STEP_PIN)-e0pulses);
    ASSERTEQUAL(996, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    ASSERTEQUAL(996, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
    ASSERTEQUAL(996, arduino.pulses(PC2_Z_STEP_PIN)-zpulses);
    XYZ3D xyz = mt.fpdController.getXYZ3D();
    ASSERTEQUALT(-21.2533, xyz.z, 0.0001);
    arduino.setPin(PC2_PROBE_PIN, HIGH);
    test_ticks(1);	// tripped
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERT(!machine.op.probe.probing);
    ASSERTEQUAL(0, arduino.pulses(PC2_E0_STEP_PIN)-e0pulses);
    ASSERTEQUAL(996, arduino.pulses(PC2_Z_STEP_PIN)-zpulses);
    ASSERTEQUAL(996, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
    ASSERTEQUAL(996, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    ASSERTQUAD(Quad<StepCoord>(1096, 1096, 1096, 100), mt.machine.getMotorPosition());
    ASSERTEQUALS(JT("{'s':0,'r':{'prbz':-21.253},'t':6.016}\n"),
                 Serial.output().c_str());
    test_ticks(1);	// tripped
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

	// clear out probe data
    for (int i=0; i<PROBE_DATA; i++) {
        machine.op.probe.archiveData(i);
    }

    // prb
    xpulses = arduino.pulses(PC2_X_STEP_PIN);
    ypulses = arduino.pulses(PC2_Y_STEP_PIN);
    zpulses = arduino.pulses(PC2_Z_STEP_PIN);
    e0pulses = arduino.pulses(PC2_E0_STEP_PIN);
    machine.setMotorPosition(Quad<StepCoord>(100, 100, 100, 100));
    arduino.setPin(PC2_PROBE_PIN, LOW);
    Serial.push(JT("{'prb':''}}\n"));
    test_ticks(1);	// parse
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    test_ticks(1);	// initialize
    ASSERTEQUAL(STATUS_BUSY_CALIBRATING, mt.status);
    ASSERTQUAD(Quad<StepCoord>(100, 100, 100, 100), mt.machine.getMotorPosition());
    ASSERTEQUAL(true, machine.op.probe.probing);
    ASSERTEQUAL(0, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    ASSERTEQUAL(0, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
    ASSERTEQUAL(0, arduino.pulses(PC2_Z_STEP_PIN)-zpulses);
    ASSERTQUAD(Quad<StepCoord>(7579, 7579, 7579, 100), mt.machine.op.probe.end);
    for (int i=0; i<1000; i++) {
        test_ticks(MS_TICKS(6));	// calibrating
        ASSERTEQUAL(STATUS_BUSY_CALIBRATING, mt.status);
        ASSERT(machine.op.probe.probing);
    }
    ASSERTQUAD(Quad<StepCoord>(1096, 1096, 1096, 100), mt.machine.getMotorPosition());
    ASSERTEQUAL(0, arduino.pulses(PC2_E0_STEP_PIN)-e0pulses);
    ASSERTEQUAL(996, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    ASSERTEQUAL(996, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
    ASSERTEQUAL(996, arduino.pulses(PC2_Z_STEP_PIN)-zpulses);
    xyz = mt.fpdController.getXYZ3D();
    ASSERTEQUALT(-21.2533, xyz.z, 0.0001);
    arduino.setPin(PC2_PROBE_PIN, HIGH);
    test_ticks(1);	// tripped
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERT(!machine.op.probe.probing);
    ASSERTEQUAL(0, arduino.pulses(PC2_E0_STEP_PIN)-e0pulses);
    ASSERTEQUAL(996, arduino.pulses(PC2_Z_STEP_PIN)-zpulses);
    ASSERTEQUAL(996, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
    ASSERTEQUAL(996, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    ASSERTQUAD(Quad<StepCoord>(1096, 1096, 1096, 100), mt.machine.getMotorPosition());
    ASSERTEQUALS(JT("{'s':0,'r':{'prb':"
                    "{'1':1096,'2':1096,'3':1096,'4':100,'ip':false,"
                    "'pb':2,'sd':800,'x':0.000,'y':-0.000,'z':-21.253}},'t':6.016}\n"),
                 Serial.output().c_str());
    test_ticks(1);	// tripped
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

    // prbd should return probe data
    machine.setMotorPosition(Quad<StepCoord>(1,2,3,4));
    Serial.push(JT("{'prbd':''}\n"));
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop();
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUALS(JT("{'s':0,'r':{'prbd':"
                    "[-21.253,8.000,7.000,6.000,5.000,4.000,3.000,2.000,1.000]},"
                    "'t':0.000}\n"),
                 Serial.output().c_str());
    mt.loop();
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

    cout << "TEST	: test_MTO_FPD_prb() OK " << endl;
}

void test_MTO_FPD_dim() {
    cout << "TEST	: test_MTO_FPD_dim() =====" << endl;

    MachineThread mt = test_MTO_FPD_setup();
    Machine &machine = mt.machine;
	DeltaCalculator& dc = machine.delta;
    int32_t xpulses;
    int32_t ypulses;
    int32_t zpulses;

    // dim get
    machine.setMotorPosition(Quad<StepCoord>(1,2,3,4));
    Serial.push(JT("{'dim':''}\n"));
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop();
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUALS(JT("{'s':0,'r':{'dim':{"
					"'bx':0.0000,'by':0.0000,'bz':0.000,'e':131.636,'f':190.526,'gr':9.474,"
				    //"'ha':-67.199,'hp':-5659,'mi':16,'re':270.000,'rf':90.000,'spa':"FPD_SPE_ANGLE_S",'spr':"FPD_SPE_RATIO_S",'st':200}"
				    "'ha':-67.199,'hp':-5659,'mi':16,'re':270.000,'rf':90.000,'spa':"FPD_SPE_ANGLE_S",'spr':0.000,'st':200}"
                    "},'t':0.000}\n"),
                 Serial.output().c_str());
	ASSERTEQUALT(9.474, dc.getGearRatio(), 0.001);
    mt.loop();
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

    // dim set
    machine.setMotorPosition(Quad<StepCoord>(1,2,3,4));
    Serial.push(JT("{'dim':{'bx':0.0010,'by':0.0020,'bz':0.003,'e':131.636,'f':190.526,'gr':9.371,"
                   "'mi':32,'re':270.000,'rf':90.000,'st':400,"
				   "'ha':-67.3}}\n"));
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop();
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUALS(JT("{'s':0,'r':{'dim':{"
					"'bx':0.0010,'by':0.0020,'bz':0.003,'e':131.636,'f':190.526,'gr':9.371,"
                    "'mi':32,'re':270.000,'rf':90.000,'st':400,"
				    "'ha':-67.300}},"
                    "'t':0.000}\n"),
                 Serial.output().c_str());
	ASSERTEQUALT(0.0010, machine.bed.a, 0.0001);
	ASSERTEQUALT(0.0020, machine.bed.b, 0.0001);
	ASSERTEQUALT(0.003, machine.bed.c, 0.001);
	ASSERTEQUALT(131.636, dc.getEffectorTriangleSide(), 0.001);
	ASSERTEQUALT(190.526, dc.getBaseTriangleSide(), 0.001);
	ASSERTEQUALT(9.371, dc.getGearRatio(), 0.001);
	ASSERTEQUALT(-67.3, dc.getHomeAngle(), 0.001);
	ASSERTEQUALT(-67.3, machine.getHomeAngle(), 0.001);
	ASSERTEQUALT(32, dc.getMicrosteps(), 0.001);
	ASSERTEQUALT(270.000, dc.getEffectorLength(), 0.001);
	ASSERTEQUALT(90.000, dc.getBaseArmLength(), 0.001);
	ASSERTEQUALT(400.000, dc.getSteps360(), 0.001);
	ASSERTEQUALT(360/400.000, machine.axis[0].stepAngle, 0.001);
	ASSERTEQUALT(360/400.000, machine.axis[1].stepAngle, 0.001);
	ASSERTEQUALT(360/400.000, machine.axis[2].stepAngle, 0.001);
    mt.loop();
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

    cout << "TEST	: test_MTO_FPD_dim() OK " << endl;
}

void test_MTO_FPD_hom() {
    cout << "TEST	: test_MTO_FPD() =====" << endl;

    {   // hom
        MachineThread mt = test_MTO_FPD_setup();
        Machine &machine = mt.machine;
        arduino.setPin(PC2_PROBE_PIN, LOW);
        Serial.push(JT("{'hom':''}}\n"));
        mt.loop();	// parse
        ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
        mt.loop(); // initializing
        ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
        arduino.setPin(PC2_X_MIN_PIN, HIGH);
        arduino.setPin(PC2_Y_MIN_PIN, HIGH);
        arduino.setPin(PC2_Z_MIN_PIN, HIGH);
        mt.loop();
    	int32_t xpulses = arduino.pulses(PC2_X_STEP_PIN);
    	int32_t ypulses = arduino.pulses(PC2_Y_STEP_PIN);
    	int32_t zpulses = arduino.pulses(PC2_Z_STEP_PIN);
        ASSERTEQUAL(STATUS_BUSY_CALIBRATING, mt.status);
        mt.loop(); // calibrating
        ASSERTEQUAL(STATUS_OK, mt.status);
    	ASSERTEQUAL(5659, arduino.pulses(PC2_X_STEP_PIN)-xpulses-machine.axis[0].latchBackoff);
    	ASSERTEQUAL(5659, arduino.pulses(PC2_Y_STEP_PIN)-ypulses-machine.axis[1].latchBackoff);
    	ASSERTEQUAL(5659, arduino.pulses(PC2_Z_STEP_PIN)-zpulses-machine.axis[2].latchBackoff);
        ASSERTEQUALS(JT("{'s':0,'r':{'hom':{'1':-5659,'2':-5659,'3':-5659,'4':0}},'t':0.000}\n"),
                     Serial.output().c_str());
        ASSERTQUAD(Quad<StepCoord>(), machine.getMotorPosition());
        XYZ3D xyz = mt.fpdController.getXYZ3D();
        ASSERTEQUALT(0, xyz.x, 0.01);
        ASSERTEQUALT(0, xyz.y, 0.01);
        ASSERTEQUALT(0, xyz.z, 0.01);
        ASSERTEQUALT(-5659, machine.axis[0].home, 0.01);
        ASSERTEQUALT(-5659, machine.axis[1].home, 0.01);
        ASSERTEQUALT(-5659, machine.axis[2].home, 0.01);
        ASSERTEQUALT(0, machine.axis[3].home, 0.01);
        ASSERTEQUALT(0, machine.axis[0].position, 0.01);
        ASSERTEQUALT(0, machine.axis[1].position, 0.01);
        ASSERTEQUALT(0, machine.axis[2].position, 0.01);
        ASSERTEQUALT(0, machine.axis[3].position, 0.01);
        mt.loop();
        ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);
    }

    {   // hom and set coordinates
        MachineThread mt = test_MTO_FPD_setup();
        Machine &machine = mt.machine;
        machine = mt.machine;
        arduino.setPin(PC2_PROBE_PIN, LOW);
        Serial.push(JT("{'hom':{'1':-5601,'2':-5601,'3':-5601,'4':4}}\n"));
        mt.loop();	// parse
        ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
        mt.loop(); // initializing
        ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
        ASSERTEQUALT(-5601, machine.axis[0].home, 0.01);
        ASSERTEQUALT(-5601, machine.axis[1].home, 0.01);
        ASSERTEQUALT(-5601, machine.axis[2].home, 0.01);
        ASSERTEQUALT(4, machine.axis[3].position, 0.01);
        ASSERTEQUALT(-5601, machine.axis[0].position, 0.01);
        ASSERTEQUALT(-5601, machine.axis[1].position, 0.01);
        ASSERTEQUALT(-5601, machine.axis[2].position, 0.01);
        ASSERTEQUALT(4, machine.axis[3].home, 0.01);
		StepCoord hp = machine.delta.getHomePulses();
		ASSERTEQUAL(machine.axis[0].home, hp);
		ASSERTEQUAL(machine.axis[1].home, hp);
		ASSERTEQUAL(machine.axis[2].home, hp);
        arduino.setPin(PC2_X_MIN_PIN, HIGH);
        arduino.setPin(PC2_Y_MIN_PIN, HIGH);
        arduino.setPin(PC2_Z_MIN_PIN, HIGH);
        mt.loop();	// moving: home and backoff
        ASSERTEQUAL(STATUS_BUSY_CALIBRATING, mt.status);
    	int32_t xpulses = arduino.pulses(PC2_X_STEP_PIN);
    	int32_t ypulses = arduino.pulses(PC2_Y_STEP_PIN);
    	int32_t zpulses = arduino.pulses(PC2_Z_STEP_PIN);
        mt.loop(); // calibrating: rapid probe to post-home destination
        ASSERTEQUAL(STATUS_OK, mt.status);
        ASSERTEQUALS(JT("{'s':0,'r':{'hom':{'1':-5601,'2':-5601,'3':-5601,'4':4}},'t':0.000}\n"),
                     Serial.output().c_str());
		hp = machine.delta.getHomePulses();
		ASSERTEQUAL(machine.axis[0].home, hp);
		ASSERTEQUAL(machine.axis[1].home, hp);
		ASSERTEQUAL(machine.axis[2].home, hp);
    	ASSERTEQUAL(5601, arduino.pulses(PC2_X_STEP_PIN)-xpulses-machine.axis[0].latchBackoff);
    	ASSERTEQUAL(5601, arduino.pulses(PC2_Y_STEP_PIN)-ypulses-machine.axis[1].latchBackoff);
    	ASSERTEQUAL(5601, arduino.pulses(PC2_Z_STEP_PIN)-zpulses-machine.axis[2].latchBackoff);
        ASSERTQUAD(Quad<StepCoord>(0,0,0,4), machine.getMotorPosition());
        XYZ3D xyz = mt.fpdController.getXYZ3D();
        ASSERTEQUALT(0, xyz.x, 0.01);
        ASSERTEQUALT(0, xyz.y, 0.01);
        ASSERTEQUALT(0, xyz.z, 0.01);
        ASSERTEQUALT(-5601, machine.axis[0].home, 0.01);
        ASSERTEQUALT(-5601, machine.axis[1].home, 0.01);
        ASSERTEQUALT(-5601, machine.axis[2].home, 0.01);
        ASSERTEQUALT(4, machine.axis[3].home, 0.01);
        ASSERTEQUALT(0, machine.axis[0].position, 0.01);
        ASSERTEQUALT(0, machine.axis[1].position, 0.01);
        ASSERTEQUALT(0, machine.axis[2].position, 0.01);
        ASSERTEQUALT(4, machine.axis[3].position, 0.01);
        mt.loop();
        ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);
    }

    {   // home single axis
        MachineThread mt = test_MTO_FPD_setup();
        Machine &machine = mt.machine;
        machine = mt.machine;
    	int32_t xpulses = arduino.pulses(PC2_X_STEP_PIN);
    	int32_t ypulses = arduino.pulses(PC2_Y_STEP_PIN);
    	int32_t zpulses = arduino.pulses(PC2_Z_STEP_PIN);
		machine.setMotorPosition(Quad<StepCoord>(1,2,3,4));
        arduino.setPin(PC2_PROBE_PIN, LOW);
        Serial.push(JT("{'hom1':''}\n"));
        mt.loop();	// parse
        ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
        mt.loop(); // initializing
        ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
        ASSERTEQUALT(-5659, machine.axis[0].home, 0.01);
        ASSERTEQUALT(-5659, machine.axis[1].home, 0.01);
        ASSERTEQUALT(-5659, machine.axis[2].home, 0.01);
        ASSERTEQUALT(0, machine.axis[3].home, 0.01);
        ASSERTEQUALT(-5659, machine.axis[0].position, 0.01);
        ASSERTEQUALT(2, machine.axis[1].position, 0.01);
        ASSERTEQUALT(3, machine.axis[2].position, 0.01);
        ASSERTEQUALT(4, machine.axis[3].position, 0.01);
		StepCoord hp = machine.delta.getHomePulses();
        arduino.setPin(PC2_X_MIN_PIN, HIGH);
        arduino.setPin(PC2_Y_MIN_PIN, HIGH);
        arduino.setPin(PC2_Z_MIN_PIN, HIGH);
        mt.loop();	// moving: home and backoff
        ASSERTEQUAL(STATUS_BUSY_CALIBRATING, mt.status);
        mt.loop(); // calibrating: rapid probe to post-home destination
        ASSERTEQUAL(STATUS_OK, mt.status);
        ASSERTEQUALS(JT("{'s':0,'r':{'hom1':-5659},'t':0.000}\n"),
                     Serial.output().c_str());
		hp = machine.delta.getHomePulses();
		ASSERTEQUAL(machine.axis[0].home, hp);
		ASSERTEQUAL(machine.axis[1].home, hp);
		ASSERTEQUAL(machine.axis[2].home, hp);
    	ASSERTEQUAL(2*LATCH_BACKOFF, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
		TESTCOUT2("ypulses:", ypulses, " lb:", machine.axis[1].latchBackoff);
    	ASSERTEQUAL(0, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
    	ASSERTEQUAL(0, arduino.pulses(PC2_Z_STEP_PIN)-zpulses);
        ASSERTQUAD(Quad<StepCoord>(-5659,2,3,4), machine.getMotorPosition());
        mt.loop();
        ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);
    }
	
	{   // autoHome
        MachineThread mt = test_MTO_FPD_setup();
        Machine &machine = mt.machine;
        machine = mt.machine;
    	int32_t xpulses = arduino.pulses(PC2_X_STEP_PIN);
    	int32_t ypulses = arduino.pulses(PC2_Y_STEP_PIN);
    	int32_t zpulses = arduino.pulses(PC2_Z_STEP_PIN);
		machine.setMotorPosition(Quad<StepCoord>(1,2,3,4));
        arduino.setPin(PC2_PROBE_PIN, LOW);
        Serial.push(JT("{'sysah':true,'sysas':true}\n"));
        mt.loop();	// parse
        ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
        mt.loop(); // initializing
        ASSERTEQUAL(STATUS_OK, mt.status);
        ASSERTEQUALS(JT("{'s':0,'r':{'sysah':true,'sysas':true},'t':0.000}\n"),
                     Serial.output().c_str());
        mt.loop();
        ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);
		// simulate restart
		mt.status = STATUS_BUSY_SETUP;
		mt.loop();
		ASSERTEQUAL(STATUS_BUSY_EEPROM, mt.status);
		mt.loop(); // parse startup JSON
		Serial.clear(); // banner
		ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
		mt.loop(); // sys
		ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
		mt.loop(); // x
		ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
		mt.loop(); // y
		ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
		mt.loop(); // z
		ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
		mt.loop(); // a
		ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
		mt.loop(); // b
		ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
		mt.loop(); // c
		ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
		mt.loop(); // home
		ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
		mt.loop(); // home
		ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    	ASSERTEQUAL(0, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    	ASSERTEQUAL(0, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
    	ASSERTEQUAL(0, arduino.pulses(PC2_Z_STEP_PIN)-zpulses);
		mt.loop(); // home moves up
		ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    	ASSERTEQUAL(3, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    	ASSERTEQUAL(3, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
    	ASSERTEQUAL(3, arduino.pulses(PC2_Z_STEP_PIN)-zpulses);
		// trip min switch
        arduino.setPin(PC2_X_MIN_PIN, HIGH);
        arduino.setPin(PC2_Y_MIN_PIN, HIGH);
        arduino.setPin(PC2_Z_MIN_PIN, HIGH);
        mt.loop();	// moving: home and backoff
        ASSERTEQUAL(STATUS_BUSY_CALIBRATING, mt.status);
		mt.loop(); // home
		ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
        mt.loop(); // calibrating: rapid probe to post-home destination
        ASSERTEQUAL(STATUS_OK, mt.status);
        ASSERTEQUALS(JT("{'s':0,'r':{'hom':{'1':-5659,'2':-5659,'3':-5659,'4':0}},'t':0.001}\n"),
                     Serial.output().c_str());
    	ASSERTEQUAL(3+5659+2*LATCH_BACKOFF, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    	ASSERTEQUAL(3+5659+2*LATCH_BACKOFF, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
    	ASSERTEQUAL(3+5659+2*LATCH_BACKOFF, arduino.pulses(PC2_Z_STEP_PIN)-zpulses);
        ASSERTQUAD(Quad<StepCoord>(0,0,0,0), machine.getMotorPosition());
        mt.loop();
        ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);
    }

    cout << "TEST	: test_MTO_FPD_hom() OK " << endl;
}

void test_MTO_FPD() {
    cout << "TEST	: test_MTO_FPD() =====" << endl;

    MachineThread mt = test_MTO_FPD_setup();
    Machine &machine = mt.machine;
    int32_t xpulses;
    int32_t ypulses;
    int32_t zpulses;

    // mov/prb
    xpulses = arduino.pulses(PC2_X_STEP_PIN);
    ypulses = arduino.pulses(PC2_Y_STEP_PIN);
    zpulses = arduino.pulses(PC2_Z_STEP_PIN);
    machine.setMotorPosition(Quad<StepCoord>());
    arduino.setPin(PC2_PROBE_PIN, LOW);
    Serial.push(JT("[{'mov':{'x':5,'y':5,'z':-50}},{'prbz':''}]\n"));
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    ASSERTEQUAL(2633, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    ASSERTEQUAL(2407, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
    ASSERTEQUAL(2573, arduino.pulses(PC2_Z_STEP_PIN)-zpulses);
    xpulses = arduino.pulses(PC2_X_STEP_PIN);
    ypulses = arduino.pulses(PC2_Y_STEP_PIN);
    zpulses = arduino.pulses(PC2_Z_STEP_PIN);
    XYZ3D xyz = mt.fpdController.getXYZ3D();
    ASSERTEQUALT(5.00, xyz.x, 0.01);
    ASSERTEQUALT(4.99142, xyz.y, 0.01);
    ASSERTEQUALT(-50, xyz.z, 0.01);
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_CALIBRATING, mt.status);
    ASSERTQUAD(Quad<StepCoord>(2633,2407,2573,0), machine.op.probe.start);
    ASSERTQUAD(Quad<StepCoord>(7312,6982,7223,0), machine.op.probe.end);
    for (int i=0; i<100; i++) {
        mt.loop();
        ASSERTEQUAL(STATUS_BUSY_CALIBRATING, mt.status);
    }
    arduino.setPin(PC2_PROBE_PIN, HIGH);
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop();
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUAL(100, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    ASSERTEQUAL(98, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
    ASSERTEQUAL(99, arduino.pulses(PC2_Z_STEP_PIN)-zpulses);
    ASSERTEQUALS(JT("{'s':0,'r':{'prbz':-51.932},'t':0.766}\n"),
                 Serial.output().c_str());
    xyz = mt.fpdController.getXYZ3D();
    ASSERTEQUALT(5.05, xyz.x, 0.03);
    ASSERTEQUALT(5.09, xyz.y, 0.03);
    ASSERTEQUALT(-51.932, xyz.z, 0.001);
    mt.loop();
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

    cout << "TEST	: test_MTO_FPD() OK " << endl;
}

MachineThread test_setup_FPD() {
    MachineThread mt = test_setup();
    Machine &machine = mt.machine;
    Serial.push(JT("{'systo':1}\n"));
    mt.loop(); // STATUS_BUSY_PARSED
    mt.loop(); // STATUS_OK
    mt.loop(); // STATUS_WAIT_IDLE
    Serial.clear();
	return mt;
}

void test_calibrate() {
    cout << "TEST	: test_calibrate() =====" << endl;

    {   // with sv == 0, nothing should change
        MachineThread mt = test_setup_FPD();
        Machine& machine = mt.machine;

        machine.op.probe.probeData[0] = -62.259;
        machine.op.probe.probeData[1] = -61.701;
        machine.op.probe.probeData[2] = -61.556;
        machine.op.probe.probeData[3] = -61.465;
        machine.op.probe.probeData[4] = -61.495;
        machine.op.probe.probeData[5] = -61.695;
        machine.op.probe.probeData[6] = -61.761;
        machine.op.probe.probeData[7] = -62.259;
        Serial.push(JT("{'cal':{'bx':'','by':'','bz':'','ha':'','sv':0.000}}\n"));
        mt.loop();
        ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
        mt.loop();
        ASSERTEQUAL(STATUS_OK, mt.status);
        ASSERTEQUALS(JT("{'s':0,'r':{"
                        "'cal':{'bx':0.0000,'by':0.0000,'bz':0.000,'ha':-67.199,'sv':0.000}},"
                        "'t':0.000}\n"),
                     Serial.output().c_str());
        ASSERTEQUAL(-5659, machine.axis[0].home);
        ASSERTEQUAL(-5659, machine.axis[1].home);
        ASSERTEQUAL(-5659, machine.axis[2].home);
        mt.loop();
        ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);
    }

    {   // with default sv (0.7) we get coarse adjustment
        MachineThread mt = test_setup_FPD();
        Machine& machine = mt.machine;
        machine.op.probe.probeData[0] = -53.510;
        machine.op.probe.probeData[1] = -53.900;
        machine.op.probe.probeData[2] = -53.654;
        machine.op.probe.probeData[3] = -53.734;
        machine.op.probe.probeData[4] = -54.011;
        machine.op.probe.probeData[5] = -54.208;
        machine.op.probe.probeData[6] = -54.208;
        machine.op.probe.probeData[7] = -53.529;
        Serial.push(JT("{'cal':{'bx':'','by':'','bz':'','ha':'','he':'','sv':''}}\n"));
        mt.loop();
        ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
        mt.loop();
        ASSERTEQUAL(STATUS_OK, mt.status);
        ASSERTEQUALS(JT("{'s':0,'r':{"
                        "'cal':{'bx':0.0037,'by':0.0060,'bz':-53.956,'ha':-58.380,'he':8.819,'sv':1.000}},"
                        "'t':0.000}\n"),
                     Serial.output().c_str());
        ASSERTEQUAL(-4916, machine.axis[0].home);
        ASSERTEQUAL(-4916, machine.axis[1].home);
        ASSERTEQUAL(-4916, machine.axis[2].home);
        mt.loop();
        ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);
    }

    {   // homeAngle: with default sv (1) we get full non-adaptive adjustment
        MachineThread mt = test_setup_FPD();
        Machine& machine = mt.machine;
        machine.op.probe.probeData[0] = -53.510;
        machine.op.probe.probeData[1] = -53.900;
        machine.op.probe.probeData[2] = -53.654;
        machine.op.probe.probeData[3] = -53.734;
        machine.op.probe.probeData[4] = -54.011;
        machine.op.probe.probeData[5] = -54.208;
        machine.op.probe.probeData[6] = -54.208;
        machine.op.probe.probeData[7] = -53.529;
        Serial.push(JT("{'cal':{'bx':'','by':'','bz':'','ha':'','he':'','sv':'','zc':'','zr':''}}\n"));
        mt.loop();
        ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
        mt.loop();
        ASSERTEQUAL(STATUS_OK, mt.status);
        ASSERTEQUALS(JT("{'s':0,'r':{"
                        "'cal':{'bx':0.0037,'by':0.0060,'bz':-53.956,'ha':-58.380,'he':8.819,'sv':1.000,'zc':-53.520,'zr':-53.953}},"
                        "'t':0.000}\n"),
                     Serial.output().c_str());
        ASSERTEQUAL(-4916, machine.axis[0].home);
        ASSERTEQUAL(-4916, machine.axis[1].home);
        ASSERTEQUAL(-4916, machine.axis[2].home);
        mt.loop();
        ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);
    }

#ifdef LEGACY
    {   // gearRatio: with default sv (1) we get full non-adaptive adjustment
        MachineThread mt = test_setup_FPD();
        Machine& machine = mt.machine;
        machine.op.probe.probeData[0] = -53.510;
        machine.op.probe.probeData[1] = -53.900;
        machine.op.probe.probeData[2] = -53.654;
        machine.op.probe.probeData[3] = -53.734;
        machine.op.probe.probeData[4] = -54.011;
        machine.op.probe.probeData[5] = -54.208;
        machine.op.probe.probeData[6] = -54.208;
        machine.op.probe.probeData[7] = -53.529;
        Serial.push(JT("{'cal':{'bx':'','by':'','bz':'','gr':'','ge':'','sv':'','zc':'','zr':''}}\n"));
        mt.loop();
        ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
        mt.loop();
        ASSERTEQUAL(STATUS_OK, mt.status);
        ASSERTEQUALS(JT("{'s':0,'r':{"
                        "'cal':{'bx':0.0038,'by':0.0061,'bz':-50.500,'gr':10.107,'ge':0.732,'sv':1.000,'zc':-53.520,'zr':-53.953}},"
                        "'t':0.000}\n"),
                     Serial.output().c_str());
        ASSERTEQUAL(-5659, machine.axis[0].home);
        ASSERTEQUAL(-5659, machine.axis[1].home);
        ASSERTEQUAL(-5659, machine.axis[2].home);
        mt.loop();
        ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);
    }
#endif

    {   // gearRatio: upward facing bowl
        MachineThread mt = test_setup_FPD();
        Machine& machine = mt.machine;
		PH5TYPE zCenter = -53;
		PH5TYPE zRim = zCenter + 0.1; // about 0.15 gear ratio error
        machine.op.probe.probeData[0] = zCenter;
        machine.op.probe.probeData[1] = 
        machine.op.probe.probeData[2] = 
        machine.op.probe.probeData[3] =
        machine.op.probe.probeData[4] =
        machine.op.probe.probeData[5] =
        machine.op.probe.probeData[6] = zRim;
        machine.op.probe.probeData[7] = zCenter;
        Serial.push(JT("{'cal':{'bx':'','by':'','bz':'','gr':'','ge':'','sv':'','zc':'','zr':''}}\n"));
        mt.loop();
        ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
        mt.loop();
        ASSERTEQUAL(STATUS_OK, mt.status);
        ASSERTEQUALS(JT("{'s':0,'r':{"
                        "'cal':{'bx':0.0000,'by':0.0000,'bz':-53.536,'gr':9.344,'ge':-0.129,'sv':1.000,'zc':-53.000,'zr':-52.900}},"
                        "'t':0.000}\n"),
                     Serial.output().c_str());
        ASSERTEQUAL(-5659, machine.axis[0].home);
        ASSERTEQUAL(-5659, machine.axis[1].home);
        ASSERTEQUAL(-5659, machine.axis[2].home);
        mt.loop();
        ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);
    }

    {   // gearRatio: downward facing bowl
        MachineThread mt = test_setup_FPD();
        Machine& machine = mt.machine;
		PH5TYPE zCenter = -53;
		PH5TYPE zRim = zCenter - 0.1; // about 0.15 gear ratio error
        machine.op.probe.probeData[0] = zCenter;
        machine.op.probe.probeData[1] = 
        machine.op.probe.probeData[2] = 
        machine.op.probe.probeData[3] =
        machine.op.probe.probeData[4] =
        machine.op.probe.probeData[5] =
        machine.op.probe.probeData[6] = zRim;
        machine.op.probe.probeData[7] = zCenter;
        Serial.push(JT("{'cal':{'bx':'','by':'','bz':'','gr':'','ge':'','sv':'','zc':'','zr':''}}\n"));
        mt.loop();
        ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
        mt.loop();
        ASSERTEQUAL(STATUS_OK, mt.status);
        ASSERTEQUALS(JT("{'s':0,'r':{"
                        "'cal':{'bx':0.0000,'by':0.0000,'bz':-52.292,'gr':9.642,'ge':0.169,'sv':1.000,'zc':-53.000,'zr':-53.100}},"
                        "'t':0.000}\n"),
                     Serial.output().c_str());
        ASSERTEQUAL(-5659, machine.axis[0].home);
        ASSERTEQUAL(-5659, machine.axis[1].home);
        ASSERTEQUAL(-5659, machine.axis[2].home);
        mt.loop();
        ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);
    }

    {   // gearRatio: downward facing bowl
        MachineThread mt = test_setup_FPD();
        Machine& machine = mt.machine;
		PH5TYPE zCenter = -53;
		PH5TYPE zRim = zCenter - 0.5; // excessive z-bowl error
        machine.op.probe.probeData[0] = zCenter;
        machine.op.probe.probeData[1] = 
        machine.op.probe.probeData[2] = 
        machine.op.probe.probeData[3] =
        machine.op.probe.probeData[4] =
        machine.op.probe.probeData[5] =
        machine.op.probe.probeData[6] = zRim;
        machine.op.probe.probeData[7] = zCenter;
        Serial.push(JT("{'cal':{'bx':'','by':'','bz':'','gr':'','ge':'','sv':'','zc':'','zr':''}}\n"));
        mt.loop();
        ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
        mt.loop();
        ASSERTEQUAL(STATUS_ZBOWL_GEAR, mt.status);
        ASSERTEQUALS(JT("{'s':-148,'r':{"
                        "'cal':{'bx':'','by':'','bz':'','gr':'','ge':'','sv':1.000,'zc':'','zr':''}},"
                        "'t':0.000}\n"),
                     Serial.output().c_str());
		mt.status = STATUS_WAIT_IDLE;
        ASSERTEQUAL(-5659, machine.axis[0].home);
        ASSERTEQUAL(-5659, machine.axis[1].home);
        ASSERTEQUAL(-5659, machine.axis[2].home);
        mt.loop();
        ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);
    }

#ifdef DISCARD
    {   // gearRatio: reversing error should solve for gear ratio
        MachineThread mt = test_setup_FPD();
        Machine& machine = mt.machine;
        machine.setHomePulses(-5058);
        machine.delta.setGearRatio(9.363);
        machine.op.probe.probeData[0] = -65.172;
        machine.op.probe.probeData[1] = -64.793;
        machine.op.probe.probeData[2] = -64.668;
        machine.op.probe.probeData[3] = -64.888;
        machine.op.probe.probeData[4] = -65.161;
        machine.op.probe.probeData[5] = -65.196;
        machine.op.probe.probeData[6] = -65.143;
        machine.op.probe.probeData[7] = -65.190;
        Serial.push(JT("{'cal':{'bx':'','by':'','bz':'','gr':'','ge':'','sv':'','zc':'','zr':''}}\n"));
        mt.loop();
        ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
        mt.loop();
        ASSERTEQUAL(STATUS_OK, mt.status);
        ASSERTEQUALS(JT("{'s':0,'r':{"
                        "'cal':{'bx':0.0043,'by':0.0044,'bz':-66.520,'gr':9.101,'ge':-0.262,'sv':1.000,'zc':-65.181,'zr':-64.975}},"
                        "'t':0.000}\n"),
                     Serial.output().c_str());
        ASSERTEQUALT(9.101, machine.delta.getGearRatio(),0.001);
        ASSERTEQUAL(-5058, machine.axis[0].home);
        ASSERTEQUAL(-5058, machine.axis[1].home);
        ASSERTEQUAL(-5058, machine.axis[2].home);
        mt.loop();
        ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);
		
		// Second data
        machine.op.probe.probeData[0] = -66.945;
        machine.op.probe.probeData[1] = -66.405;
        machine.op.probe.probeData[2] = -66.308;
        machine.op.probe.probeData[3] = -66.447;
        machine.op.probe.probeData[4] = -66.797;
        machine.op.probe.probeData[5] = -66.821;
        machine.op.probe.probeData[6] = -66.688;
        machine.op.probe.probeData[7] = -67.002;
        Serial.push(JT("{'cal':{'bx':'','by':'','bz':'','gr':'','ge':'','sv':'','zc':'','zr':''}}\n"));
        mt.loop();
        ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
        mt.loop();
        ASSERTEQUAL(STATUS_OK, mt.status);
        ASSERTEQUALS(JT("{'s':0,'r':{"
                        "'cal':{'bx':0.0037,'by':0.0042,'bz':-69.429,'gr':8.645,'ge':-0.456,'sv':1.000,'zc':-66.973,'zr':-66.578}},"
                        "'t':1.000}\n"),
                     Serial.output().c_str());
        ASSERTEQUAL(-5058, machine.axis[0].home);
        ASSERTEQUAL(-5058, machine.axis[1].home);
        ASSERTEQUAL(-5058, machine.axis[2].home);
        mt.loop();
        ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);


    }
#endif // DISCARD

    {   // bed Z-plane: with default sv (1) we get full non-adaptive adjustment
        MachineThread mt = test_setup_FPD();
        Machine& machine = mt.machine;
        machine.op.probe.probeData[0] = -53.510;
        machine.op.probe.probeData[1] = -53.900;
        machine.op.probe.probeData[2] = -53.654;
        machine.op.probe.probeData[3] = -53.734;
        machine.op.probe.probeData[4] = -54.011;
        machine.op.probe.probeData[5] = -54.208;
        machine.op.probe.probeData[6] = -54.208;
        machine.op.probe.probeData[7] = -53.529;
        Serial.push(JT("{'cal':{'bx':'','by':'','bz':'','sv':'','zc':'','zr':''}}\n"));
        mt.loop();
        ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
        mt.loop();
        ASSERTEQUAL(STATUS_OK, mt.status);
        ASSERTEQUALS(JT("{'s':0,'r':{"
                        "'cal':{'bx':0.0037,'by':0.0060,'bz':-53.956,'sv':1.000,'zc':-53.520,'zr':-53.953}},"
                        "'t':0.000}\n"),
                     Serial.output().c_str());
        ASSERTEQUAL(-5659, machine.axis[0].home);
        ASSERTEQUAL(-5659, machine.axis[1].home);
        ASSERTEQUAL(-5659, machine.axis[2].home);
        ASSERTEQUALT(FPD_GEAR_RATIO, machine.delta.getGearRatio(),0.001);
        mt.loop();
        ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);
    }

    {   // gearRatio and homeAngle: should split error
        MachineThread mt = test_setup_FPD();
        Machine& machine = mt.machine;
        machine.op.probe.probeData[0] = -53.510;
        machine.op.probe.probeData[1] = -53.900;
        machine.op.probe.probeData[2] = -53.654;
        machine.op.probe.probeData[3] = -53.734;
        machine.op.probe.probeData[4] = -54.011;
        machine.op.probe.probeData[5] = -54.208;
        machine.op.probe.probeData[6] = -54.208;
        machine.op.probe.probeData[7] = -53.529;
        ASSERTEQUAL(-5659, machine.axis[0].home);
        ASSERTEQUAL(-5659, machine.axis[1].home);
        ASSERTEQUAL(-5659, machine.axis[2].home);
        Serial.push(JT("{'cal':{'bx':'','by':'','bz':'','gr':'','ge':'','ha':'','he':'','sv':'','zc':'','zr':''}}\n"));
        mt.loop();
        ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
        mt.loop();
        ASSERTEQUAL(STATUS_OK, mt.status);
        ASSERTEQUALS(JT("{'s':0,'r':{"
                        "'cal':{'bx':0.0037,'by':0.0060,'bz':-53.194,'gr':9.630,'ge':0.156,"
                        "'ha':-58.668,'he':8.531,'sv':1.000,'zc':-53.520,'zr':-53.953}},"
                        "'t':0.000}\n"),
                     Serial.output().c_str());
        ASSERTEQUAL(-4941, machine.axis[0].home);
        ASSERTEQUAL(-4941, machine.axis[1].home);
        ASSERTEQUAL(-4941, machine.axis[2].home);
        ASSERTEQUALT(-58.668, machine.getHomeAngle(), 0.001);
        mt.loop();
        ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);
    }

    {   // cal should be same as specifying all attributes
        MachineThread mt = test_setup_FPD();
        Machine& machine = mt.machine;
        machine.op.probe.probeData[0] = -53.510;
        machine.op.probe.probeData[1] = -53.900;
        machine.op.probe.probeData[2] = -53.654;
        machine.op.probe.probeData[3] = -53.734;
        machine.op.probe.probeData[4] = -54.011;
        machine.op.probe.probeData[5] = -54.208;
        machine.op.probe.probeData[6] = -54.208;
        machine.op.probe.probeData[7] = -53.529;
        ASSERTEQUAL(-5659, machine.axis[0].home);
        ASSERTEQUAL(-5659, machine.axis[1].home);
        ASSERTEQUAL(-5659, machine.axis[2].home);
        Serial.push(JT("{'cal':''}\n"));
        mt.loop();
        ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
        mt.loop();
        ASSERTEQUAL(STATUS_OK, mt.status);
        ASSERTEQUALS(JT("{'s':0,'r':{"
                        "'cal':{'bx':0.0037,'by':0.0060,'bz':-53.194,'gr':9.630,'ge':0.156,"
                        "'ha':-58.668,'he':8.531,'sv':1.000,'zc':-53.520,'zr':-53.953}},"
                        "'t':0.000}\n"),
                     Serial.output().c_str());
        ASSERTEQUAL(-4941, machine.axis[0].home);
        ASSERTEQUAL(-4941, machine.axis[1].home);
        ASSERTEQUAL(-4941, machine.axis[2].home);
        ASSERTEQUALT(-58.668, machine.getHomeAngle(), 0.001);
        mt.loop();
        ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);
    }

    cout << "TEST	: test_calibrate() OK " << endl;
}

void test_mark() {
    cout << "TEST	: test_mark() =====" << endl;

    {   // MTO_RAW
        MachineThread mt = test_setup();
        Machine &machine = mt.machine;

        for (int8_t i=0; i<MARK_COUNT; i++) {
            machine.marks[i] = (i+1)*10;
        }
        machine.axis[0].position = 100;
        machine.axis[1].position = 200;
        machine.axis[2].position = 300;
        machine.axis[3].position = 400;

        // mrk: you can get/set a mark value
        Serial.push(JT("{'mrk':''}\n"));
        mt.loop();
        ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
        mt.loop();
        ASSERTEQUAL(STATUS_OK, mt.status);
        ASSERTEQUALS(JT("{'s':0,'r':"
                        "{'mrk':{'m1':10.000,'m2':20.000,'m3':30.000,'m4':40.000,"
                        "'m5':50.000,'m6':60.000,'m7':70.000,'m8':80.000,'m9':90.000}"
                        "},'t':0.000}\n"),
                     Serial.output().c_str());
        mt.loop();
        ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

        // mrk: you can set a mark value from an axis position
        Serial.push(JT("{'mrka1':1,'mrka2':2,'mrkaz':3,'mrkm1':'','mrkm2':'','mrkm3':''}\n"));
        mt.loop();
        ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
        mt.loop();
        ASSERTEQUAL(STATUS_OK, mt.status);
        ASSERTEQUALS(JT("{'s':0,'r':"
                        "{'mrka1':1,'mrka2':2,'mrkaz':3,'mrkm1':100.000,'mrkm2':200.000,'mrkm3':300.000}"
                        ",'t':0.000}\n"),
                     Serial.output().c_str());
        mt.loop();
        ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);
    }

    {   // MTO_FPD
        MachineThread mt = test_MTO_FPD_setup();
        Machine &machine(mt.machine);
        machine.marks[0] = 10;
        machine.marks[1] = 20;
        machine.marks[2] = 30;
        machine.marks[3] = 40;
        XYZ3D xyz(1.11,2.22,3.33);
        machine.loadDeltaCalculator();
        Step3D pulses = machine.delta.calcPulses(xyz);
        machine.axis[0].position = pulses.p1;
        machine.axis[1].position = pulses.p2;
        machine.axis[2].position = pulses.p3;
        ASSERTQUAD(Quad<StepCoord>(-127,-228,-183,0), machine.getMotorPosition());

        // mrk: you can set a mark value from a Cartesian position
        Serial.push(JT("{'mrkax':1,'mrkay':2,'mrkaz':3,'mrkm1':'','mrkm2':'','mrkm3':'','mrka1':5,'mrka2':6,'mrka3':7}\n"));
        mt.loop();
        ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
        mt.loop();
        ASSERTEQUAL(STATUS_OK, mt.status);
        ASSERTEQUALS(JT("{'s':0,'r':"
                        "{'mrkax':1,'mrkay':2,'mrkaz':3,'mrkm1':1.104,'mrkm2':2.225,'mrkm3':3.330,'mrka1':5,'mrka2':6,'mrka3':7}"
                        ",'t':0.000}\n"),
                     Serial.output().c_str());
        ASSERTEQUAL(-127, machine.marks[4]);
        ASSERTEQUAL(-228, machine.marks[5]);
        ASSERTEQUAL(-183, machine.marks[6]);
        mt.loop();
        ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

        // move away from mark
        int32_t xpulses = arduino.pulses(PC2_X_STEP_PIN);
        int32_t ypulses = arduino.pulses(PC2_Y_STEP_PIN);
        int32_t zpulses = arduino.pulses(PC2_Z_STEP_PIN);
        Serial.push(JT("{'movz':-1}\n"));
        mt.loop();
        ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
        mt.loop();
        ASSERTEQUAL(STATUS_OK, mt.status);
        ASSERTQUAD(Quad<StepCoord>(105,6,51,0), machine.getMotorPosition());
        ASSERTEQUAL(232, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
        ASSERTEQUAL(234, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
        ASSERTEQUAL(234, arduino.pulses(PC2_Z_STEP_PIN)-zpulses);
        ASSERTEQUALS(JT("{'s':0,'r':{'movz':-1.000},'t':0.226}\n"),
                     Serial.output().c_str());
        mt.loop();
        ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

        // mark waypoint 4
        xpulses = arduino.pulses(PC2_X_STEP_PIN);
        ypulses = arduino.pulses(PC2_Y_STEP_PIN);
        zpulses = arduino.pulses(PC2_Z_STEP_PIN);
        Serial.push(JT("{'mrkwp':4}\n"));
        mt.loop();
        ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
        mt.loop();
        ASSERTEQUAL(STATUS_OK, mt.status);
        ASSERTQUAD(Quad<StepCoord>(105,6,51,0), machine.getMotorPosition());
        ASSERTEQUAL(0, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
        ASSERTEQUAL(0, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
        ASSERTEQUAL(0, arduino.pulses(PC2_Z_STEP_PIN)-zpulses);
        ASSERTEQUALS(JT("{'s':0,'r':{'mrkwp':4},'t':0.000}\n"),
                     Serial.output().c_str());
        ASSERTEQUALT(1.128, machine.marks[3], 0.001);
        ASSERTEQUALT(2.21, machine.marks[4], 0.005);
        ASSERTEQUALT(-1.00, machine.marks[5], 0.005);
        mt.loop();
        ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

        // return to mark
        xpulses = arduino.pulses(PC2_X_STEP_PIN);
        ypulses = arduino.pulses(PC2_Y_STEP_PIN);
        zpulses = arduino.pulses(PC2_Z_STEP_PIN);
        Serial.push(JT("{'mov':{'zm':3,'ym':2,'xm':1}}\n"));
        mt.loop();
        ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
        mt.loop();
        ASSERTEQUAL(STATUS_OK, mt.status);
        ASSERTQUAD(Quad<StepCoord>(-127,-228,-183,0), machine.getMotorPosition());
        ASSERTEQUAL(232, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
        ASSERTEQUAL(234, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
        ASSERTEQUAL(234, arduino.pulses(PC2_Z_STEP_PIN)-zpulses);
        ASSERTEQUALS(JT("{'s':0,'r':{'mov':{'zm':3,'ym':2,'xm':1}},'t':0.226}\n"),
                     Serial.output().c_str());
        mt.loop();
        ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

        // return to waypoint 4
        xpulses = arduino.pulses(PC2_X_STEP_PIN);
        ypulses = arduino.pulses(PC2_Y_STEP_PIN);
        zpulses = arduino.pulses(PC2_Z_STEP_PIN);
        Serial.push(JT("{'movwp':4}\n"));
        mt.loop();
        ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
        mt.loop();
        ASSERTEQUAL(STATUS_OK, mt.status);
        ASSERTQUAD(Quad<StepCoord>(105,6,51,0), machine.getMotorPosition());
        ASSERTEQUAL(232, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
        ASSERTEQUAL(234, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
        ASSERTEQUAL(234, arduino.pulses(PC2_Z_STEP_PIN)-zpulses);
        ASSERTEQUALS(JT("{'s':0,'r':{'movwp':4},'t':0.226}\n"),
                     Serial.output().c_str());
        mt.loop();
        ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

    }

    cout << "TEST	: test_mark() OK " << endl;
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
                    "'sc':2,'us':1873817,'dp':[1556,7742,-4881]}},'t':1.903}\n"),
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
    ASSERTEQUALS(JT("{'s':0,'r':{'dvs':{'us':5000000,'x':50}},'t':5.702}\n"), Serial.output().c_str());
    ASSERTEQUAL(50, arduino.pulses(PC2_X_STEP_PIN) - xpulses);
    ASSERTQUAD(Quad<StepCoord>(150, 100, 100, 100), machine.getMotorPosition());

    cout << "TEST	: test_dvs() OK " << endl;
}

void test_error(MachineThread &mt, const char * cmd, Status status, const char *output = NULL) {
    TESTCOUT1("test_error: ", cmd);
    Serial.push(JT(cmd));
    test_ticks(1); // parse
    if (mt.status == STATUS_BUSY_PARSED) {
        test_ticks(1); // initialize
    }
    ASSERTEQUAL(status, mt.status);
    if (output) {
        ASSERTEQUALS(JT(output), Serial.output().c_str());
    }
}

void test_errors() {
    cout << "TEST	: test_errors() =====" << endl;

    MachineThread mt = test_setup();
    Machine &machine = mt.machine;

    test_error(mt, "{'abc':true}\n", STATUS_UNRECOGNIZED_NAME,
               "{'s':-402,'r':{'abc':true},'e':'abc','t':0.000}\n");
    test_error(mt, "{bad-json}\n", STATUS_JSON_PARSE_ERROR, "{'s':-403}\n");
    test_error(mt, "bad-json\n", STATUS_JSON_PARSE_ERROR, "{'s':-403}\n");
    test_error(mt, "{'xud':50000}\n", STATUS_VALUE_RANGE, "{'s':-133,'r':{'xud':50000},'t':0.000}\n");

    cout << "TEST	: test_errors() OK " << endl;
}

void test_Idle() {
    cout << "TEST	: test_Idle() =====" << endl;

    MachineThread mt = test_setup();
    Machine &machine = mt.machine;
    int32_t xenpulses = arduino.pulses(PC2_X_ENABLE_PIN);

    test_ticks(1);
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);
    ASSERTEQUAL(0, arduino.pulses(PC2_X_ENABLE_PIN)-xenpulses);

    test_ticks(1);
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);
    ASSERTEQUAL(0, arduino.pulses(PC2_X_ENABLE_PIN)-xenpulses);

    test_ticks(1);
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);
    ASSERTEQUAL(0, arduino.pulses(PC2_X_ENABLE_PIN)-xenpulses);

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
    ASSERTEQUALS(JT("{\r\n  's': 0,\r\n  'r': {\r\n    'sysjp': true\r\n  },\r\n  't': 0.000\r\n}\n"),
                 Serial.output().c_str());

    cout << "TEST	: test_PrettyPrint() OK " << endl;
}

void test_autoSync() {
    cout << "TEST	: test_autoSync() =====" << endl;

    char buf[100];
    char *out = saveConfigValue("tv", (PH5TYPE) 1.2345, buf);
    ASSERTEQUALS("\"tv\":1.23,", buf);
    ASSERTEQUAL(strlen(buf), (size_t)(out-buf));
    out = saveConfigValue("tv", (PH5TYPE) -1.2345, buf);
    ASSERTEQUALS("\"tv\":-1.23,", buf);
    ASSERTEQUAL(strlen(buf), (size_t)(out-buf));
    out = saveConfigValue("as", true, buf);
    ASSERTEQUALS("\"as\":1,", buf);
    ASSERTEQUAL(strlen(buf), (size_t)(out-buf));
    out = saveConfigValue("as", false, buf);
    ASSERTEQUALS("\"as\":0,", buf);
    ASSERTEQUAL(strlen(buf), (size_t)(out-buf));

    arduino.clear();
    threadRunner.clear();
    MachineThread mt;
    mt.machine.pDisplay = &testDisplay;
    mt.setup(PC1_EMC02);

    Machine & machine = mt.machine;
    testDisplay.clear();

    ASSERTEQUAL(1000, MAX_JSON);
    uint8_t *eeaddr = 0;
    ASSERTEQUAL(0, machine.syncHash);
    ASSERT(!machine.autoSync);
    ASSERTEQUAL(STATUS_BUSY_SETUP, mt.status);
    mt.loop();
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);
    mt.loop();
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);
    Serial.clear(); // banner
    ASSERT(machine.syncHash);
    int32_t hash1 = machine.syncHash;
    ASSERTEQUAL(hash1, machine.hash());
    machine.axis[4].enable(false);
    machine.axis[5].enable(false);
    int32_t hash2 = machine.hash();
    ASSERT(hash1 != hash2);

    // enable auto-sync
    Serial.push(JT("{'sysas':1,'sysom':3,'sysah':1,'syspc':2}\n"));
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    ASSERTEQUAL(false, machine.axis[4].isEnabled());
    mt.loop();
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERT(machine.autoSync);
    int32_t hash3 = machine.hash();
    ASSERT(hash2 != hash3);
    ASSERTEQUAL(false, machine.axis[4].isEnabled());
#define HASH3 "1131203205"
    snprintf(buf, sizeof(buf), "%ld", (long) hash3);
    ASSERTEQUALS(HASH3, buf);

    mt.loop();
    string eeprom3 = eeprom_read_string(0);
    ASSERTEQUALS(JT( "["
                     "{'sys':{'ch':" HASH3 ",'pc':2,'to':0,'ah':1,'db':0,'hp':3,'jp':0,"
                     "'lh':0,'mv':12800,'om':3,'pb':2,'pi':11,'tv':0.70}},"
                     "{'x':{'dh':1,'en':1,'ho':0,'is':0,'lb':200,'mi':16,'sa':1.8,'tm':32000,'tn':-32000,'ud':0}},"
                     "{'y':{'dh':1,'en':1,'ho':0,'is':0,'lb':200,'mi':16,'sa':1.8,'tm':32000,'tn':-32000,'ud':0}},"
                     "{'z':{'dh':1,'en':1,'ho':0,'is':0,'lb':200,'mi':16,'sa':1.8,'tm':32000,'tn':-32000,'ud':0}},"
                     "{'a':{'dh':1,'en':1,'ho':0,'is':0,'lb':200,'mi':16,'sa':1.8,'tm':32000,'tn':-32000,'ud':0}},"
                     "{'b':{'en':0}},"
                     "{'c':{'en':0}},"
                     "{'hom':''}]"),
                 eeprom3.c_str());
    snprintf(buf, sizeof(buf), "%ld", (long) machine.syncHash);
    ASSERTEQUALS(HASH3, buf);
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);
    mt.loop();
    ASSERT(hash1 != machine.syncHash);
    mt.loop();
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);
    ASSERTEQUAL(hash3, machine.hash());
    ASSERTEQUAL(hash3, machine.syncHash);
    mt.loop();
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

    // simulate restart
    machine.syncHash = 0;
    mt.printBannerOnIdle = true;
    machine.autoSync = false;
    mt.status = STATUS_BUSY_SETUP;
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_EEPROM, mt.status);
    mt.loop(); // parse startup JSON
    Serial.clear(); // banner
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop(); // sys
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    ASSERTEQUAL(false, machine.autoSync); // autoSync must always be enabled explicitly
    ASSERTEQUALS(eeprom3.c_str(), eeprom_read_string(0).c_str());
    ASSERTEQUALS(JT("{'s':0,'r':"
                    "{'sys':{'ch':" HASH3 ",'pc':2,'to':0,'ah':true,'db':0,'hp':3,"
                    "'jp':false,'lh':false,'mv':12800,'om':3,'pb':2,'pi':11,'tv':0.700}},"
                    "'t':0.000}\n"),
                 Serial.output().c_str());
    mt.loop(); // x
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop(); // y
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop(); // z
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop(); // a
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop(); // b
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop(); // c
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop(); // home
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    arduino.setPin(PC2_X_MIN_PIN, HIGH);
    arduino.setPin(PC2_Y_MIN_PIN, HIGH);
    arduino.setPin(PC2_Z_MIN_PIN, HIGH);
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_CALIBRATING, mt.status);
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop();
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUAL(hash3, machine.hash());
    ASSERTEQUAL(false, machine.autoSync);
    Serial.clear();
    mt.loop();
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);
    mt.loop(); // banner
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);
    snprintf(buf, sizeof(buf), "FireStep %d.%d.%d sysch:%s\n",
             VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, HASH3);
    ASSERTEQUALS(buf, Serial.output().c_str());

    cout << "TEST	: test_autoSync() OK " << endl;
}

void test_eep() {
    cout << "TEST	: test_eep() =====" << endl;

    uint8_t *eeaddr = 0;
    arduino.clear();
    eeprom_write_byte(eeaddr++, '{');
    eeprom_write_byte(eeaddr++, '"');
    eeprom_write_byte(eeaddr++, 's');
    eeprom_write_byte(eeaddr++, 'y');
    eeprom_write_byte(eeaddr++, 's');
    eeprom_write_byte(eeaddr++, 'f');
    eeprom_write_byte(eeaddr++, 'r');
    eeprom_write_byte(eeaddr++, '"');
    eeprom_write_byte(eeaddr++, ':');
    eeprom_write_byte(eeaddr++, '"');
    eeprom_write_byte(eeaddr++, '"');
    eeprom_write_byte(eeaddr++, '}');
    MachineThread mt = test_setup(false);
    Machine &machine = mt.machine;
    ASSERTEQUAL(STATUS_BUSY_EEPROM, mt.status);
    mt.loop();
    Serial.clear(); // banner
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop();
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUALS(JT("{'s':0,'r':{'sysfr':1000},'t':0.000}\n"), Serial.output().c_str());
    test_ticks(1);
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

    Serial.push(JT("{'eep':{'100':''}}\n"));
    test_ticks(1);
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    test_ticks(1);
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUALS(JT("{'s':0,'r':{'eep':{'100':''}},'t':0.000}\n"), Serial.output().c_str());
    test_ticks(1);

    Serial.push(JT("{'eep100':{'sysv':''}}\n"));
    test_ticks(1);
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    test_ticks(1);
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUALS(JT("{'s':0,'r':{'eep100':{'sysv':''}},'t':0.000}\n"), Serial.output().c_str());
    test_ticks(1);

    Serial.push(JT("{'eep100':''}\n"));
    test_ticks(1);
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    test_ticks(1);
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUALS(JT("{'s':0,'r':{'eep100':'{\\\"sysv\\\":\\\"\\\"}'},'t':0.000}\n"), Serial.output().c_str());
    test_ticks(1);

    Serial.push(JT("{'eep':{'123':'hello'}}\n"));
    test_ticks(1);
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    test_ticks(1);
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUALS(JT("{'s':0,'r':{'eep':{'123':'hello'}},'t':0.000}\n"), Serial.output().c_str());
    test_ticks(1);

    Serial.push(JT("{'eep':{'124':''}}\n"));
    test_ticks(1);
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    test_ticks(1);
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUALS(JT("{'s':0,'r':{'eep':{'124':'ello'}},'t':0.000}\n"), Serial.output().c_str());
    test_ticks(1);

    Serial.push(JT("{'eep':{'0':''}}\n"));
    test_ticks(1);
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    test_ticks(1);
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUALS(JT("{'s':0,'r':{'eep':{'0':'{\\\"sysfr\\\":\\\"\\\"}'}},'t':0.000}\n"), Serial.output().c_str());
    test_ticks(1);

    Serial.push(JT("{'eep!0':{'sysmv':''}}\n"));
    test_ticks(1);
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    test_ticks(1);
    ASSERTEQUAL(STATUS_OK, mt.status);
    test_ticks(1);
    ASSERTEQUALS(JT("{'s':0,'r':{'eep!0':{'sysmv':12800}},'t':0.000}\n"), Serial.output().c_str());
    Serial.push(JT("{'eep':{'!0':{'systv':''}}}\n"));
    test_ticks(1);
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    test_ticks(1);
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUALS(JT("{'s':0,'r':{'eep':{'!0':{'systv':0.700}}},'t':0.000}\n"), Serial.output().c_str());
    test_ticks(1);
    Serial.push(JT("{'eep':{'0':''}}\n"));
    test_ticks(1);
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    test_ticks(1);
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUALS(JT("{'s':0,'r':{'eep':{'0':'{\\\"systv\\\":0.700}'}},'t':0.000}\n"), Serial.output().c_str());
    test_ticks(1);
    ASSERTEQUALS(JT("{'systv':0.700}"), eeprom_read_string(0).c_str());

    // test restart
    mt.status = STATUS_BUSY_SETUP;
    machine.tvMax = 0.5;
    test_ticks(1);
    ASSERTEQUAL(STATUS_BUSY_EEPROM, mt.status);
    ASSERTEQUALT(0.5, machine.tvMax, 0.0001);
    test_ticks(1);
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    ASSERTEQUALT(0.5, machine.tvMax, 0.0001);
    test_ticks(1);
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    test_ticks(1);
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUALT(0.7, machine.tvMax, 0.0001);
    test_ticks(1);
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);
    ASSERTEQUALT(0.7, machine.tvMax, 0.0001);

    // test eeUser
    Serial.clear();
    Serial.push(JT("{'sysom':1,'eep':{'2000':{'systv':0.6}}}\n"));
    test_ticks(1);
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    test_ticks(1);
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUALS(JT("{'s':0,'r':{'sysom':1,'eep':"\
                    "{'2000':{'systv':0.6}}},'t':0.000}\n"),
                 Serial.output().c_str());
    test_ticks(1);
    mt.status = STATUS_BUSY_SETUP;
    test_ticks(1);
    ASSERTEQUAL(STATUS_BUSY_EEPROM, mt.status);
    test_ticks(1); // parse EEPROM
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    test_ticks(1); // system EEPROM
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    ASSERTEQUALT(0.7, machine.tvMax, 0.0001);
    test_ticks(1);
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUAL(false, machine.isEEUserEnabled());
    test_ticks(1);
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

    Serial.clear();
    Serial.push(JT("{'syseu':1}\n"));
    test_ticks(1);
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    test_ticks(1);
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUALS(JT("{'s':0,'r':{'syseu':true},'t':0.000}\n"),
                 Serial.output().c_str());
    test_ticks(1);
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);
    ASSERTEQUAL(true, machine.isEEUserEnabled());
    mt.status = STATUS_BUSY_SETUP;
    test_ticks(1);
    ASSERTEQUAL(STATUS_BUSY_EEPROM, mt.status);
    test_ticks(1); // parse EEPROM
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    test_ticks(1); // system EEPROM
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    test_ticks(1); // user EEPROM (enabled)
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    test_ticks(1);
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUALT(0.6, machine.tvMax, 0.0001);
    test_ticks(1);
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

    cout << "TEST	: test_eep() OK " << endl;
}

void test_io() {
    cout << "TEST	: test_io() =====" << endl;

    MachineThread mt = test_setup();
    Machine &machine = mt.machine;

    arduino.setPin(22, HIGH);
    Serial.push(JT("{'io':{'d22':''}}\n"));
    test_ticks(1);
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    test_ticks(1);
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUAL(INPUT, arduino.getPinMode(22));
    ASSERTEQUALS(JT("{'s':0,'r':{'io':{'d22':true}},'t':0.000}\n"), Serial.output().c_str());
    test_ticks(1);

    arduino.setPin(22, LOW);
    Serial.push(JT("{'iod22':''}\n"));
    test_ticks(1);
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    test_ticks(1);
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUALS(JT("{'s':0,'r':{'iod22':false},'t':0.000}\n"), Serial.output().c_str());
    ASSERTEQUAL(INPUT, arduino.getPinMode(22));
    test_ticks(1);

    Serial.push(JT("{'io':{'d220':''}}\n"));
    test_ticks(1);
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    test_ticks(1);
    ASSERTEQUAL(STATUS_NO_SUCH_PIN, mt.status);
    ASSERTEQUALS(JT("{'s':-136,'r':{'io':{'d220':''}},'e':'d220','t':0.000}\n"), Serial.output().c_str());
    test_ticks(1);

    Serial.push(JT("{'iod22':true}\n"));
    test_ticks(1);
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    test_ticks(1);
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUALS(JT("{'s':0,'r':{'iod22':true},'t':0.000}\n"), Serial.output().c_str());
    ASSERT(arduino.getPin(22));
    ASSERTEQUAL(OUTPUT, arduino.getPinMode(22));
    test_ticks(1);

    Serial.push(JT("{'io':{'d22':0}}\n"));
    test_ticks(1);
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    test_ticks(1);
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUALS(JT("{'s':0,'r':{'io':{'d22':0}},'t':0.000}\n"), Serial.output().c_str());
    ASSERT(!arduino.getPin(22));
    ASSERTEQUAL(OUTPUT, arduino.getPinMode(22));
    test_ticks(1);

    Serial.push(JT("{'io':{'d22':1}}\n"));
    test_ticks(1);
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    test_ticks(1);
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUALS(JT("{'s':0,'r':{'io':{'d22':1}},'t':0.000}\n"), Serial.output().c_str());
    ASSERT(arduino.getPin(22));
    ASSERTEQUAL(OUTPUT, arduino.getPinMode(22));
    test_ticks(1);

    Serial.push(JT("{'io':{'a6':123}}\n"));
    test_ticks(1);
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    test_ticks(1);
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUAL(OUTPUT, arduino.getPinMode(A6));
    ASSERTEQUALS(JT("{'s':0,'r':{'io':{'a6':123}},'t':0.000}\n"), Serial.output().c_str());
    test_ticks(1);

    Serial.push(JT("{'io':{'a6':''}}\n"));
    test_ticks(1);
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    test_ticks(1);
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUAL(INPUT, arduino.getPinMode(A6));
    ASSERTEQUALS(JT("{'s':0,'r':{'io':{'a6':123}},'t':0.000}\n"), Serial.output().c_str());
    test_ticks(1);

    cout << "TEST	: test_io() OK " << endl;
}

void test_probe() {
    cout << "TEST	: test_probe() =====" << endl;

    MachineThread mt = test_setup();
    Machine &machine = mt.machine;
    int32_t xpulses = arduino.pulses(PC2_X_STEP_PIN);
    int32_t ypulses = arduino.pulses(PC2_Y_STEP_PIN);
    int32_t zpulses = arduino.pulses(PC2_Z_STEP_PIN);
    int32_t e0pulses = arduino.pulses(PC2_E0_STEP_PIN);
    machine.setMotorPosition(Quad<StepCoord>(100, 100, 100, 100));
    ASSERTEQUAL(NOVALUE, arduino.getPin(PC2_PROBE_PIN));
    arduino.setPin(PC2_PROBE_PIN, LOW);

    Serial.push(JT("{'prb':{'1':99,'2':95,'3':90,'pn':''}}\n"));
    test_ticks(1);	// parse
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    ASSERTQUAD(Quad<StepCoord>(100, 100, 100, 100), mt.machine.getMotorPosition());

    test_ticks(1);	// initialize
    ASSERTEQUAL(STATUS_BUSY_CALIBRATING, mt.status);
    ASSERT(machine.op.probe.probing);
    ASSERTQUAD(Quad<StepCoord>(100, 100, 100, 100), mt.machine.getMotorPosition());

    test_ticks(1);	// calibrating
    ASSERTEQUAL(STATUS_BUSY_CALIBRATING, mt.status);
    ASSERT(machine.op.probe.probing);
    ASSERTEQUAL(0, arduino.pulses(PC2_E0_STEP_PIN)-e0pulses);
    ASSERTEQUAL(1, arduino.pulses(PC2_Z_STEP_PIN)-zpulses);
    ASSERTEQUAL(0, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
    ASSERTEQUAL(0, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    ASSERTQUAD(Quad<StepCoord>(100, 100, 99, 100), mt.machine.getMotorPosition());

    test_ticks(1);	// calibrating
    ASSERTEQUAL(STATUS_BUSY_CALIBRATING, mt.status);
    ASSERT(machine.op.probe.probing);
    ASSERTEQUAL(0, arduino.pulses(PC2_E0_STEP_PIN)-e0pulses);
    ASSERTEQUAL(2, arduino.pulses(PC2_Z_STEP_PIN)-zpulses);
    ASSERTEQUAL(1, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
    ASSERTEQUAL(0, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    ASSERTQUAD(Quad<StepCoord>(100, 99, 98, 100), mt.machine.getMotorPosition());

    test_ticks(1);	// calibrating
    ASSERTEQUAL(STATUS_BUSY_CALIBRATING, mt.status);
    ASSERT(machine.op.probe.probing);
    ASSERTEQUAL(0, arduino.pulses(PC2_E0_STEP_PIN)-e0pulses);
    ASSERTEQUAL(3, arduino.pulses(PC2_Z_STEP_PIN)-zpulses);
    ASSERTEQUAL(1, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
    ASSERTEQUAL(0, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    ASSERTQUAD(Quad<StepCoord>(100, 99, 97, 100), mt.machine.getMotorPosition());

    test_ticks(1);	// calibrating
    ASSERTEQUAL(STATUS_BUSY_CALIBRATING, mt.status);
    ASSERT(machine.op.probe.probing);
    ASSERTEQUAL(0, arduino.pulses(PC2_E0_STEP_PIN)-e0pulses);
    ASSERTEQUAL(4, arduino.pulses(PC2_Z_STEP_PIN)-zpulses);
    ASSERTEQUAL(2, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
    ASSERTEQUAL(0, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    ASSERTQUAD(Quad<StepCoord>(100, 98, 96, 100), mt.machine.getMotorPosition());

    test_ticks(1);	// calibrating
    ASSERTEQUAL(STATUS_BUSY_CALIBRATING, mt.status);
    ASSERT(machine.op.probe.probing);
    ASSERTEQUAL(0, arduino.pulses(PC2_E0_STEP_PIN)-e0pulses);
    ASSERTEQUAL(5, arduino.pulses(PC2_Z_STEP_PIN)-zpulses);
    ASSERTEQUAL(2, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
    ASSERTEQUAL(0, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    ASSERTQUAD(Quad<StepCoord>(100, 98, 95, 100), mt.machine.getMotorPosition());

    test_ticks(1);	// calibrating
    ASSERTEQUAL(STATUS_BUSY_CALIBRATING, mt.status);
    ASSERT(machine.op.probe.probing);
    ASSERTEQUAL(0, arduino.pulses(PC2_E0_STEP_PIN)-e0pulses);
    ASSERTEQUAL(6, arduino.pulses(PC2_Z_STEP_PIN)-zpulses);
    ASSERTEQUAL(3, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
    ASSERTEQUAL(1, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    ASSERTQUAD(Quad<StepCoord>(99, 97, 94, 100), mt.machine.getMotorPosition());

    arduino.setPin(PC2_PROBE_PIN, HIGH);
    test_ticks(1);	// tripped
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERT(!machine.op.probe.probing);
    ASSERTEQUAL(0, arduino.pulses(PC2_E0_STEP_PIN)-e0pulses);
    ASSERTEQUAL(6, arduino.pulses(PC2_Z_STEP_PIN)-zpulses);
    ASSERTEQUAL(3, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
    ASSERTEQUAL(1, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    ASSERTQUAD(Quad<StepCoord>(99, 97, 94, 100), mt.machine.getMotorPosition());
    ASSERTEQUALS(JT("{'s':0,'r':{'prb':{'1':99,'2':97,'3':94,'pn':2}},'t':0.001}\n"),
                 Serial.output().c_str());
    test_ticks(1);	// tripped
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

    // Invert pin sense
    xpulses = arduino.pulses(PC2_X_STEP_PIN);
    ypulses = arduino.pulses(PC2_Y_STEP_PIN);
    zpulses = arduino.pulses(PC2_Z_STEP_PIN);
    e0pulses = arduino.pulses(PC2_E0_STEP_PIN);
    machine.setMotorPosition(Quad<StepCoord>(100, 100, 100, 100));
    arduino.setPin(PC2_PROBE_PIN, HIGH);

    Serial.push(JT("{'prb':{'1':99,'2':95,'3':90,'pn':2,'ip':true}}\n"));
    test_ticks(1);	// parse
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    ASSERTQUAD(Quad<StepCoord>(100, 100, 100, 100), mt.machine.getMotorPosition());

    test_ticks(1);	// initialize
    ASSERTEQUAL(STATUS_BUSY_CALIBRATING, mt.status);
    ASSERT(machine.op.probe.probing);
    ASSERTQUAD(Quad<StepCoord>(100, 100, 100, 100), mt.machine.getMotorPosition());

    test_ticks(1);	// calibrating
    ASSERTEQUAL(STATUS_BUSY_CALIBRATING, mt.status);
    ASSERT(machine.op.probe.probing);
    ASSERTEQUAL(0, arduino.pulses(PC2_E0_STEP_PIN)-e0pulses);
    ASSERTEQUAL(1, arduino.pulses(PC2_Z_STEP_PIN)-zpulses);
    ASSERTEQUAL(0, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
    ASSERTEQUAL(0, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    ASSERTQUAD(Quad<StepCoord>(100, 100, 99, 100), mt.machine.getMotorPosition());

    test_ticks(1);	// calibrating
    ASSERTEQUAL(STATUS_BUSY_CALIBRATING, mt.status);
    ASSERT(machine.op.probe.probing);
    ASSERTEQUAL(0, arduino.pulses(PC2_E0_STEP_PIN)-e0pulses);
    ASSERTEQUAL(2, arduino.pulses(PC2_Z_STEP_PIN)-zpulses);
    ASSERTEQUAL(1, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
    ASSERTEQUAL(0, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    ASSERTQUAD(Quad<StepCoord>(100, 99, 98, 100), mt.machine.getMotorPosition());

    test_ticks(1);	// calibrating
    ASSERTEQUAL(STATUS_BUSY_CALIBRATING, mt.status);
    ASSERT(machine.op.probe.probing);
    ASSERTEQUAL(0, arduino.pulses(PC2_E0_STEP_PIN)-e0pulses);
    ASSERTEQUAL(3, arduino.pulses(PC2_Z_STEP_PIN)-zpulses);
    ASSERTEQUAL(1, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
    ASSERTEQUAL(0, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    ASSERTQUAD(Quad<StepCoord>(100, 99, 97, 100), mt.machine.getMotorPosition());

    test_ticks(1);	// calibrating
    ASSERTEQUAL(STATUS_BUSY_CALIBRATING, mt.status);
    ASSERT(machine.op.probe.probing);
    ASSERTEQUAL(0, arduino.pulses(PC2_E0_STEP_PIN)-e0pulses);
    ASSERTEQUAL(4, arduino.pulses(PC2_Z_STEP_PIN)-zpulses);
    ASSERTEQUAL(2, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
    ASSERTEQUAL(0, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    ASSERTQUAD(Quad<StepCoord>(100, 98, 96, 100), mt.machine.getMotorPosition());

    test_ticks(1);	// calibrating
    ASSERTEQUAL(STATUS_BUSY_CALIBRATING, mt.status);
    ASSERT(machine.op.probe.probing);
    ASSERTEQUAL(0, arduino.pulses(PC2_E0_STEP_PIN)-e0pulses);
    ASSERTEQUAL(5, arduino.pulses(PC2_Z_STEP_PIN)-zpulses);
    ASSERTEQUAL(2, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
    ASSERTEQUAL(0, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    ASSERTQUAD(Quad<StepCoord>(100, 98, 95, 100), mt.machine.getMotorPosition());

    test_ticks(1);	// calibrating
    ASSERTEQUAL(STATUS_BUSY_CALIBRATING, mt.status);
    ASSERT(machine.op.probe.probing);
    ASSERTEQUAL(0, arduino.pulses(PC2_E0_STEP_PIN)-e0pulses);
    ASSERTEQUAL(6, arduino.pulses(PC2_Z_STEP_PIN)-zpulses);
    ASSERTEQUAL(3, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
    ASSERTEQUAL(1, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    ASSERTQUAD(Quad<StepCoord>(99, 97, 94, 100), mt.machine.getMotorPosition());

    arduino.setPin(PC2_PROBE_PIN, LOW);
    test_ticks(1);	// tripped
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERT(!machine.op.probe.probing);
    ASSERTEQUAL(0, arduino.pulses(PC2_E0_STEP_PIN)-e0pulses);
    ASSERTEQUAL(6, arduino.pulses(PC2_Z_STEP_PIN)-zpulses);
    ASSERTEQUAL(3, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
    ASSERTEQUAL(1, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    ASSERTQUAD(Quad<StepCoord>(99, 97, 94, 100), mt.machine.getMotorPosition());
    ASSERTEQUALS(JT("{'s':0,'r':{'prb':{'1':99,'2':97,'3':94,'pn':2,'ip':true}},'t':0.001}\n"),
                 Serial.output().c_str());
    test_ticks(1);
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

    // Test non-contact
    xpulses = arduino.pulses(PC2_X_STEP_PIN);
    ypulses = arduino.pulses(PC2_Y_STEP_PIN);
    zpulses = arduino.pulses(PC2_Z_STEP_PIN);
    e0pulses = arduino.pulses(PC2_E0_STEP_PIN);
    machine.setMotorPosition(Quad<StepCoord>(100, 100, 100, 100));
    arduino.setPin(PC2_PROBE_PIN, HIGH);

    Serial.push(JT("{'prb':{'1':99,'2':99,'3':99,'pn':2,'ip':true}}\n"));
    test_ticks(1);	// parse
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    ASSERTQUAD(Quad<StepCoord>(100, 100, 100, 100), mt.machine.getMotorPosition());

    test_ticks(1);	// initialize
    ASSERTEQUAL(STATUS_BUSY_CALIBRATING, mt.status);
    ASSERT(machine.op.probe.probing);
    ASSERTQUAD(Quad<StepCoord>(100, 100, 100, 100), mt.machine.getMotorPosition());

    test_ticks(1);	// calibrating
    ASSERTEQUAL(STATUS_BUSY_CALIBRATING, mt.status);
    ASSERT(machine.op.probe.probing);
    ASSERTEQUAL(0, arduino.pulses(PC2_E0_STEP_PIN)-e0pulses);
    ASSERTEQUAL(1, arduino.pulses(PC2_Z_STEP_PIN)-zpulses);
    ASSERTEQUAL(1, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
    ASSERTEQUAL(1, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    ASSERTQUAD(Quad<StepCoord>(99, 99, 99, 100), mt.machine.getMotorPosition());

    test_ticks(1);	// calibrating
    ASSERTEQUAL(STATUS_PROBE_FAILED, mt.status);
    ASSERT(machine.op.probe.probing);
    ASSERTEQUAL(0, arduino.pulses(PC2_E0_STEP_PIN)-e0pulses);
    ASSERTEQUAL(1, arduino.pulses(PC2_Z_STEP_PIN)-zpulses);
    ASSERTEQUAL(1, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
    ASSERTEQUAL(1, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    ASSERTQUAD(Quad<StepCoord>(99, 99, 99, 100), mt.machine.getMotorPosition());

    cout << "TEST	: test_probe() OK " << endl;
}

void test_MTO_RAW_hom() {
    cout << "TEST	: test_MTO_RAW_hom() =====" << endl;

    MachineThread mt = test_setup();
    Machine &machine = mt.machine;
    int32_t xpulses = arduino.pulses(PC2_X_STEP_PIN);
    int32_t ypulses = arduino.pulses(PC2_Y_STEP_PIN);
    int32_t zpulses = arduino.pulses(PC2_Z_STEP_PIN);
    int32_t e0pulses = arduino.pulses(PC2_E0_STEP_PIN);
    int16_t hPulses = machine.fastSearchPulses;
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
    Serial.push(JT("{'hom':{'x':'','z':16}}\n"));
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
    ASSERTQUAD(Quad<StepCoord>(5, 100, 16, 100), mt.machine.getMotorPosition());
    ASSERTEQUALS("", Serial.output().c_str());
    threadClock.ticks++;
    mt.loop(); // moving
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    ASSERTEQUALS("", Serial.output().c_str());
    ASSERT(machine.motorAxis[0]->homing);
    ASSERT(!machine.motorAxis[1]->homing);
    ASSERT(machine.motorAxis[2]->homing);
    ASSERT(!machine.motorAxis[3]->homing);
    ASSERTEQUAL(xpulses + hPulses * 1, arduino.pulses(PC2_X_STEP_PIN));
    ASSERTEQUAL(ypulses, arduino.pulses(PC2_Y_STEP_PIN));
    ASSERTEQUAL(zpulses + hPulses * 1, arduino.pulses(PC2_Z_STEP_PIN));
    ASSERTEQUALS("", Serial.output().c_str());
    threadClock.ticks++;
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    ASSERT(machine.motorAxis[0]->homing);
    ASSERT(!machine.motorAxis[1]->homing);
    ASSERT(machine.motorAxis[2]->homing);
    ASSERT(!machine.motorAxis[3]->homing);
    ASSERTEQUAL(xpulses + hPulses * 2, arduino.pulses(PC2_X_STEP_PIN));
    ASSERTEQUAL(ypulses, arduino.pulses(PC2_Y_STEP_PIN));
    ASSERTEQUAL(zpulses + hPulses * 2, arduino.pulses(PC2_Z_STEP_PIN));
    ASSERTEQUAL(true, machine.axis[0].homing);
    ASSERTEQUAL(false, machine.axis[0].atMin);
    ASSERTEQUAL(false, machine.axis[1].homing);
    ASSERTEQUAL(false, machine.axis[1].atMin);
    ASSERTEQUAL(true, machine.axis[2].homing);
    ASSERTEQUAL(LOW, arduino.getPin(PC2_X_DIR_PIN));
    ASSERTEQUAL(HIGH, arduino.getPin(PC2_Y_DIR_PIN));
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
    ASSERTEQUAL(hPulses * 2, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    ASSERTEQUAL(0, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
    ASSERTEQUAL(hPulses * 3, arduino.pulses(PC2_Z_STEP_PIN)-zpulses);
    ASSERTEQUAL(true, machine.axis[0].homing);
    ASSERTEQUAL(true, machine.axis[0].atMin);
    ASSERTEQUAL(false, machine.axis[1].homing);
    ASSERTEQUAL(false, machine.axis[1].atMin);
    ASSERTEQUAL(true, machine.axis[2].homing);
    ASSERTEQUAL(LOW, arduino.getPin(PC2_X_DIR_PIN));
    ASSERTEQUAL(HIGH, arduino.getPin(PC2_Y_DIR_PIN));
    ASSERTEQUAL(LOW, arduino.getPin(PC2_Z_DIR_PIN));
    ASSERTEQUALS("", Serial.output().c_str());
    ASSERT(machine.axis[0].atMin);
    ASSERT(!machine.axis[1].atMin);
    ASSERT(!machine.axis[2].atMin);
    ASSERT(!machine.axis[3].atMin);
    arduino.setPin(PC2_Z_MIN_PIN, HIGH);
    threadClock.ticks++;
    mt.loop(); // hit final limit switch
    ASSERTEQUAL(STATUS_BUSY_CALIBRATING, mt.status);
    ASSERTEQUAL(hPulses * 2 + LATCH_BACKOFF, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    ASSERTEQUAL(0, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
    ASSERTEQUAL(hPulses * 3 + LATCH_BACKOFF, arduino.pulses(PC2_Z_STEP_PIN)-zpulses);
    ASSERTEQUAL(0, arduino.pulses(PC2_E0_STEP_PIN)-e0pulses);
    threadClock.ticks++;
    mt.loop(); // calibrating
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUAL(hPulses * 2 + 2*LATCH_BACKOFF, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    ASSERTEQUAL(0, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
    ASSERTEQUAL(hPulses * 3 + 2*LATCH_BACKOFF, arduino.pulses(PC2_Z_STEP_PIN)-zpulses);
    ASSERTEQUAL(0, arduino.pulses(PC2_E0_STEP_PIN)-e0pulses);
    ASSERTQUAD(Quad<StepCoord>(5, 100, 16, 100), mt.machine.getMotorPosition());
    ASSERTEQUAL(false, machine.axis[0].homing);
    ASSERTEQUAL(false, machine.axis[1].homing);
    ASSERTEQUAL(false, machine.axis[2].homing);
    ASSERTEQUAL(HIGH, arduino.getPin(PC2_X_DIR_PIN)); // HIGH because we backed off
    ASSERTEQUAL(HIGH, arduino.getPin(PC2_Y_DIR_PIN));
    ASSERTEQUAL(HIGH, arduino.getPin(PC2_Z_DIR_PIN)); // HIGH because we backed off
    ASSERTEQUALS(JT("{'s':0,'r':{'hom':{'x':5,'z':16}},'t':0.000}\n"), Serial.output().c_str());
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
    zpulses = arduino.pulses(PC2_Z_STEP_PIN);
    threadClock.ticks++;
    mt.loop(); // ready for next command
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);
    ASSERTEQUAL(NOPIN, machine.axis[3].pinMin);
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
    ASSERTEQUAL(STATUS_BUSY_CALIBRATING, mt.status);
    ASSERTEQUAL(hPulses * 1 + LATCH_BACKOFF, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
    threadClock.ticks++;
    mt.loop(); // calibrating
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUAL(hPulses * 1 + 2*LATCH_BACKOFF, arduino.pulses(PC2_Y_STEP_PIN)-ypulses);
    ASSERTQUAD(Quad<StepCoord>(100, 10, 100, 100), mt.machine.getMotorPosition());
    ASSERTEQUALS(JT("{'s':0,'r':{'homy':10},'t':0.000}\n"), Serial.output().c_str());

    // TEST SHORT FORM
    xpulses = arduino.pulses(PC2_X_STEP_PIN);
    e0pulses = arduino.pulses(PC2_E0_STEP_PIN);
    machine.axis[0].enable(true);
    machine.axis[1].enable(false);
    machine.axis[2].enable(false);
    machine.axis[3].enable(true);
    machine.setMotorPosition(Quad<StepCoord>(100, 100, 100, 100));
    arduino.setPin(PC2_X_MIN_PIN, LOW);
    threadClock.ticks++;
    mt.loop(); // ready for next command
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);
    threadClock.ticks++;
    machine.axis[0].home = 5;
    machine.axis[1].home = 10;
    machine.axis[2].home = 15;
    machine.axis[3].home = 20;
    ASSERT(machine.axis[0].isEnabled());
    ASSERT(!machine.axis[1].isEnabled());
    ASSERT(!machine.axis[2].isEnabled());
    ASSERT(machine.axis[3].isEnabled());
    Serial.push(JT("{'hom':''}\n"));
    mt.loop();	// parse
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    threadClock.ticks++;
    mt.loop(); // initializing
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    ASSERTEQUAL(0, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    ASSERTEQUAL(0, arduino.pulses(PC2_E0_STEP_PIN)-e0pulses);
    ASSERTEQUALS("", Serial.output().c_str());
    threadClock.ticks++;
    mt.loop(); // moving
    ASSERTEQUALS("", Serial.output().c_str());
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    ASSERTEQUAL(0, arduino.pulses(PC2_E0_STEP_PIN)-e0pulses);
    arduino.setPin(PC2_X_MIN_PIN, HIGH);
    threadClock.ticks++;
    mt.loop(); // hit limit switch
    ASSERTEQUAL(STATUS_BUSY_CALIBRATING, mt.status);
    ASSERTEQUAL(hPulses * 1 + LATCH_BACKOFF, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    threadClock.ticks++;
    mt.loop(); // hit limit switch
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUAL(hPulses * 1 + 2*LATCH_BACKOFF, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    ASSERTEQUAL(0, arduino.pulses(PC2_E0_STEP_PIN)-e0pulses);
    ASSERTQUAD(Quad<StepCoord>(5, 100, 100, 20), mt.machine.getMotorPosition());
    ASSERTEQUALS(JT("{'s':0,'r':{'hom':{'1':5,'2':10,'3':15,'4':20}},'t':0.000}\n"),
                 Serial.output().c_str());
    mt.loop(); // hit limit switch
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

    cout << "TEST	: test_MTO_RAW_hom() OK " << endl;
}

void test_MachineThread() {
    cout << "TEST	: test_MachineThread() =====" << endl;

    threadRunner.clear();
    MachineThread mt;
    Machine& machine = mt.machine;
    mt.setup(PC2_RAMPS_1_4);
    ASSERTQUAD(Quad<StepCoord>(0, 0, 0, 0), mt.machine.getMotorPosition());

    Serial.clear();
    test_ticks(1);
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

    Serial.clear();
    Serial.push("{");
    TCNT1 = 100;
    test_ticks(1);
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
        "{'s':0,'r':{'systc':103,'xen':true,'yen':true,'zen':true,'aen':true},'t':0.000}\n";
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
        "{'s':0,'r':{'systc':113,'dvs':{'us':512,'dp':[100,200],'1':100,'2':200,'3':0}},'t':0.001}\n";
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

    testJSON(mt, "'\"",
             "{'dpycr':10,'dpycg':20,'dpycb':30,'dpyds':12,'dpydl':255}",
             "{'s':0,'r':{'dpycr':10,'dpycg':20,'dpycb':30,'dpyds':12,'dpydl':255},'t':0.000}\n");
    testJSON(mt, "'\"",
             "{'dpy':{'ds':30,'dl':255,'cr':1,'cg':2,'cb':3}}",
             "{'s':25,'r':{'dpy':{'ds':30,'dl':255,'cr':1,'cg':2,'cb':3}},'t':0.000}\n", STATUS_WAIT_BUSY);

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
    ASSERTEQUAL(2, machine.stroke.seg[1].value[0]);
    ASSERTEQUAL(4, machine.stroke.seg[2].value[0]);
    ASSERTEQUAL(9, machine.stroke.seg[3].value[0]);
    ASSERTEQUAL(13, machine.stroke.seg[4].value[0]);
    ASSERTEQUAL(17, machine.stroke.seg[5].value[0]);
    ASSERTEQUAL(21, machine.stroke.seg[6].value[0]); // peak velocity
    ASSERTEQUAL(22, machine.stroke.seg[7].value[0]);
    ASSERTEQUAL(24, machine.stroke.seg[8].value[0]);
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
        JT("{'s':-901,'r':{'tstph':{'pu':3200,'tv':0.700,'sg':16,'mv':12800,"\
           "'lp':26144,'pp':7611.1,'tp':0.837,'ts':0.837}},'t':3.347}\n"),
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
        JT("{'s':-901,'r':{'tstph':{'pu':6400,'tv':0.010,'sg':99,'mv':12800,"\
           "'lp':15936,'pp':12811.4,'tp':0.510,'ts':0.510}},'t':1.020}\n"),
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
        JT("{'s':-901,'r':{'tstph':{'pu':6400,'tv':0.100,'sg':32,'mv':40000,"\
           "'lp':8124,'pp':40118.1,'tp':0.260,'ts':0.260}},'t':0.520}\n"),
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
        JT("{'s':-901,'r':{'tstph':{'pu':1,'tv':0.100,'sg':16,'mv':40000,"\
           "'lp':98,'pp':0.0,'tp':0.003,'ts':0.003}},'t':0.007}\n"),
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
        JT("{'s':-901,'r':{'tstph':{'pu':32000,'tv':0.150,'sg':99,'mv':16000,"\
           "'lp':67186,'pp':16024.1,'tp':2.150,'ts':2.150}},'t':4.300}\n"),
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
        JT("{'s':-901,'r':{'tstph':{'pu':32000,'tv':0.200,'sg':99,'mv':4000,"\
           "'lp':256250,'pp':4008.3,'tp':8.200,'ts':8.200}},'t':16.400}\n"),
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
        JT("{'s':-901,'r':{'tstph':{'pu':1600,'tv':0.300,'mv':16000,'lp':10824,"\
           "'pp':9237.0,'sg':16,'tp':0.346,'ts':0.346}},'t':0.693}\n"),
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
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    test_ticks(1); // process second command
    ASSERTEQUALS(JT(""), Serial.output().c_str());
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    test_ticks(1);
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUALS(JT("{'s':0,'r':{'ypo':2},'t':0.000}\n"), Serial.output().c_str());
    test_ticks(1); // done
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

    // TEST serial interrupt of command array
    Serial.push(JT("[{'xpo':''},{'ypo':''},{'zpo':''}]\n"));
    test_ticks(1); // parse JsonCommand
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    test_ticks(1); // process first command
    ASSERTEQUALS(JT(""), Serial.output().c_str());
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    Serial.push("\n");
    test_ticks(1);
    ASSERTEQUAL(STATUS_WAIT_CANCELLED, mt.status);
    ASSERTEQUALS(JT("{'s':-901,'r':{'xpo':1},'t':0.000}\n"), Serial.output().c_str());
    test_ticks(1);
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

    // TEST two command array
    ASSERT(machine.axis[0].isEnabled());
    ASSERT(machine.axis[1].isEnabled());
    ASSERT(machine.axis[2].isEnabled());
    ASSERT(machine.axis[3].isEnabled());
    machine.axis[0].home = 5;
    machine.axis[1].home = 10;
    machine.axis[2].home = 15;
    machine.axis[3].home = 20;
    int32_t xpulses = arduino.pulses(PC2_X_STEP_PIN);
    Serial.push(JT("[{'syspc':2},{'hom':''}]\n"));
    test_ticks(1); // parse JsonCommand
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    test_ticks(1); // process first command
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    test_ticks(1); // process second command
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    ASSERTEQUAL(0, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    test_ticks(1); // process second command
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    ASSERTEQUAL(3, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    test_ticks(1); // process second command
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    ASSERTEQUAL(6, arduino.pulses(PC2_X_STEP_PIN)-xpulses);
    test_ticks(1); // process second command
    ASSERTEQUAL(STATUS_BUSY_MOVING, mt.status);
    arduino.setPin(machine.axis[0].pinMin, 1);
    arduino.setPin(machine.axis[1].pinMin, 1);
    arduino.setPin(machine.axis[2].pinMin, 1);
    test_ticks(1);
    ASSERTEQUAL(STATUS_BUSY_CALIBRATING, mt.status);
    test_ticks(1);
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    test_ticks(1); // done
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUALS(JT("{'s':0,'r':{'hom':{'1':5,'2':10,'3':15,'4':20}},'t':0.001}\n"), Serial.output().c_str());
    test_ticks(1); // done
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

    cout << "TEST	: test_command_arraytest_pnp() OK " << endl;
}

void test_DeltaCalculator() {
    cout << "TEST	: test_DeltaCalculator() =====" << endl;

    DeltaCalculator dc;
    dc.useEffectorOrigin();

    PH5TYPE ha1 = dc.getHomeAngle();
    dc.setHomePulses(dc.getHomePulses());
    PH5TYPE ha2 = dc.getHomeAngle();
    ASSERTEQUALT(ha1, ha2, 0.00001);
    StepCoord homePulses = dc.getHomePulses();
    ASSERTEQUALT(131.636, dc.getEffectorTriangleSide(), 0.0001);
    ASSERTEQUALT(190.526, dc.getBaseTriangleSide(), 0.0001);
    ASSERTEQUALT(270, dc.getEffectorLength(), 0.000001);
    ASSERTEQUALT(90, dc.getBaseArmLength(), 0.000001);
    ASSERTEQUALT(200, dc.getSteps360(), 0.000001);
    ASSERTEQUALT(16, dc.getMicrosteps(), 0.000001);
    ASSERTEQUALT(FPD_GEAR_RATIO, dc.getGearRatio(), 0.000001);
    ASSERTEQUALT(FPD_GEAR_RATIO, 360/(16*200*dc.getDegreesPerPulse()), 0.000001);
    ASSERTEQUALT(-111.571, dc.getMinZ(), 0.001);
    ASSERTEQUALT(-69.571, dc.getMinZ(100,100), 0.001);
    ASSERTEQUALT(-67.2, dc.getMinDegrees(), 0.001);
    ASSERTEQUAL(-5659, homePulses);
    ASSERTEQUAL(-5659, homePulses);
    ASSERTEQUAL(-5659, homePulses);
	Step3D pulses65 = dc.calcPulses(XYZ3D(0,0,65));
	ASSERTEQUALT(-5444, pulses65.p1, 0.001);
	ASSERTEQUALT(-5444, pulses65.p2, 0.001);
	ASSERTEQUALT(-5444, pulses65.p3, 0.001);
    PH5TYPE homeAngle = dc.getHomeAngle();
    ASSERTEQUALT(-67.2, homeAngle, 0.001);
    XYZ3D xyz = dc.calcXYZ(Angle3D());
    ASSERT(xyz.isValid());
    ASSERTEQUALT(0, xyz.x, 0.00001);
    ASSERTEQUALT(0, xyz.y, 0.00001);
    ASSERTEQUALT(0, xyz.z, 0.00001);
    xyz = dc.calcXYZ(Angle3D(1,1,1));
    ASSERT(xyz.isValid());
    ASSERTEQUAL(0, xyz.x);
    ASSERTEQUAL(0, xyz.y);
    ASSERTEQUALT(-1.57663, xyz.z, 0.00001);
    Angle3D angles = dc.calcAngles(xyz);
    ASSERTEQUALT(1, angles.theta1, 0.00001);
    ASSERTEQUALT(1, angles.theta2, 0.00001);
    ASSERTEQUALT(1, angles.theta3, 0.00001);
    Step3D pulses = dc.calcPulses(XYZ3D(1,2,3));
    ASSERTEQUAL(-115, pulses.p1);
    ASSERTEQUAL(-205, pulses.p2);
    ASSERTEQUAL(-165, pulses.p3);
    xyz = dc.calcXYZ(pulses);
    ASSERTEQUALT(0.983, xyz.x, 0.01);
    ASSERTEQUALT(1.98762, xyz.y, 0.01);
    ASSERTEQUALT(3, xyz.z, 0.01);
    PH5TYPE dz = dc.getZOffset();
    ASSERTEQUALT(247.893, dz, 0.001);
    Step3D pulses4(
        pulses.p1+4,
        pulses.p2+4,
        pulses.p3+4
    );
    XYZ3D xyz4 = dc.calcXYZ(pulses4);
    ASSERTEQUALT(0.0734863, xyz.z - xyz4.z, 0.00001);

    // ZBowlError
    PH5TYPE e = 0.001;
    PH5TYPE zCenter = -60.5;
    PH5TYPE radius = 50;
    ASSERTEQUALT(0.909290921, dc.calcZBowlErrorFromETheta(zCenter, radius, -15), e);
    ASSERTEQUALT(0.599884, dc.calcZBowlErrorFromETheta(zCenter, radius, -10), e);
    ASSERTEQUALT(0.425232, dc.calcZBowlErrorFromETheta(zCenter, radius, -360 * 0.02), e);
    ASSERTEQUALT(0.289734, dc.calcZBowlErrorFromETheta(zCenter, radius, -5), e);
    ASSERTEQUALT(0.204468, dc.calcZBowlErrorFromETheta(zCenter, radius, -360 * 0.01), e);
    ASSERTEQUALT(-0.00879, dc.calcZBowlErrorFromETheta(zCenter, radius, 0), e);
    ASSERTEQUALT(-0.211, dc.calcZBowlErrorFromETheta(zCenter, radius, 360 * 0.01), e);
    ASSERTEQUALT(-0.286, dc.calcZBowlErrorFromETheta(zCenter, radius, 5), e);
    ASSERTEQUALT(-0.399, dc.calcZBowlErrorFromETheta(zCenter, radius, 360 * 0.02), e);
    ASSERTEQUALT(-0.535, dc.calcZBowlErrorFromETheta(zCenter, radius, 10), e);
    ASSERTEQUALT(-0.750763, dc.calcZBowlErrorFromETheta(zCenter, radius, 15), e);

    PH5TYPE gearRatio = FPD_GEAR_RATIO;
	e = 0.01;
    ASSERTEQUALT(0.13, dc.calcZBowlErrorFromGearRatio(zCenter, radius, gearRatio * 0.98), e);
    ASSERTEQUALT(0.06, dc.calcZBowlErrorFromGearRatio(zCenter, radius, gearRatio * 0.99), e);
    ASSERTEQUALT(-0.01, dc.calcZBowlErrorFromGearRatio(zCenter, radius, gearRatio), e);
    ASSERTEQUALT(-0.07, dc.calcZBowlErrorFromGearRatio(zCenter, radius, gearRatio * 1.01), e);
    ASSERTEQUALT(-0.13, dc.calcZBowlErrorFromGearRatio(zCenter, radius, gearRatio * 1.02), e);

	// base triangle side error
    ASSERTEQUALT(0.10, dc.calcZBowlErrorFromE_f(zCenter, radius, 0.98), e);
    ASSERTEQUALT(0.05, dc.calcZBowlErrorFromE_f(zCenter, radius, 0.99), e);
    ASSERTEQUALT(-0.01, dc.calcZBowlErrorFromE_f(zCenter, radius, 1.0), e);
    ASSERTEQUALT(-0.06, dc.calcZBowlErrorFromE_f(zCenter, radius, 1.01), e);
    ASSERTEQUALT(-0.11, dc.calcZBowlErrorFromE_f(zCenter, radius, 1.02), e);

	// effector arm length error
    ASSERTEQUALT(-0.09, dc.calcZBowlErrorFromE_re(zCenter, radius, 0.98), e);
    ASSERTEQUALT(-0.05, dc.calcZBowlErrorFromE_re(zCenter, radius, 0.99), e);
    ASSERTEQUALT(-0.01, dc.calcZBowlErrorFromE_re(zCenter, radius, 1.0), e);
    ASSERTEQUALT(0.04, dc.calcZBowlErrorFromE_re(zCenter, radius, 1.01), e);
    ASSERTEQUALT(0.08, dc.calcZBowlErrorFromE_re(zCenter, radius, 1.02), e);

	// effector triangle side error
    ASSERTEQUALT(-0.08, dc.calcZBowlErrorFromE_e(zCenter, radius, 0.98), e);
    ASSERTEQUALT(-0.04, dc.calcZBowlErrorFromE_e(zCenter, radius, 0.99), e);
    ASSERTEQUALT(-0.01, dc.calcZBowlErrorFromE_e(zCenter, radius, 1.0), e);
    ASSERTEQUALT(0.03, dc.calcZBowlErrorFromE_e(zCenter, radius, 1.01), e);
    ASSERTEQUALT(0.07, dc.calcZBowlErrorFromE_e(zCenter, radius, 1.02), e);

	// base arm length error
    ASSERTEQUALT(0.05, dc.calcZBowlErrorFromE_rf(zCenter, radius, 0.98), e);
    ASSERTEQUALT(0.02, dc.calcZBowlErrorFromE_rf(zCenter, radius, 0.99), e);
    ASSERTEQUALT(-0.01, dc.calcZBowlErrorFromE_rf(zCenter, radius, 1.0), e);
    ASSERTEQUALT(-0.03, dc.calcZBowlErrorFromE_rf(zCenter, radius, 1.01), e);
    ASSERTEQUALT(-0.06, dc.calcZBowlErrorFromE_rf(zCenter, radius, 1.02), e);

	{	TESTCOUT1("TEST	: ", "ZBowl home angle (initial calibration)");
        PH5TYPE zRim = zCenter - 0.5; // 0.5mm bowl error
        DeltaCalculator dc1(dc);
        PH5TYPE eTheta1 = dc1.calcZBowlETheta(zCenter, zRim, radius);
        ASSERTEQUALT(-67.2, dc1.getHomeAngle(),0.001); // default
        dc1.setHomeAngle(dc1.getHomeAngle()+eTheta1);
        TESTCOUT2("Homing angle:", dc1.getHomeAngle(), " error:", eTheta1);
        ASSERTEQUALT(-57.94, dc1.getHomeAngle(), e); // corrected
        ASSERTEQUALT(9.26, eTheta1, e);
        ASSERTEQUAL(-4880, dc1.getHomePulses());
        // subsequent calibration with no error
        PH5TYPE eTheta2 = dc1.calcZBowlETheta(zCenter, zCenter, radius);
        ASSERTEQUALT(0, eTheta2, 0.00001);
        // subsequent calibration with almost undetectable error
		PH5TYPE zErrTiny = 0.01; // 10 microns
        PH5TYPE eTheta3 = dc1.calcZBowlETheta(zCenter, zCenter+zErrTiny, radius);
        ASSERTEQUALT(-0.322, eTheta3, 0.001);
        dc1.setHomeAngle(dc1.getHomeAngle()+eTheta3);
        ASSERTEQUALT(-58.2658, dc1.getHomeAngle(),0.001); // corrected
        ASSERTEQUAL(-4907, dc1.getHomePulses());
    }

	{	TESTCOUT1("TEST	: ", "ZBowl gear ratio (initial calibration)");
        PH5TYPE zRim = zCenter - 0.1; // 0.1mm bowl error
        PH5TYPE zError = zRim - zCenter;
        ASSERTEQUALT(-0.1, zError, 0.001);
        DeltaCalculator dc2(dc);
        ASSERTEQUALT(FPD_GEAR_RATIO, dc2.getGearRatio(), 0.00001);
        ASSERTEQUALT(-0.074, dc.calcZBowlErrorFromGearRatio(zCenter, radius, gearRatio + 0.1), e);
        PH5TYPE gearRatio1 = dc2.calcZBowlGearRatio(zCenter, zRim, radius);
        ASSERTEQUALT(9.607, gearRatio1, 0.001);
        PH5TYPE eGear = gearRatio1 - dc2.getGearRatio();
        ASSERTEQUALT(0.134, eGear, 0.001);
        TESTCOUT2("gearRatio1:", gearRatio1, " eGear:", eGear);
        dc2.setGearRatio(gearRatio1);
        ASSERTEQUALT(9.608, dc2.getGearRatio(), 0.001);
        // subsequent calibration with no error
        PH5TYPE gearRatio2 = dc2.calcZBowlGearRatio(zCenter, zCenter, radius);
        ASSERTEQUALT(9.608, gearRatio2, 0.001);
        // subsequent calibration with almost undetectable error
		PH5TYPE zErrTiny = 0.01; 
        PH5TYPE gearRatio3 = dc2.calcZBowlGearRatio(zCenter, zCenter+zErrTiny, radius);
        ASSERTEQUALT(9.602, gearRatio3, 0.001);
    }

    {   // verify internal constraints
        DeltaCalculator dc3;
        ASSERTEQUALT(FPD_GEAR_RATIO, dc3.getGearRatio(), 0.00001);
        ASSERTEQUALT(-67.2, dc3.getHomeAngle(), 0.001);
        ASSERTEQUALT(84.2122, 1/dc3.getDegreesPerPulse(), 0.001);
        ASSERTEQUAL(-5659, dc3.getHomePulses());
        ASSERTEQUAL(0, dc3.getZOffset());

        dc3.useEffectorOrigin();
        ASSERTEQUALT(FPD_GEAR_RATIO, dc3.getGearRatio(), 0.00001);
        ASSERTEQUALT(-67.2, dc3.getHomeAngle(), 0.001);
        //ASSERTEQUALT(0.0118748, dc3.getDegreesPerPulse(), 0.00001);
        ASSERTEQUALT(84.2122, 1/dc3.getDegreesPerPulse(), 0.001);
        ASSERTEQUAL(-5659, dc3.getHomePulses());
        ASSERTEQUALT(247.893, dc3.getZOffset(), 0.001); 	// CHANGED

        dc3.setGearRatio(9.5);
        ASSERTEQUALT(9.5, dc3.getGearRatio(), 0.001); 		// CHANGED
        ASSERTEQUALT(-67.0145, dc3.getHomeAngle(), 0.001); 	// CHANGED
        //ASSERTEQUALT(0.0118421, dc3.getDegreesPerPulse(), 0.00001);
        ASSERTEQUALT(84.444, 1/dc3.getDegreesPerPulse(), 0.001); 	// CHANGED
        ASSERTEQUAL(-5659, dc3.getHomePulses());
        ASSERTEQUALT(247.893, dc3.getZOffset(), 0.001);

        dc3.setGearRatio(FPD_GEAR_RATIO);
        ASSERTEQUALT(FPD_GEAR_RATIO, dc3.getGearRatio(), 0.00001);
        ASSERTEQUALT(-67.2, dc3.getHomeAngle(), 0.001); 	// CHANGED
        ASSERTEQUALT(84.2122, 1/dc3.getDegreesPerPulse(), 0.001);
        ASSERTEQUAL(-5659, dc3.getHomePulses());
        ASSERTEQUALT(247.893, dc3.getZOffset(), 0.001);

        dc3.setHomeAngle(-66);
        ASSERTEQUALT(FPD_GEAR_RATIO, dc3.getGearRatio(), 0.00001);
        ASSERTEQUALT(-66, dc3.getHomeAngle(), 0.001); 		// CHANGED
        ASSERTEQUALT(84.2122, 1/dc3.getDegreesPerPulse(), 0.001);
        ASSERTEQUAL(-5558, dc3.getHomePulses()); 			// CHANGED
        ASSERTEQUALT(247.893, dc3.getZOffset(), 0.001);

        dc3.setMicrosteps(32);
        ASSERTEQUALT(FPD_GEAR_RATIO, dc3.getGearRatio(), 0.00001);
        ASSERTEQUALT(-66, dc3.getHomeAngle(), 0.001); 		
        ASSERTEQUALT(168.424, 1/dc3.getDegreesPerPulse(), 0.001);	// CHANGED
        ASSERTEQUAL(-11116, dc3.getHomePulses()); 			// CHANGED
        ASSERTEQUALT(247.893, dc3.getZOffset(), 0.001);

        dc3.setSteps360(100);
        ASSERTEQUALT(FPD_GEAR_RATIO, dc3.getGearRatio(), 0.00001);
        ASSERTEQUALT(-66, dc3.getHomeAngle(), 0.001); 		
        ASSERTEQUALT(84.2122, 1/dc3.getDegreesPerPulse(), 0.001);	// CHANGED
        ASSERTEQUAL(-5558, dc3.getHomePulses()); 			// CHANGED
        ASSERTEQUALT(247.893, dc3.getZOffset(), 0.001);
    }

	// SPE off
	ASSERTEQUALT(0, dc.getSPERatio(), 0.001);
	// verify SPE calculation round trip
	ASSERTEQUAL(-4175, dc.calcSPEPulses(dc.getSPEAngle()+2));
	ASSERTEQUALT(2.004, dc.calcSPEAngle(-4175)-dc.getSPEAngle(), 0.001);
	ASSERTEQUAL(-4260, dc.calcSPEPulses(dc.getSPEAngle()+1));
	ASSERTEQUALT(0.995, dc.calcSPEAngle(-4260)-dc.getSPEAngle(), 0.001);
	ASSERTEQUAL(-4344, dc.calcSPEPulses(dc.getSPEAngle()+0));
	ASSERTEQUALT(-0.003, dc.calcSPEAngle(-4344)-dc.getSPEAngle(), 0.001);
	ASSERTEQUAL(-4428, dc.calcSPEPulses(dc.getSPEAngle()-1));
	ASSERTEQUALT(-1.00048, dc.calcSPEAngle(-4428)-dc.getSPEAngle(), 0.001);
	ASSERTEQUAL(-4512, dc.calcSPEPulses(dc.getSPEAngle()-2));
	ASSERTEQUALT(-1.998, dc.calcSPEAngle(-4512)-dc.getSPEAngle(), 0.001);
	// SPE on
	dc.setSPERatio(FPD_SPE_RATIO);
	ASSERTEQUALT(FPD_SPE_ANGLE, dc.getSPEAngle(), 0.001);
	ASSERTEQUALT(FPD_SPE_RATIO, dc.getSPERatio(), 0.001);
	// verify SPE calculation round trip
	ASSERTEQUAL(-4175, dc.calcSPEPulses(dc.getSPEAngle()+2));
	ASSERTEQUALT(2.004, dc.calcSPEAngle(-4175)-dc.getSPEAngle(), 0.001);
	ASSERTEQUAL(-4260, dc.calcSPEPulses(dc.getSPEAngle()+1));
	ASSERTEQUALT(0.995, dc.calcSPEAngle(-4260)-dc.getSPEAngle(), 0.001);
	ASSERTEQUAL(-4344, dc.calcSPEPulses(dc.getSPEAngle()+0));
	ASSERTEQUALT(-0.003, dc.calcSPEAngle(-4344)-dc.getSPEAngle(), 0.001);
	ASSERTEQUAL(-4411, dc.calcSPEPulses(dc.getSPEAngle()-1)); // SPE change
	ASSERTEQUALT(-0.955, dc.calcSPEAngle(-4411)-dc.getSPEAngle(), 0.001); // SPE change
	ASSERTEQUAL(-4479, dc.calcSPEPulses(dc.getSPEAngle()-2)); // SPE change
	ASSERTEQUALT(-1.921, dc.calcSPEAngle(-4479)-dc.getSPEAngle(), 0.001); // SPE change

    cout << "TEST	: test_DeltaCalculator() OK " << endl;
}

void test_axis() {
    cout << "TEST	: test_axis() =====" << endl;

    MachineThread mt = test_MTO_FPD_setup();
    Machine &machine(mt.machine);

    Serial.push(JT("[{'x':''}]\n"));
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop();
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUAL(LOW, arduino.getPin(PC2_X_ENABLE_PIN));
    ASSERTEQUALS(JT("{'s':0,'r':{'x':"
                    "{'dh':true,'en':true,'ho':-5659,'is':0,'lb':200,'lm':false,'ln':false,'mi':16,"
                    "'pd':55,'pe':38,'pm':255,'pn':3,'po':0,'ps':54,'sa':1.800,'tm':32000,'tn':-32000,'ud':0}"
                    "},'t':0.000}\n"), Serial.output().c_str());
    mt.loop();
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

	// latchBackoff
	ASSERTEQUAL(200, machine.axis[0].latchBackoff);
	ASSERTEQUAL(200, machine.axis[1].latchBackoff);
	ASSERTEQUAL(200, machine.axis[2].latchBackoff);
    Serial.push(JT("[{'xlb':201,'y':{'lb':202},'zlb':203}]\n"));
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop();
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUAL(LOW, arduino.getPin(PC2_X_ENABLE_PIN));
    ASSERTEQUALS(JT("{'s':0,'r':{'xlb':201,'y':{'lb':202},'zlb':203}"
                    ",'t':0.000}\n"), Serial.output().c_str());
	ASSERTEQUAL(201, machine.axis[0].latchBackoff);
	ASSERTEQUAL(202, machine.axis[1].latchBackoff);
	ASSERTEQUAL(203, machine.axis[2].latchBackoff);
    mt.loop();
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

    cout << "TEST	: test_axis() OK " << endl;
}

void test_msg_cmt_idl() {
    cout << "TEST	: test_msg_cmt_idl() =====" << endl;

    arduino.clear();
    threadRunner.clear();
    MachineThread mt;
    Machine &machine(mt.machine);
    machine.pDisplay = &testDisplay;
    mt.setup(PC1_EMC02);

    ASSERTEQUAL(STATUS_BUSY_SETUP, mt.status);
    mt.loop();
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);
    Serial.push(JT("[{'msg':'quack'},{'cmt':'hello'},{'msg':'duck'}]\n"));
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop(); // quack
    ASSERTEQUALS(JT("quack\n"), Serial.output().c_str());
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop(); // hello
    ASSERTEQUALS(JT(""), Serial.output().c_str());
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop(); // duck
    ASSERTEQUALS(JT("duck\n"), Serial.output().c_str());
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop();
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUALS(JT("{'s':0,'r':{'msg':'duck'},'t':0.000}\n"), Serial.output().c_str());
    mt.loop();
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

    machine.outputMode = OUTPUT_CMT;
    Serial.push(JT("[{'msg':'quack'},{'cmt':'hello'},{'msg':'duck'}]\n"));
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop(); // quack
    ASSERTEQUALS(JT("quack\n"), Serial.output().c_str());
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop(); // hello
    ASSERTEQUALS(JT("hello\n"), Serial.output().c_str());
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop(); // duck
    ASSERTEQUALS(JT("duck\n"), Serial.output().c_str());
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop();
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUALS(JT("{'s':0,'r':{'msg':'duck'},'t':0.000}\n"), Serial.output().c_str());
    mt.loop();
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

    Serial.push(JT("{'idl':123}\n"));
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop();
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUALS(JT("{'s':0,'r':{'idl':123},'t':0.123}\n"), Serial.output().c_str());
    mt.loop();
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

    cout << "TEST	: test_msg_cmt_idl() OK " << endl;
}

void test_pgm_parse(const char *pgm) {
	TESTCOUT1("test_pgm_parse:", pgm);
    JsonCommand jc;
    const char *s;
    ASSERTEQUALS("", Serial.output().c_str());
    ASSERTEQUAL(STATUS_OK, prog_dump(pgm));
    s = Serial.output().c_str();
    ASSERTEQUAL(true, (strncmp("[{\"msg\":", s, 8) != 0));
    ASSERTEQUAL(STATUS_BUSY_PARSED, jc.parse(s));
}

void test_cal_arm() {
    cout << "TEST	: test_cal_arm() =====" << endl;

    MachineThread mt = test_MTO_FPD_setup();
	Machine& machine = mt.machine;
	DeltaCalculator& dc = machine.delta;
	StepCoord armPos = 7800;

    // calgr1: calibrate gear ratio for arm 1
	machine.axis[0].position = armPos;
    Serial.push(JT("{'calgr1':90}\n"));
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop();
    ASSERTEQUAL(STATUS_OK, mt.status);
	ASSERTEQUALT(90.0/armPos, dc.getDegreesPerPulse(DELTA_AXIS_1), 0.001);
	ASSERTEQUALT(9.750, dc.getGearRatio(DELTA_AXIS_1), 0.001);
    mt.loop();
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

    // calgr2: calibrate gear ratio for arm 2
	machine.axis[1].position = armPos+10;
    Serial.push(JT("{'cal':{'gr2':90}}\n"));
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop();
    ASSERTEQUAL(STATUS_OK, mt.status);
	ASSERTEQUALT(90.0/(armPos+10), dc.getDegreesPerPulse(DELTA_AXIS_2), 0.00001);
	ASSERTEQUALT(9.7625, dc.getGearRatio(DELTA_AXIS_2), 0.00001);
    mt.loop();
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

    // calgr3: calibrate gear ratio for arm 3
	machine.axis[2].position = armPos-10;
    Serial.push(JT("{'calgr3':90}\n"));
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop();
    ASSERTEQUAL(STATUS_OK, mt.status);
	ASSERTEQUALT(90.0/(armPos-10), dc.getDegreesPerPulse(DELTA_AXIS_3), 0.00001);
	ASSERTEQUALT(9.7375, dc.getGearRatio(DELTA_AXIS_3), 0.00001);
    mt.loop();
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

    cout << "TEST	: test_cal_arm() =====" << endl;
}

void test_pgm() {
    cout << "TEST	: test_pgm() =====" << endl;

    // ProgMem.cpp test
    ASSERTEQUALS(JT("[{'msg':'test A'},{'msg':'test B'}]"), prog_src("test"));

    // pgmd: dump program
    MachineThread mt = test_MTO_FPD_setup();
    Machine& machine = mt.machine;
    Serial.push(JT("{'pgmd':'test'}\n"));
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop();
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUALS(JT("[{'msg':'test A'},{'msg':'test B'}]\n"
                    "{'s':0,'r':{'pgmd':'test'}"
                    ",'t':0.000}\n"),
                 Serial.output().c_str());
    mt.loop();
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

    // pgmx: execute program
    Serial.push(JT("{'pgmx':'test2'}\n"));
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop(); // program
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop(); // program
    ASSERTEQUALS(JT("test A\n"), Serial.output().c_str());
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop(); // program
    ASSERTEQUALS(JT("test B\n"), Serial.output().c_str());
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop(); // program end
    ASSERTEQUAL(STATUS_OK, mt.status);
    ASSERTEQUALS(JT("{'s':0,'r':{'msg':'test B'}"
                    ",'t':0.000}\n"),
                 Serial.output().c_str());
    mt.loop();
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

    // check syntax of pgm sources
    //test_pgm_parse("cal");
    //test_pgm_parse("cal-coarse");
    //test_pgm_parse("cal-fine");
    test_pgm_parse("cal-fpd-bed-coarse");
    test_pgm_parse("cal-fpd-bed-medium");
    test_pgm_parse("cal-fpd-bed-fine");
    //test_pgm_parse("cal-gear");
    //test_pgm_parse("cal-gear-coarse");
    //test_pgm_parse("cal-gear-fine");
    test_pgm_parse("cal-fpd-home-coarse");
    test_pgm_parse("cal-fpd-home-medium");
    test_pgm_parse("cal-fpd-home-fine");
    test_pgm_parse("dim-fpd");
    test_pgm_parse("fpd-hex-probe");

    // pgmx: dim-fpd
    machine.delta.setGearRatio(9); // wrong value
    machine.delta.setBaseTriangleSide(180); // wrong value
    machine.delta.setEffectorTriangleSide(123); // wrong value
    machine.delta.setBaseArmLength(80); // wrong value
    machine.delta.setEffectorLength(230);
    machine.delta.setHomeAngle(66); // wrong value
    machine.homePulses = -1234; // wrong value
    Serial.push(JT("{'pgmx':'dim-fpd'}\n"));
    mt.loop();
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop(); // program
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop(); // program
    ASSERTEQUAL(STATUS_BUSY_PARSED, mt.status);
    mt.loop(); // program
    ASSERTEQUAL(STATUS_OK, mt.status);
    double pi = 3.14159265359;
    double GT2_pitch_length_offset = 0.75;
    double gearRatioFPD = FPD_GEAR_RATIO;
    printf("%.12lf", gearRatioFPD);
    ASSERTEQUALT(gearRatioFPD, machine.delta.getGearRatio(), 0.000001);
    ASSERTEQUALT(131.636, machine.delta.getEffectorTriangleSide(), 0.001);
    ASSERTEQUALT(190.526, machine.delta.getBaseTriangleSide(), 0.001);
    ASSERTEQUALT(270.000, machine.delta.getEffectorLength(), 0.001);
    ASSERTEQUALT(90.000, machine.delta.getBaseArmLength(), 0.001);
    ASSERTEQUAL(-1234, machine.homePulses); // still custom value
    ASSERTEQUALS(JT("{'s':0,'r':{'dim':"
                    "{'gr':9.474,'ha':-67.200,'e':131.636,'f':190.526,'re':270.000,'rf':90.000,'spa':"FPD_SPE_ANGLE_S",'spr':"FPD_SPE_RATIO_S",'st':200}}"
                    ",'t':0.000}\n"),
                 Serial.output().c_str());
    mt.loop();
    ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

    //// pmgx: cal-fpd-home-coarse
    //machine.outputMode = (OutputMode)1;
    //machine.setMotorPosition(Quad<StepCoord>());
    //Serial.push(JT("{'pgmx':'cal-fpd-home-coarse'}\n"));
    //do {
        //if (machine.axis[0].position > 2000 && 
			//machine.axis[1].position > 2000 && 
			//machine.axis[2].position > 2000) {
            //arduino.setPin(PC2_PROBE_PIN, LOW);
        //} else {
            //arduino.setPin(PC2_PROBE_PIN, HIGH);
        //}
		//arduino.setPin(PC2_X_MIN_PIN, machine.axis[0].position < -5600 ? LOW : HIGH);
		//arduino.setPin(PC2_Y_MIN_PIN, machine.axis[1].position < -5600 ? LOW : HIGH);
		//arduino.setPin(PC2_Z_MIN_PIN, machine.axis[2].position < -5600 ? LOW : HIGH);
        //mt.loop();
        //string s = Serial.output();
        //if (s.length() > 0) {
            //cout << "Serial	:" << s << endl;
			//TESTCOUT3(" position: ", machine.axis[0].position,
				//",", machine.axis[1].position,
				//",", machine.axis[2].position);
        //}
		//arduino.timer1(MS_TICKS(1));
    //} while(mt.status > 0);
    //mt.loop();
    //ASSERTEQUAL(STATUS_WAIT_IDLE, mt.status);

    cout << "TEST	: test_pgm() OK " << endl;
}

void test_ZPlane() {
    cout << "TEST	: test_ZPlane() =====" << endl;

    ZPlane plane;
    PH5TYPE e = 0.000001;
    ASSERTEQUALT(0, plane.calcZ(100000,100000), e);
    ASSERTEQUALT(0, plane.getZOffset(), e);

    XYZ3D p1(1,1,3);
    XYZ3D p2(1,-2,6);
    XYZ3D p3(-1,1,12);
    ASSERT(plane.initialize(p1,p2,p3));
    ASSERTEQUALT(p1.z, plane.calcZ(p1.x,p1.y), e);
    ASSERTEQUALT(p2.z, plane.calcZ(p2.x,p2.y), e);
    ASSERTEQUALT(p3.z, plane.calcZ(p3.x,p3.y), e);
    ASSERTEQUALT(8.5, plane.getZOffset(), e);

    ZPlane plane2;
    plane2 = plane;
    ASSERTEQUALT(p1.z, plane2.calcZ(p1.x,p1.y), e);
    ASSERTEQUALT(p2.z, plane2.calcZ(p2.x,p2.y), e);
    ASSERTEQUALT(p3.z, plane2.calcZ(p3.x,p3.y), e);
    ASSERTEQUALT(8.5, plane2.getZOffset(), e);

    cout << "TEST	: test_ZPlane() OK " << endl;
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
        //test_pgm();
        //test_MTO_FPD_hom();
        test_DeltaCalculator();
        //test_MTO_FPD();
        //test_calibrate();
        //test_eep();
        //test_autoSync();
        //test_mpo();
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
        test_MTO_RAW_hom();
        test_PrettyPrint();
        test_Idle();
        test_Move();
        test_PinConfig();
        test_dvs();
        test_sys();
        test_errors();
        test_ph5();
        test_stroke_endpos();
        test_command_array();
        test_pnp();
        test_io();
        test_eep();
        test_probe();
        test_DeltaCalculator();
        test_MTO_FPD_setup();
        test_MTO_FPD_mov();
        test_MTO_FPD_prb();
        test_MTO_FPD_dim();
        test_MTO_FPD_hom();
        test_MTO_FPD();
        test_autoSync();
        test_msg_cmt_idl();
        test_mpo();
        test_axis();
        test_calibrate();
        test_mark();
        test_ZPlane();
        test_pgm();
        test_gearRatio();
        test_cal_arm();
    }

    cout << "TEST	: END OF TEST main()" << endl;
}

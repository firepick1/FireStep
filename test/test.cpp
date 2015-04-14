#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include "FireLog.h"
#include "FireUtils.hpp"
#include "version.h"
#include "Arduino.h"
#include "SerialTypes.h"

SerialType Serial;

void test_Serial() {
    cout << "TEST	: test_Serial()" << endl;

	ASSERTEQUAL(0, Serial.available());
	Serial.bytes.clear();
	Serial.bytes.push_back(0x01);
	Serial.bytes.push_back(0x02);
	ASSERTEQUAL(2, Serial.available());
	ASSERTEQUAL(0x01, Serial.read());
	ASSERTEQUAL(0x02, Serial.read());
	ASSERTEQUAL(0, Serial.available());

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

	cout << "TEST	:   test_Serial() OK " << endl;
}

int main(int argc, char *argv[]) {
    LOGINFO3("INFO	: FireStep test v%d.%d.%d",
		VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
    firelog_level(FIRELOG_TRACE);

    test_Serial();

    cout << "TEST	: END OF TEST main()" << endl;
}

#include <vector>
#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include "FireUtils.hpp"
#include "Arduino.h"
#include "Thread.h"

SerialType Serial;
MockDuino arduino;

vector<uint8_t> serialbytes;

void SerialType::clear() {
    serialbytes.clear();
    serialout.clear();
    serialline.clear();
}

void SerialType::push(uint8_t value) {
    serialbytes.push_back(value);
}

void SerialType::push(int16_t value) {
    uint8_t *pvalue = (uint8_t *) &value;
    serialbytes.push_back((uint8_t)((value >> 8) & 0xff));
    serialbytes.push_back((uint8_t)(value & 0xff));
}

void SerialType::push(int32_t value) {
    uint8_t *pvalue = (uint8_t *) &value;
    serialbytes.push_back((uint8_t)((value >> 24) & 0xff));
    serialbytes.push_back((uint8_t)((value >> 16) & 0xff));
    serialbytes.push_back((uint8_t)((value >> 8) & 0xff));
    serialbytes.push_back((uint8_t)(value & 0xff));
}

void SerialType::push(float value) {
    uint8_t *pvalue = (uint8_t *) &value;
    serialbytes.push_back(pvalue[0]);
    serialbytes.push_back(pvalue[1]);
    serialbytes.push_back(pvalue[2]);
    serialbytes.push_back(pvalue[3]);
}

void SerialType::push(string value) {
    push(value.c_str());
}

void SerialType::push(const char * value) {
    for (const char *s = value; *s; s++) {
        serialbytes.push_back(*s);
    }
}

string SerialType::output() {
    string result = serialout;
    serialout = "";
    return result;
}

int SerialType::available() {
    return serialbytes.size();
}

void SerialType::begin(long speed) {
}

byte SerialType::read() {
    if (serialbytes.size() < 1) {
        return 0;
    }
    byte c = serialbytes[0];
    serialbytes.erase(serialbytes.begin());
    return c;
}

size_t SerialType::write(uint8_t value) {
    serialout.append(1, (char) value);
	if (value == '\r') {
		serialline.append(1, '\\');
		serialline.append(1, 'r');
		// skip
	} else if (value == '\n') {
        cout << "Serial	: \"" << serialline << "\"" << endl;
        serialline = "";
    } else {
        serialline.append(1, (char)value);
    }
    return 1;
}

void SerialType::print(const char *value) {
    serialout.append(value);
    serialline.append(value);
}

void SerialType::print(int value, int format) {
    stringstream buf;
    switch (format) {
    case HEX:
        buf << std::hex << value;
        buf << std::dec;
        break;
    default:
    case DEC:
        buf << value;
        break;
    }
    string bufVal = buf.str();
    serialline.append(bufVal);
    serialout.append(bufVal);
}

void SerialType::println(const char value, int format) {
    print(value, format);
    write('\n');
}

void SerialType::println(const char *value) {
    if (*value) {
        print(value);
    }
    write('\n');
}

///////////////////// MockDuino ///////////////////

int __heap_start, *__brkval;

MockDuino::MockDuino() {
    clear();
}

int16_t& MockDuino::MEM(int addr) {
    ASSERT(0 <= addr && addr < ARDUINO_MEM);
    return mem[addr];
}

void MockDuino::clear() {
    int novalue = 0xfe;
    Serial.output();	// discard
    for (int i = 0; i < ARDUINO_PINS; i++) {
        pin[i] = NOVALUE;
        _pinMode[i] = NOVALUE;
    }
    for (int i = 0; i < ARDUINO_MEM; i++) {
        mem[i] = NOVALUE;
    }
    memset(pinPulses, 0, sizeof(pinPulses));
    usDelay = 0;
    ADCSRA = 0;	// ADC control and status register A (disabled)
    TCNT1 = 0; 	// Timer/Counter1
    CLKPR = 0;	// Clock prescale register
}

uint32_t MockDuino::pulses(int16_t pin) {
    ASSERT(0 <= pin && pin < ARDUINO_PINS);
    return pinPulses[pin];
}

void MockDuino::dump() {
    for (int i = 0; i < ARDUINO_MEM; i += 16) {
        int dead = true;
        for (int j = 0; j < 16; j++) {
            if (mem[i + j] != NOVALUE) {
                dead = false;
                break;
            }
        }
        if (!dead) {
            cout << "MEM" << setfill('0') << setw(3) << i << "\t: ";
            for (int j = 0; j < 16; j++) {
                cout << setfill('0') << setw(4) << std::hex << mem[i + j] << " ";
                cout << std::dec;
                if (j % 4 == 3) {
                    cout << "| ";
                }
            }
            cout << endl;
        }
    }
}

void MockDuino::timer1(int increment) {
    if (TIMER_ENABLED) {
        TCNT1 += increment;
    }
}

void MockDuino::delay500ns() {
}

void delayMicroseconds(uint16_t usDelay) {
    arduino.usDelay += usDelay;
}

void digitalWrite(int16_t pin, int16_t value) {
    ASSERT(0 <= pin && pin < ARDUINO_PINS);
    ASSERTEQUAL(OUTPUT, arduino.getPinMode(pin));
    if (arduino.pin[pin] != value) {
        if (value == 0) {
            arduino.pinPulses[pin]++;
        }
        arduino.pin[pin] = value ? HIGH : LOW;
    }
}

int16_t digitalRead(int16_t pin) {
    ASSERT(0 <= pin && pin < ARDUINO_PINS);
    ASSERT(arduino.pin[pin] != NOVALUE);
    return arduino.pin[pin];
}

void pinMode(int16_t pin, int16_t inout) {
    ASSERT(0 <= pin && pin < ARDUINO_PINS);
    arduino._pinMode[pin] = inout;
}

int16_t MockDuino::getPinMode(int16_t pin) {
    ASSERT(0 <= pin && pin < ARDUINO_PINS);
    return arduino._pinMode[pin];
}

int16_t MockDuino::getPin(int16_t pin) {
    ASSERT(pin != NOPIN);
    return arduino.pin[pin];
}

void MockDuino::setPin(int16_t pin, int16_t value) {
    if (pin != NOPIN) {
        arduino.pin[pin] = value;
    }
}

void MockDuino::setPinMode(int16_t pin, int16_t value) {
    if (pin != NOPIN) {
        arduino._pinMode[pin] = value;
    }
}

void delay(int ms) {
    arduino.timer1(MS_TICKS(ms));
}

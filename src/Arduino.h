// DUMMY ARDUINO HEADER
#ifndef ARDUINO_H
#define ARDUINO_H
#include <iostream>
#include <sstream>
#include <iomanip>
#include <stdint.h>
#include "../ArduinoJson/include/ArduinoJson/Arduino/Print.hpp"

using namespace std;

typedef uint8_t byte;
typedef uint8_t boolean;

#define NOVALUE 32767 /* 0x77FF */
#define NOVALUESTR "32767"

#define PROGMEM

#define DEC 1
#define BYTE 0
#define HEX 2
#define HIGH 1
#define LOW 0
#define INPUT 0
#define OUTPUT 1

#define ADEN 7
#define ADSC 6
#define ADATE 5
//#define ADFR 5
#define ADIF 4
#define ADIE 3
#define ADPS2 2
#define ADPS1 1
#define ADPS0 0
//MUX bit definitions
#define REFS1 7
#define REFS0 6
#define ADLAR 5
#define MUX3 3
#define MUX2 2
#define MUX1 1
#define MUX0 0
#define PRADC 0
#define TOIE1 0
#define CS10 0
#define CS11 1
#define CS12 2

extern "C" {
    extern unsigned long millis();
}

#define ARDUINO_PINS 127
#define ARDUINO_MEM 1024

#define A0 54
#define A1 (A0+1)
#define A2 (A1+1)
#define A3 (A2+1)
#define A4 (A3+1)
#define A5 (A4+1)
#define A6 (A5+1)
#define A7 (A6+1)
#define A8 (A7+1)
#define A9 (A8+1)
#define A10 (A9+1)
#define A11 (A10+1)
#define A12 (A11+1)
#define A13 (A12+1)
#define A14 (A13+1)
#define A15 (A14+1)

#endif

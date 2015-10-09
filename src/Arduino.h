// DUMMY ARDUINO HEADER
#ifndef ARDUINO_H
#define ARDUINO_H
#include <iostream>
#include <sstream>
#include <iomanip>
#include <stdint.h>
#include "../ArduinoJson/include/ArduinoJson/Arduino/Print.hpp"
#include "MockDuino.h"

using namespace std;

typedef uint8_t byte;
typedef uint8_t boolean;

#define NOVALUE 32767 /* 0x77FF */
#define NOVALUESTR "32767"




extern "C" {
    extern unsigned long millis();
}


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

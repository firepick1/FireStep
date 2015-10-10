// DUMMY ARDUINO HEADER
#ifndef ARDUINO_H
#define ARDUINO_H
#include "MockDuino.h"

using namespace std;

typedef uint8_t byte;
typedef uint8_t boolean;

#define NOVALUE 32767 /* 0x77FF */
#define NOVALUESTR "32767"

extern "C" {
    extern unsigned long millis();
}



#endif

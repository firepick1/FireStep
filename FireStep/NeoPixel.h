#ifndef NEOPIXEL_H
#define NEOPIXEL_H

#include <Arduino.h>

#ifdef CMAKE
#define NEO_GRB 0
#define NEO_KHZ800 0
/////////////////// MOCK BEGIN ///////////
typedef class Adafruit_NeoPixel { 
    public:
        Adafruit_NeoPixel(int a=0, int b=0, int c=0) {}
        uint32_t Color(int r, int g, int b) {
            return 0;
        }
        void setPixelColor(int pixel, uint32_t color) {}
        void begin() {}
        void show() {}
        int numPixels() {return 0;}
} Adafruit_NeoPixel;
/////////////////// MOCK END ///////////
#else
#include <Adafruit_NeoPixel.h>
#include <avr/power.h>
#endif

#include "Display.h"
#include "Thread.h"

namespace firestep {

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.
//
typedef class NeoPixel : public Display {
    protected:
        Adafruit_NeoPixel strip;
        uint8_t curLevel;
        uint8_t curStatus;
        uint8_t fgIndex;
        Ticks fgTicks;
        uint32_t fg; // foreground color
        uint32_t bg; // background color
    public:
        NeoPixel(uint16_t ledCount, PinType pin);
        void begin();
        void show();
} NeoPixel;

} // namespace firestep

#endif

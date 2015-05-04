#include "NeoPixel.h"
#include "pins.h"

using namespace firestep;

NeoPixel::NeoPixel(uint16_t ledCount, PinType pin)
    : strip(ledCount, pin, NEO_GRB + NEO_KHZ800)
{}

void NeoPixel::setup() {
    // Parameter 1 = number of pixels in strip
    // Parameter 2 = Arduino pin number (most are valid)
    // Parameter 3 = pixel type flags, add together as needed:
    //   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
    //   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
    //   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
    //   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
    strip.begin();
    strip.show(); 
    fgIndex = 0;
    fgTicks = 0;
    curLevel = 0;
    curStatus = DISPLAY_WAIT_IDLE;
	show();
}

void NeoPixel::show() {
    bool updateDisplay = (curLevel == level && curStatus == status);
    if (fgTicks < threadClock.ticks && strip.numPixels()) {
        fgIndex = (fgIndex + 1) % strip.numPixels();
        fgTicks = threadClock.ticks + MS_TICKS(3000 / 16);
        updateDisplay = true;
    }
    if (!updateDisplay) {
        return;
    }
    curLevel = level;

    int intensity = curLevel;
    curStatus = status;
    switch (curStatus) {
    case DISPLAY_WAIT_IDLE:
        bg = strip.Color(0,0,0);
        fg = strip.Color(intensity, intensity, intensity);
        break;
    case DISPLAY_WAIT_EOL:
        bg = strip.Color(intensity / 2, intensity / 2, intensity / 2);
        fg = strip.Color(intensity, intensity, intensity);
        break;
    default:
    case DISPLAY_WAIT_ERROR:
        bg = strip.Color(intensity / 2, 0, 0);
        fg = strip.Color(255, 0, 0);
        break;
    case DISPLAY_WAIT_OPERATOR:
        bg = strip.Color(intensity , intensity, intensity );
        fg = strip.Color(255, 0, 0);
        break;
    case DISPLAY_WAIT_CAMERA:
        fg = bg = strip.Color(intensity, intensity, intensity);
        break;
    case DISPLAY_WAIT_BUSY:
    case DISPLAY_BUSY:
        bg = strip.Color(intensity/2, intensity/2, intensity/2);
        fg = strip.Color(0, 0, intensity);
        break;
    case DISPLAY_BUSY_MOVING:
        bg = strip.Color(intensity/4, intensity / 4, intensity/4);
        fg = strip.Color(0, intensity, 0);
        break;
    }
    for (uint16_t i = 0; i < strip.numPixels(); i++) {
        if (i == fgIndex) {
            strip.setPixelColor(i, fg);
        } else {
            strip.setPixelColor(i, bg);
        }
    }
    strip.show();
}

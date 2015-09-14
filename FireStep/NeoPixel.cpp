#include "NeoPixel.h"
#include "pins.h"

using namespace firestep;

NeoPixel::NeoPixel(uint16_t ledCount)
    : strip(ledCount, 0, NEO_GRB + NEO_KHZ800)
{}

void NeoPixel::setup(IDuinoPtr pDuino, int pin) {
	Display::setup(pDuino, pin);
    // Parameter 1 = number of pixels in strip
    // Parameter 2 = Arduino pin number (most are valid)
    // Parameter 3 = pixel type flags, add together as needed:
    //   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
    //   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
    //   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
    //   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
    if (pin != NOPIN) {
        strip.setPin(pin);
        strip.begin();
        strip.show();
    }
	this->pDuino = pDuino;
    fgIndex = 0;
    fgMillis = 0;
    cameraR = 0;
    cameraG = 0;
    cameraB = 0;
    curLevel = 0;
    curStatus = DISPLAY_WAIT_IDLE;
    show();
}

void NeoPixel::show() {
    bool updateDisplay = curLevel != level || curStatus != status;
    if (fgMillis < pDuino->millis() && strip.numPixels()) {
        fgIndex = (fgIndex + 1) % strip.numPixels();
        fgMillis = pDuino->millis() + 3000/16;
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
        bg = strip.Color(0, 0, 0);
        fg = strip.Color(intensity, intensity, intensity);
        break;
    case DISPLAY_WAIT_IDLE_FPD:
        bg = strip.Color(0, 0, 0);
        fg = strip.Color(intensity/8, intensity, intensity/8);
        break;
    case DISPLAY_EEPROM:
        bg = strip.Color(0, 0, intensity);
        fg = strip.Color(intensity, intensity, 0);
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
        fg = strip.Color(255, 0, 0);	// red
        break;
    case DISPLAY_WAIT_CANCELLED:
        bg = strip.Color(intensity , intensity, intensity );
        fg = strip.Color(255, 255, 0);	// yellow
        break;
    case DISPLAY_WAIT_CAMERA:
        fg = bg = strip.Color(
                      cameraR ? cameraR : intensity,
                      cameraG ? cameraG : intensity,
                      cameraB ? cameraB : intensity);
        break;
    case DISPLAY_BUSY:
        bg = strip.Color(intensity, intensity, 0); // yellow
        fg = strip.Color(255, 255, 255); // yellow
        break;
    case DISPLAY_BUSY_CALIBRATING:
        bg = strip.Color(intensity / 2, intensity / 2, intensity / 2);
        fg = strip.Color(0, 255, 0);	// green
        break;
    case DISPLAY_BUSY_MOVING:
        bg = strip.Color(intensity / 4, intensity / 4, intensity / 4);
        fg = strip.Color(0, intensity, 0);	// green
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

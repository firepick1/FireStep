#include "NeoPixel.h"
#include "pins.h"

using namespace firestep;

NeoPixel::NeoPixel(PinType pin) {
//#if defined (__AVR_ATtiny85__)
    //if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
//#endif
    // End of trinket special code

	// Parameter 1 = number of pixels in strip
	// Parameter 2 = Arduino pin number (most are valid)
	// Parameter 3 = pixel type flags, add together as needed:
	//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
	//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
	//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
	//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
    strip = Adafruit_NeoPixel(60, pin, NEO_GRB + NEO_KHZ800);
    strip.begin();
    strip.show(); // Initialize all pixels to 'off'
	fgIndex = 0;
	fgTicks = 0;
	curStatus = level = DISPLAY_NORMAL;
	curStatus = status = DISPLAY_PROCESSING;
}

void NeoPixel::show() {
	bool updateDisplay = (curLevel == level && curStatus == status);
	if (fgTicks < threadClock.ticks) {
		fgIndex = (fgIndex+1) % strip.numPixels();
		fgTicks = threadClock.ticks + MS_TICKS(2000/16);
		updateDisplay = true;
	}
	if (!updateDisplay) {
		return;
	}
	curLevel = level;

	int intensity;
	switch (curLevel) {
		case DISPLAY_OFF:
			intensity = 0;
			break;
		case DISPLAY_LOW:
			intensity = 63;
			break;
		default:
		case DISPLAY_NORMAL:
			intensity = 127;
			break;
		case DISPLAY_NIGH:
			intensity = 191;
			break;
		case DISPLAY_HIGHEST:
			intensity = 255;
			break;
	}
	curStatus = status;
	switch (curStatus) {
		case DISPLAY_IDLE:
        	bg = strip.Color(intensity/2, intensity/2, intensity/2);
			fg = strip.Color(intensity, intensity, intensity);
			break;
		default:
    	case DISPLAY_ERROR:
        	bg = strip.Color(intensity/2, intensity/4, intensity/4);
			fg = strip.Color(intensity, intensity/2, intensity/2);
			break;
    	case DISPLAY_OPERATOR:
        	bg = strip.Color(intensity/2, intensity/2, intensity/4);
			fg = strip.Color(intensity, 0, 0);
			break;
    	case DISPLAY_PROCESSING:
        	bg = strip.Color(intensity/4, intensity/4, intensity/2);
			fg = strip.Color(intensity/2, intensity/2, intensity);
			break;
    	case DISPLAY_MOVING:
        	bg = strip.Color(intensity/4, intensity/2, intensity/4);
			fg = strip.Color(intensity, intensity/2, intensity);
			break;
    	case DISPLAY_CAMERA:
        	fg = bg = strip.Color(intensity, intensity, intensity);
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

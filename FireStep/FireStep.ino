#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include "Arduino.h"
#include "MachineThread.h"

firestep::MachineThread mainThread;

void setup()										// run once, when the sketch starts
{
	Serial.begin(115200); 
	//Serial.begin(57600);

	mainThread.setup();
	firestep::threadRunner.setup(LED_PIN_RED, LED_PIN_GRN);
}

void loop()										 // run over and over again
{
	firestep::threadRunner.run();
}

#ifdef TBD
#include <Adafruit_NeoPixel.h>
#include <avr/power.h>

#define PIN 8

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(60, PIN, NEO_GRB + NEO_KHZ800);

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

void setup() {
#if defined (__AVR_ATtiny85__)
  if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
#endif
  // End of trinket special code


  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  
  int uniform=1;
  
  int lvl = 127;
  int gb = 2;
  if (uniform) {
    uint32_t c = strip.Color(lvl,lvl/gb,lvl/gb);
    colorWipe(c, 50);
  } else {
    uint32_t c = strip.Color(lvl,lvl/gb,lvl/gb);
  //  strip.setPixelColor(2, c);
  //  strip.setPixelColor(3, c);
    strip.setPixelColor(4, c);
    strip.setPixelColor(5, c);
    strip.setPixelColor(6, c);
    strip.setPixelColor(7, c);
    strip.setPixelColor(8, c);
    strip.setPixelColor(9, c);
    strip.setPixelColor(10, c);
    strip.setPixelColor(11, c);
  //  strip.setPixelColor(12, c);
  //  strip.setPixelColor(13, c);
    strip.show();
  }
}
void loop() {
}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, c);
      strip.show();
      delay(wait);
  }
}

#endif

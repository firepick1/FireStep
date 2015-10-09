#ifndef MOCKDUINO_H
#define MOCKDUINO_H

#include <stdint.h>

namespace mockduino {
	int16_t digitalRead(int16_t pin);
	void digitalWrite(int16_t dirPin, int16_t value);
}

#endif

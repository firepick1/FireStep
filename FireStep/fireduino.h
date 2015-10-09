#ifndef FIREDUINO_H
#define FIREDUINO_H

namespace fireduino {
inline int16_t digitalRead(int16_t pin) {
	return ::digitalRead(pin);
}
} // fireduino

#endif

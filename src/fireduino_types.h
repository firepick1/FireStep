#ifndef FIREDUINO_TYPES_H
#define FIREDUINO_TYPES_H

// Common includes for non-Arduino implementations

typedef uint8_t byte;

#ifndef Arduino_h
#define DEC 1
#define BYTE 0
#define HEX 2
#define HIGH 1
#define LOW 0
#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLUP 2
#endif

#ifndef HIGH
#define HIGH 1
#define LOW 0
#define DEC 1
#define BYTE 0
#define HEX 2
#define INPUT 0
#define OUTPUT 1
#endif

#ifndef PI
#define PI 3.1415926535897932384626433832795
#define HALF_PI 1.5707963267948966192313216916398
#define TWO_PI 6.283185307179586476925286766559
#define DEG_TO_RAD 0.017453292519943295769236907684886
#define RAD_TO_DEG 57.295779513082320876798154814105
#define EULER 2.718281828459045235360287471352
#endif

#endif

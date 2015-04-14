#include "Arduino.h"
#include "SerialTypes.h"

void sendHex(byte value) {
    if (value < 16) {
        Serial.print(0, HEX);
    }
    Serial.print(value, HEX);
}

void SerialInt16::read() {
    while (Serial.available() < 2) {
        // wait;
    }

    msb = Serial.read();
    lsb = Serial.read();
}

void SerialInt16::send() {
    sendHex(msb);
    sendHex(lsb);
}

void SerialInt32::read() {
    while (Serial.available() < 4) {
        // wait;
    }

    msb = Serial.read();
    b3 = Serial.read();
    b2 = Serial.read();
    lsb = Serial.read();
}

void SerialInt32::send() {
    sendHex(msb);
    sendHex(b3);
    sendHex(b2);
    sendHex(lsb);
}

void SerialVector8::read() {
    while (Serial.available() < 3) {
        // wait;
    }

    bx = Serial.read();
    by = Serial.read();
    bz = Serial.read();
}

void SerialVector16::read() {
    x.read();
    y.read();
    z.read();
}

void SerialVector16::increment(union SerialVector8 *pThat) {
    x.intValue = x.intValue + pThat->x;
    y.intValue = y.intValue + pThat->y;
    z.intValue = z.intValue + pThat->z;
}

void SerialVector16::copyFrom(struct SerialVector16 *pThat) {
    x.intValue = pThat->x.intValue;
    y.intValue = pThat->y.intValue;
    z.intValue = pThat->z.intValue;
}

void SerialVector16::clear() {
    x.intValue = 0;
    y.intValue = 0;
    z.intValue = 0;
}

void SerialVector32::clear() {
    x.longValue = 0;
    y.longValue = 0;
    z.longValue = 0;
}

void SerialVector32::increment(struct SerialVector16 *pThat) {
    x.longValue = x.longValue + pThat->x.intValue;
    y.longValue = y.longValue + pThat->y.intValue;
    z.longValue = z.longValue + pThat->z.intValue;
}

void SerialVector32::increment(struct SerialVector32 *pThat) {
    x.longValue += pThat->x.longValue;
    y.longValue += pThat->y.longValue;
    z.longValue += pThat->z.longValue;
}

void SerialVector32::increment(struct SerialVectorF *pThat) {
    x.longValue += pThat->x;
    y.longValue += pThat->y;
    z.longValue += pThat->z;
}

void SerialVector32::decrement(struct SerialVector32 *pThat) {
    x.longValue -= pThat->x.longValue;
    y.longValue -= pThat->y.longValue;
    z.longValue -= pThat->z.longValue;
}

void SerialVector32::copyFrom(struct SerialVectorF *pThat) {
    x.longValue = pThat->x;
    y.longValue = pThat->y;
    z.longValue = pThat->z;
}

void SerialVector32::copyFrom(struct SerialVector32 *pThat) {
    x.longValue = pThat->x.longValue;
    y.longValue = pThat->y.longValue;
    z.longValue = pThat->z.longValue;
}

void SerialVector32::interpolateTo(struct SerialVector32 *pThat, float p) {
    float p1 = 1 - p;
    x.longValue = (long) (p1 * (float) x.longValue + p * (float) pThat->x.longValue);
    y.longValue = (long) (p1 * (float) y.longValue + p * (float) pThat->y.longValue);
    z.longValue = (long) (p1 * (float) z.longValue + p * (float) pThat->z.longValue);
}

void SerialVector32::read() {
    x.read();
    y.read();
    z.read();
}

void SerialVectorF::copyFrom(struct SerialVector16 *pThat) {
    x = pThat->x.intValue;
    y = pThat->y.intValue;
    z = pThat->z.intValue;
}

void SerialVectorF::copyFrom(struct SerialVector32 *pThat) {
    x = pThat->x.longValue;
    y = pThat->y.longValue;
    z = pThat->z.longValue;
}

void SerialVectorF::increment(struct SerialVector32 *pThat) {
    x = x + (float) pThat->x.longValue;
    y = y + (float) pThat->y.longValue;
    z = z + (float) pThat->z.longValue;
}

void SerialVectorF::multiply(struct SerialVectorF *pThat) {
    x = x * (float) pThat->x;
    y = y * (float) pThat->y;
    z = z * (float) pThat->z;
}

void SerialVectorF::divide(struct SerialVector32 *pThat) {
    x =  x / (float) pThat->x.longValue;
    y =  y / (float) pThat->y.longValue;
    z =  z / (float) pThat->z.longValue;
}

void SerialVectorF::scale(float value) {
    x = x * value;
    y = y * value;
    z = z * value;
}


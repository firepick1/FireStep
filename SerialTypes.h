#ifndef SERIALTYPES_H
#define SERIALTYPES_H

typedef union SerialInt16 {
    struct {
        int16_t intValue;
    };
    struct {
        uint8_6 lsb;
        uint8_6 msb;
    };

    void read();
    void send();
} SerialInt16;

typedef union SerialInt32 {
    struct {
        int32_t longValue;
    };
    struct {
        uint8_t lsb;
        uint8_t b2;
        uint8_t b3;
        uint8_t msb;
    };
    struct {
        int16_t lsInt;
        int16_t msInt;
    };

    void read();
    void send();
} SerialInt32;

typedef struct SerialVectorF {
    float x;
    float y;
    float z;

    void copyFrom(struct SerialVector16 *pThat);
    void copyFrom(struct SerialVector32 *pThat);
    void increment(struct SerialVector32 *pThat);
    void multiply(struct SerialVectorF *pThat);
    void divide(struct SerialVector32 *pThat);
    void scale(float value);
} SerialVectorF;

typedef union SerialVector8 {
    struct {
        int8_t x;
        int8_t y;
        int8_t z;
    };
    struct {
        uint8_t bx;
        uint8_t by;
        uint8_t bz;
    };

    void read();
} SerialVector8;

typedef struct SerialVector16 {
    SerialInt16 x;
    SerialInt16 y;
    SerialInt16 z;

    void read();
    void clear();
    void copyFrom(struct SerialVector16 *pThat);
    void increment(union SerialVector8 *pThat);
} SerialVector16;

typedef struct SerialVector32 {
    SerialInt32 x;
    SerialInt32 y;
    SerialInt32 z;

    void clear();
    void copyFrom(struct SerialVector32 *pThat);
    void copyFrom(struct SerialVectorF *pThat);
    void increment(struct SerialVector16 *pThat);
    void increment(struct SerialVector32 *pThat);
    void increment(struct SerialVectorF *pThat);
    void decrement(struct SerialVector32 *pThat);
    void interpolateTo(struct SerialVector32 *pThat, float p);
    void read();
} SerialVector32;

#endif

#ifndef SERIALTYPES_H
#define SERIALTYPES_H

typedef union SerialInt16 {
	struct {
		int intValue;
	};
	struct {
		byte lsb;
		byte msb;
	};

	void read();
	void send();
} SerialInt16;

typedef union SerialInt32 {
	struct {
		long longValue;
	};
	struct {
		byte lsb;
		byte b2;
		byte b3;
		byte msb;
	};
	struct {
		int lsInt;
		int msInt;
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
};

typedef union SerialVector8 {
	struct {
		char x;
		char y;
		char z;
	};
	struct {
		byte bx;
		byte by;
		byte bz;
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

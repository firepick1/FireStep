// DUMMY ARDUINO HEADER
#ifndef ARDUINO_H
#define ARDUINO_H
#include <vector>
#include <iostream>
#include <stdint.h>

using namespace std;

typedef uint8_t byte;


#define HEX 1

typedef class SerialType {
    public:
        vector<uint8_t> bytes;
		void push(int16_t value) {
			uint8_t *pvalue = (uint8_t *) &value;
			bytes.push_back((uint8_t)((value>>8) & 0xff));
			bytes.push_back((uint8_t)(value & 0xff));
		}
		void push(int32_t value) {
			uint8_t *pvalue = (uint8_t *) &value;
			bytes.push_back((uint8_t)((value>>24) & 0xff));
			bytes.push_back((uint8_t)((value>>16) & 0xff));
			bytes.push_back((uint8_t)((value>>8) & 0xff));
			bytes.push_back((uint8_t)(value & 0xff));
		}
		void push(float value) {
			uint8_t *pvalue = (uint8_t *) &value;
			bytes.push_back(pvalue[0]);
			bytes.push_back(pvalue[1]);
			bytes.push_back(pvalue[2]);
			bytes.push_back(pvalue[3]);
		}

    public:
        int available() {
            return bytes.size();
        }

        byte read() {
            if (bytes.size() < 1) {
                return 0;
            }
            byte c = bytes[0];
            bytes.erase(bytes.begin());
            return c;
        }
		
        void print(byte value, byte format) {
            cout << "Serial:" << value << endl;
        }

}SerialType;

extern SerialType Serial;


#endif

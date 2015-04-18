// DUMMY ARDUINO HEADER
#ifndef ARDUINO_H
#define ARDUINO_H
#include <vector>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <stdint.h>
#include "../ArduinoJson/include/ArduinoJson/Arduino/Print.hpp"

using namespace std;

typedef uint8_t byte;
typedef uint8_t boolean;

#define NOVALUE 57005 /* 0xDEAD */
#define NOVALUESTR "57005"


#define DEC 1
#define BYTE 0
#define HEX 2
#define HIGH 1
#define LOW 0
#define INPUT 0
#define OUTPUT 1

#define ADEN 7
#define ADSC 6
#define ADATE 5
//#define ADFR 5
#define ADIF 4
#define ADIE 3
#define ADPS2 2
#define ADPS1 1
#define ADPS0 0
//MUX bit definitions
#define REFS1 7
#define REFS0 6
#define ADLAR 5
#define MUX3 3
#define MUX2 2
#define MUX1 1
#define MUX0 0
#define PRADC 0
#define TOIE1 0
#define CS10 0
#define CS11 1
#define CS12 2

#define ADCH arduino.mem[0]
#define ADCSRA arduino.mem[1]
#define ADCSRB arduino.mem[2]
#define ADMUX arduino.mem[3]
#define CLKPR arduino.mem[4]
#define DIDR0 arduino.mem[5]
#define PRR arduino.mem[6]
#define SREGI arduino.mem[7]
#define TCCR1A arduino.mem[8]
#define TCCR1B arduino.mem[9]
#define TCNT1 arduino.mem[10]
#define TIMSK1 arduino.mem[11]

#define cli() SREGI=0
#define sei() SREGI=1

extern "C" {
    extern long millis();
}

typedef class SerialType : public Print {
    private:
        string serialout;
		string serialline;
        vector<uint8_t> bytes;

    public:
		void clear() {
			bytes.clear();
		}
		void push(uint8_t value) {
            bytes.push_back(value);
		}
        void push(int16_t value) {
            uint8_t *pvalue = (uint8_t *) &value;
            bytes.push_back((uint8_t)((value >> 8) & 0xff));
            bytes.push_back((uint8_t)(value & 0xff));
        }
        void push(int32_t value) {
            uint8_t *pvalue = (uint8_t *) &value;
            bytes.push_back((uint8_t)((value >> 24) & 0xff));
            bytes.push_back((uint8_t)((value >> 16) & 0xff));
            bytes.push_back((uint8_t)((value >> 8) & 0xff));
            bytes.push_back((uint8_t)(value & 0xff));
        }
        void push(float value) {
            uint8_t *pvalue = (uint8_t *) &value;
            bytes.push_back(pvalue[0]);
            bytes.push_back(pvalue[1]);
            bytes.push_back(pvalue[2]);
            bytes.push_back(pvalue[3]);
        }
		void push(const char * value) {
			for (const char *s=value; *s; s++) {
				bytes.push_back(*s);
			}
		}
        string output() {
            string result = serialout;
            serialout = "";
            return result;
        }

    public:
        int available() {
            return bytes.size();
        }
        void begin(long speed) {
        }

        byte read() {
            if (bytes.size() < 1) {
                return 0;
            }
            byte c = bytes[0];
            bytes.erase(bytes.begin());
            return c;
        }

		virtual size_t write(uint8_t value) {
            serialout.append(1, (char) value);
            if (value == '\n') {
                cout << "Serial	: \"" << serialline << "\"" << endl;
				serialline = "";
            } else {
				serialline.append(1, (char)value);
			}
			return 1;
		}
        void print(const char *value) {
            serialout.append(value);
			serialline.append(value);
        }
        void print(int value, int format = DEC) {
            stringstream buf;
            switch (format) {
            case HEX:
                buf << std::hex << value;
                break;
            default:
            case DEC:
                buf << value;
                break;
            }
			string bufVal = buf.str();
			serialline.append(bufVal);
            serialout.append(bufVal);
        }
        void println(const char value, int format = DEC) {
            print(value, format);
            write('\n');
        }
        void println(const char *value = "") {
			if (*value) {
				print(value);
			}
            write('\n');
        }
} SerialType;


void digitalWrite(int dirPin, int value);
int digitalRead(int dirPin);
void pinMode(int pin, int inout);
void delay(int ms);

extern SerialType Serial;

#define ARDUINO_PINS 127
#define ARDUINO_MEM 1024
typedef class ArduinoType {
    public:
        uint16_t pinMode[ARDUINO_PINS];
        uint16_t pin[ARDUINO_PINS];
        uint16_t mem[ARDUINO_MEM];
		uint32_t pinPulses[ARDUINO_PINS];

    public:
        ArduinoType();
		void dump();
		void clear();
		void timer1(int increment=1);
		void delay500ns();
} ArduinoType;

#define DELAY500NS arduino.delay500ns();

extern ArduinoType arduino;

#endif

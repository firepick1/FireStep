#include "Mockino.h"
#include <fstream>
#include <sstream>
//#include "Thread.h"
#include "FireUtils.h"

using namespace std;
using namespace firestep;

Mockino firestep::mockino;

Mockino::Mockino() {
    clear();
}

void Mockino::clear() {
    int novalue = 0xfe;
    serial_output();	// discard
    for (int16_t i = 0; i < ARDUINO_PINS; i++) {
        pins[i] = NOVALUE;
        _pinMode[i] = NOVALUE;
    }
    for (int16_t i = 0; i < ARDUINO_MEM; i++) {
        mem[i] = NOVALUE;
    }
	for (int16_t i=0; i<EEPROM_SIZE; i++) {
		eeprom_data[i] = NOVALUE;
	}
    memset(pinPulses, 0, sizeof(pinPulses));
    usDelay = 0;
    //ADCSRA = 0;	// ADC control and status register A (disabled)
    //TCNT1 = 0; 	// Timer/Counter1
    //CLKPR = 0;	// Clock prescale register
	///sei(); // enable interrupts
	ticksEnabled = true;
	_ticks = 1; // must be >0 so we can subtract one
}

void Mockino::serial_clear() {
    serialbytes.clear();
    serialout.clear();
    serialline.clear();
}

//void Mockino::serial_push(uint8_t value) {
    //serialbytes.push_back(value);
//}

//void Mockino::serial_push(int16_t value) {
    //uint8_t *pvalue = (uint8_t *) &value;
    //serialbytes.push_back((uint8_t)((value >> 8) & 0xff));
    //serialbytes.push_back((uint8_t)(value & 0xff));
//}

//void Mockino::serial_push(int32_t value) {
    //uint8_t *pvalue = (uint8_t *) &value;
    //serialbytes.push_back((uint8_t)((value >> 24) & 0xff));
    //serialbytes.push_back((uint8_t)((value >> 16) & 0xff));
    //serialbytes.push_back((uint8_t)((value >> 8) & 0xff));
    //serialbytes.push_back((uint8_t)(value & 0xff));
//}

//void Mockino::serial_push(float value) {
    //uint8_t *pvalue = (uint8_t *) &value;
    //serialbytes.push_back(pvalue[0]);
    //serialbytes.push_back(pvalue[1]);
    //serialbytes.push_back(pvalue[2]);
    //serialbytes.push_back(pvalue[3]);
//}

void Mockino::serial_push(string value) {
    serial_push(value.c_str());
}

void Mockino::serial_push(const char * value) {
    for (const char *s = value; *s; s++) {
        serialbytes.push_back(*s);
    }
}

string Mockino::serial_output() {
    string result = serialout;
    serialout = "";
    return result;
}

int Mockino::serial_available() {
    return serialbytes.size();
}

void Mockino::serial_begin(long speed) {
}

byte Mockino::serial_read() {
    if (serialbytes.size() < 1) {
        return 0;
    }
    byte c = serialbytes[0];
    serialbytes.erase(serialbytes.begin());
    return c;
}

size_t Mockino::write(uint8_t value) {
    serialout.append(1, (char) value);
	if (value == '\r') {
		serialline.append(1, '\\');
		serialline.append(1, 'r');
		// skip
	} else if (value == '\n') {
        cout << "Serial	: \"" << serialline << "\"" << endl;
        serialline = "";
    } else {
        serialline.append(1, (char)value);
    }
    return 1;
}

void Mockino::serial_print(const char value) {
	char buf[2];
	buf[0] = value;
	buf[1] = 0;
    serialout.append(buf);
    serialline.append(buf);
}

void Mockino::serial_print(const char *value) {
    serialout.append(value);
    serialline.append(value);
}

void Mockino::serial_print(int value, int format) {
    stringstream buf;
    switch (format) {
    case HEX:
        buf << std::hex << value;
        buf << std::dec;
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

int16_t& Mockino::MEM(int addr) {
    ASSERT(0 <= addr && addr < ARDUINO_MEM);
    return mem[addr];
}

int32_t Mockino::pulses(int16_t pin) {
    ASSERT(0 <= pin && pin < ARDUINO_PINS);
    return pinPulses[pin];
}

void Mockino::dump() {
    for (int i = 0; i < ARDUINO_MEM; i += 16) {
        int dead = true;
        for (int j = 0; j < 16; j++) {
            if (mem[i + j] != NOVALUE) {
                dead = false;
                break;
            }
        }
        if (!dead) {
            cout << "MEM" << setfill('0') << setw(3) << i << "\t: ";
            for (int j = 0; j < 16; j++) {
                cout << setfill('0') << setw(4) << std::hex << mem[i + j] << " ";
                cout << std::dec;
                if (j % 4 == 3) {
                    cout << "| ";
                }
            }
            cout << endl;
        }
    }
}

void Mockino::delay500ns() {
}

void Mockino::delayMicroseconds(uint16_t us) {
    usDelay += us;
	_ticks += MS_TICKS(us/1000.0);
}

void Mockino::analogWrite(int16_t pin, int16_t value) {
    ASSERT(0 <= pin && pin < ARDUINO_PINS);
    ASSERTEQUAL(OUTPUT, getPinMode(pin));
	ASSERT(0 <= value && value <= 255);
	pins[pin] = value;
}

int16_t Mockino::analogRead(int16_t pin) {
    ASSERT(0 <= pin && pin < ARDUINO_PINS);
    ASSERTEQUAL(INPUT, getPinMode(pin));
    ASSERT(pins[pin] != NOVALUE);
	return pins[pin];
}

void Mockino::digitalWrite(int16_t pin, int16_t value) {
    ASSERT(0 <= pin && pin < ARDUINO_PINS);
	if (getPinMode(pin) != OUTPUT) {
		cerr << "digitalWrite(" << pin << "," << value << ") pin mode not OUTPUT" << endl;
		ASSERTEQUAL(OUTPUT, getPinMode(pin));
	}
    if (pins[pin] != value) {
        if (value == 0) {
            pinPulses[pin]++;
        }
        pins[pin] = value ? HIGH : LOW;
    }
}

int16_t Mockino::digitalRead(int16_t pin) {
    ASSERT(0 <= pin && pin < ARDUINO_PINS);
	//if (getPinMode(pin) != INPUT) {
		//cerr << "digitalRead(" << pin << "," << value << ") pin mode not INPUT" << endl;
		//ASSERTEQUAL(INPUT, getPinMode(pin));
	//}
    ASSERT(pins[pin] != NOVALUE);
    return pins[pin];
}

void Mockino::pinMode(int16_t pin, int16_t inout) {
    ASSERT(0 <= pin && pin < ARDUINO_PINS);
    _pinMode[pin] = inout;
}

int16_t Mockino::getPinMode(int16_t pin) {
    ASSERT(0 <= pin && pin < ARDUINO_PINS);
    return _pinMode[pin];
}

int16_t Mockino::getPin(int16_t pin) {
    ASSERT(pin != NOPIN);
    return pins[pin];
}

void Mockino::setPin(int16_t pin, int16_t value) {
    if (pin != NOPIN) {
		ASSERT(0 <= pin && pin < ARDUINO_PINS);
        pins[pin] = value;
    }
}

void Mockino::setPinMode(int16_t pin, int16_t value) {
    if (pin != NOPIN) {
        _pinMode[pin] = value;
    }
}

void Mockino::delay(int ms) {
	if (ticksEnabled) {
		setTicks(_ticks + MS_TICKS(ms));
	}
}

uint8_t Mockino::eeprom_read_byte(uint8_t *addr) {
    if ((size_t) addr < 0 || EEPROM_SIZE <= (size_t) addr) {
        return 255;
    }
    return eeprom_data[(size_t) addr];
}

void Mockino::eeprom_write_byte(uint8_t *addr, uint8_t value) {
    if (0 <= (size_t) addr && (size_t) addr < EEPROM_SIZE) {
        eeprom_data[(size_t) addr] = value;
    }
}

string Mockino::eeprom_read_string(uint8_t *addr) {
	string result;
	for (size_t i=0; i+(size_t)addr<EEPROM_SIZE; i++) {
		uint8_t b = eeprom_read_byte(i+addr);
		if (!b) { break; }
		result += (char) b;
	}
	return result;
}	

Ticks Mockino::ticks(bool peek) {
	if (ticksEnabled && !peek) {
		return _ticks++;
	} 

	return _ticks;
}

void Mockino::setTicks(Ticks value) {
	_ticks = value;
}

bool Mockino::isTicksEnabled() {
	return ticksEnabled;
}

void Mockino::enableTicks(bool enable) {
	if (ticksEnabled) {
		_ticks--; // balance out post-increment for testing
	}
	ticksEnabled = enable;
}


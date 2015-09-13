#ifndef MOCKINO_H
#define MOCKINO_H

#include <vector>
#include "IDuino.h"
#include "Thread.h"
//#include "Mega2560.h"

namespace firestep {

typedef class Mockino : public IDuino {
private:
    vector<uint8_t> serialbytes;
    std::string		serialout;
    std::string		serialline;
    int16_t			eeprom_data[EEPROM_SIZE];
    int16_t			pins[ARDUINO_PINS];
    int16_t			_pinMode[ARDUINO_PINS];
    int32_t			pinPulses[ARDUINO_PINS];
    int16_t			mem[ARDUINO_MEM];
    int32_t			usDelay;
    Ticks			_ticks;
    bool			_timer_enabled;

public: // construction
    Mockino();

public: // ArduinoJson Print
    virtual size_t write(uint8_t value);

public: // Serial
    virtual byte serial_read();
    virtual int serial_available();
    virtual void serial_begin(long baud);
    virtual void serial_print(const char *value);
    virtual void serial_print(const char value);
    virtual void serial_print(int value, int format = DEC);

public: // Pins
    virtual void analogWrite(int16_t dirPin, int16_t value);
    virtual int16_t analogRead(int16_t dirPin);
    virtual void digitalWrite(int16_t dirPin, int16_t value);
    virtual int16_t digitalRead(int16_t dirPin);
    virtual void pinMode(int16_t pin, int16_t inout);

public: // misc
    virtual void delay(int ms);
    virtual void delayMicroseconds(uint16_t usDelay);
    virtual void timer_enable(bool enable);
    virtual bool timer_enabled();

public: // EEPROM
    virtual uint8_t		eeprom_read_byte(uint8_t *addr);
    virtual void		eeprom_write_byte(uint8_t *addr, uint8_t value);
    virtual std::string eeprom_read_string(uint8_t *addr);

public: // FireStep
    virtual void pulseFast(uint8_t pin) {
        digitalWrite(pin, HIGH);
        digitalWrite(pin, LOW);
    }
    virtual Ticks ticks();

public: // Testing: Serial
    std::string serial_output();
    void serial_push(std::string value);
    void serial_push(const char *value);
    void serial_clear();

public: // Testing: other
    void dump();
    int16_t& MEM(int addr);
    void clear();
    void delay500ns();
    void setTicks(Ticks value);
    int16_t getPinMode(int16_t pin);
    int16_t getPin(int16_t pin);
    void setPin(int16_t pin, int16_t value);
    void setPinMode(int16_t pin, int16_t value);
    int32_t pulses(int16_t pin);
    uint32_t get_usDelay() {
        return usDelay;
    }

} Mockino;

extern Mockino mockino;

}

#endif

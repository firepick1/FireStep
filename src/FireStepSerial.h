#ifndef FIRESTEPSERIAL_H
#define FIRESTEPSERIAL_H

#include "ArduinoUSB.h"
#include "FireStep.h"

namespace firestep {

typedef class FireStepSerial : public IFireStep {
protected:
    std::string serialPath;
    int32_t msResponse; // response timeout
    firestep::ArduinoUSB usb;

protected: /* IFireStep */ 
    virtual int executeCore(const std::string &jsonRequest, std::string &jsonResponse);

public: // construction
    FireStepSerial(const char *serialPath=FIRESTEP_SERIAL_PATH, int32_t msResponse=10*1000);
    ~FireStepSerial();
public: /* IFireStep */
    virtual int reset();
public: /* IFireStep */ 
    virtual int open();
public: /* IFireStep */ 
    virtual int close();
} FireStepSerial;

}

#endif


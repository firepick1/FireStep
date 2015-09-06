#ifndef FIRESTEPCLIENT_H
#define FIRESTEPCLIENT_H

#include "ArduinoUSB.h"

namespace firestep {

typedef class FireStepClient {
private:
    bool prompt;
    std::string serialPath;
    int32_t msResponse; // response timeout
    firestep::ArduinoUSB usb;
protected:
	std::string readLine(std::istream &is);
public:
    FireStepClient(bool prompt=true, const char *serialPath=FIRESTEP_SERIAL_PATH, int32_t msResponse=10*1000);
	static std::string version(bool verbose=true);
    int reset();
    int console();
    int sendJson(std::string json);
} FireStepClient;

}

#endif

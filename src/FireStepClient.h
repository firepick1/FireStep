#ifndef FIRESTEPCLIENT_H
#define FIRESTEPCLIENT_H

#include "ArduinoUSB.h"
#include "FireStep.h"

namespace firestep {

typedef class FireStepClient {
private:
    IFireStep *pFireStep;
    bool prompt;
protected:
	std::string readLine(std::istream &is);
public:
    FireStepClient(IFireStep *pFireStep, bool prompt=true);
	static std::string version(bool verbose=true);
    int reset();
    int console();
    int sendJson(std::string json);
} FireStepClient;

}

#endif


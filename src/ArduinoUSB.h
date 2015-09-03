#ifndef ARDUINOUSB_H
#define ARDUINOUSB_H

#include <sys/types.h>
#include <fstream>
#include <string>

#define SERIAL_PATH "/dev/ttyACM0"
#define FIRESTEP_STTY "0:4:cbe:0:3:1c:7f:15:4:0:0:0:11:13:1a:0:12:f:17:16:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0"

namespace firestep {

typedef class ArduinoUSB {
private:
    int fd;
	int resultCode;
	std::string path;
    std::ofstream os;
public:
    ArduinoUSB(const char *path=SERIAL_PATH);
	~ArduinoUSB();
	int init_stty(const char *sttyArgs=FIRESTEP_STTY);
	bool isOpen();
	int open();
	std::string readln(int32_t msTimeout=100);
	void writeln(std::string line);
} ArduinoUSB;

}

#endif

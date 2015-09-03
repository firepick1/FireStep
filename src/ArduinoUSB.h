#ifndef ARDUINOUSB_H
#define ARDUINOUSB_H

#include <sys/types.h>
#include <termios.h>
#include <fstream>
#include <string>

#define SERIAL_PATH "/dev/ttyACM0"
#define FIRESTEP_STTY "0:4:cbe:0:3:1c:7f:15:4:0:0:0:11:13:1a:0:12:f:17:16:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0"

namespace firestep {

typedef class ArduinoUSB {
private:
    int fd;
	int rc;
	struct termios term_save;
    std::ofstream os;
public:
    ArduinoUSB(const char *path=SERIAL_PATH);
	~ArduinoUSB();
	bool configure(int baud=B19200);
	bool isOpen();
	std::string readln(int32_t msTimeout=100);
	void write(std::string line);
} ArduinoUSB;

}

#endif

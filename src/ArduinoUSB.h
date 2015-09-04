#ifndef ARDUINOUSB_H
#define ARDUINOUSB_H

#include <sys/types.h>
#include <fstream>
#include <string>

#define FIRESTEP_SERIAL_PATH "/dev/ttyACM0"
#define FIRESTEP_STTY "0:4:cbe:0:3:1c:7f:15:4:0:0:0:11:13:1a:0:12:f:17:16:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0"
/**
 *	speed 19200 baud; rows 0; columns 0; line = 0;
 *	intr = ^C; quit = ^\; erase = ^?; kill = ^U; eof = ^D; eol = <undef>;
 *	eol2 = <undef>; swtch = <undef>; start = ^Q; stop = ^S; susp = ^Z; rprnt = ^R;
 *	werase = ^W; lnext = ^V; flush = ^O; min = 0; time = 0;
 *	-parenb -parodd cs8 hupcl -cstopb cread clocal -crtscts
 *	-ignbrk -brkint -ignpar -parmrk -inpck -istrip -inlcr -igncr -icrnl -ixon -ixoff
 *	-iuclc -ixany -imaxbel -iutf8
 *	-opost -olcuc -ocrnl onlcr -onocr -onlret -ofill -ofdel nl0 cr0 tab0 bs0 vt0 ff0
 *	-isig -icanon -iexten -echo -echoe -echok -echonl -noflsh -xcase -tostop -echoprt
 *	-echoctl -echoke
*/

namespace firestep {

typedef class ArduinoUSB {
private:
    int fd;
	int resultCode;
	std::string sttyArgs;
	std::string path;
    std::ofstream os;
public:
    ArduinoUSB(const char *path=FIRESTEP_SERIAL_PATH);
	~ArduinoUSB();
	int configure(bool resetOnClose=true);
	bool isOpen();
	int open();
	int close();
	std::string readln(int32_t msTimeout=100);
	void writeln(std::string line);
	void write(std::string line);
} ArduinoUSB;

}

#endif

#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <errno.h>
#include "ArduinoUSB.h"
#include "FireLog.h"
#include "FireUtils.h"
#include "version.h"

using namespace std;
using namespace firestep;

ArduinoUSB::ArduinoUSB(const char *path) 
	: path(path), fd(-1), sttyArgs(FIRESTEP_STTY), resultCode(-1)
{}

ArduinoUSB::~ArduinoUSB() {
	close();
}

int ArduinoUSB::open() {
    resultCode = 0;
    fd = ::open(FIRESTEP_SERIAL_PATH, O_RDONLY | O_NOCTTY | O_NONBLOCK );
    if (fd < 0) {
        cerr << "ERROR	: could not open " << FIRESTEP_SERIAL_PATH << endl;
        resultCode = fd;
	} else {
		LOGINFO1("ArduinoUSB::open(%s) opened for read", path.c_str());
    }
    if (resultCode == 0) {
        os.open(path.c_str());
        if (!os.is_open()) {
            cerr << "ERROR	: could not open " << path << " for output" << endl;
            ::close(fd);
            resultCode = -EIO;
		} else {
			LOGINFO1("ArduinoUSB::open(%s) opened for write", path.c_str());
        }
    } else {
		string msg = "ArduinoUSB::open() could not open ";
		msg += path;
		msg += " for input";
        cerr << "ERROR	: " << msg << endl;
		LOGERROR1("%s", msg.c_str());
	}
	return resultCode;
}

int ArduinoUSB::close() {
	if (isOpen()) {
		resultCode = ::close(fd);
		if (resultCode) {
			LOGERROR2("ArduinoUSB::close(%s) failed:%d", path.c_str(), resultCode);
		} else {
			LOGINFO1("ArduinoUSB::close(%s)", path.c_str());
			fd = -1;
		}
		os.close();
	}
	return resultCode;
}

int ArduinoUSB::configure(bool resetOnClose) {
	struct stat fs;
	int rc = stat(path.c_str(), &fs);
	if (rc) {
		LOGERROR2("ArduinoUSB::configure(%s) could not access serial port:%d", path.c_str(), rc);
		return rc;
	}

	char cmd[255];
	snprintf(cmd, sizeof(cmd), "stty -F %s %s %s", path.c_str(), sttyArgs.c_str(), resetOnClose ? "hup" : "-hup");
	LOGINFO1("ArduinoUSB::configure() %s", cmd);
	rc = system(cmd);
	if (rc == 0) {
		LOGINFO1("ArduinoUSB::configure(%s) initialized serial port", path.c_str());
	} else if (rc == 256) {
		LOGERROR2("ArduinoUSB::configure(%s) failed. Add user to serial group and login again.", path.c_str(), rc);
	} else {
		LOGERROR2("ArduinoUSB::configure(%s) could not initialize serial port. stty=>%d", path.c_str(), rc);
	}

	return rc;
}

bool ArduinoUSB::isOpen() {
    return resultCode == 0 && fd >= 0 ? true : false;
}

string ArduinoUSB::readln(int32_t msTimeout) {
    string line;
    bool isEOL = false;
    int32_t msIdle = millis() + msTimeout;
    char buf[10];
    do {
        int res = read(fd, buf, 1);
        if (res == 1) {
            if (buf[0] == '\r') {
                line += "\n";
                isEOL = true;
            } else if (buf[0] == '\n') {
                // do nothing
            } else {
                line += buf[0];
                msIdle = millis() + msTimeout;
            }
        } else if (res == 0) {
            //cerr << "USB	: zero bytes read" << endl;
        }
        switch (errno) {
        case 0:
            // happiness
            break;
        case -EAGAIN:
            cerr << "USB	: EAGAIN" << endl;
            break;
        default:
            cerr << "USB	: ERR" << res << endl;
			usleep(1000*1000);
            break;
        }
    } while (!isEOL && millis() < msIdle);
    return line;
}

void ArduinoUSB::writeln(string line) {
    os << line << endl;
    os.flush();
}


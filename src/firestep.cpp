#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <unistd.h>

#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <errno.h>
#include "FireLog.h"
#include "FireUtils.h"
#include "version.h"
#include "Arduino.h"
#include "ArduinoUSB.h"

#include "MachineThread.h"
#include "Display.h"
#include "DeltaCalculator.h"
#include "ProgMem.h"

using namespace ph5;
using namespace firestep;
using namespace ArduinoJson;

//#define SERIAL_PATH "/dev/ttyACM0"

#define ASSERTQUAD(expected,actual) ASSERTEQUALS( expected.toString().c_str(), actual.toString().c_str() );

void replaceChar(string &s, char cmatch, char creplace) {
    for (int i = 0; i < s.size(); i++) {
        if (s[i] == cmatch) {
            s[i] = creplace;
        }
    }
}

string jsonTemplate(const char *jsonIn, string replace = "'\"") {
    string ji(jsonIn);
    for (int i = 0; i < replace.size(); i += 2) {
        char cmatch = replace[i];
        char creplace = replace[i + 1];
        replaceChar(ji, cmatch, creplace);
    }
    return ji;
}
#define JT(s) (jsonTemplate(s).c_str())

int help(int rc=0) {
    cerr << "HELP	: firestep command line client v"
         << VERSION_MAJOR <<"."<< VERSION_MINOR <<"."<< VERSION_PATCH << endl;
    return rc;
}

#define FIRESTEP_STTY "0:4:cbe:0:3:1c:7f:15:4:0:0:0:11:13:1a:0:12:f:17:16:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0"
int init_serial(const char *path=SERIAL_PATH) {
	struct stat fs;
	int rc = stat(path, &fs);
	if (rc) {
		LOGERROR2("init_serial(%s) could not access serial port:%d", path, rc);
		return rc;
	}

	char cmd[255];
	snprintf(cmd, sizeof(cmd), "stty -F %s %s", path, FIRESTEP_STTY);
	cout << "EXEC	: " << cmd << endl;
	rc = system(cmd);
	if (rc == 0) {
		LOGINFO1("init_serial(%s) initialized serial port", path);
	} else if (rc == 256) {
		LOGERROR2("init_serial(%s) failed. Add user to serial group and login again.", path, rc);
	} else {
		LOGERROR2("init_serial(%s) could not initialize serial port. stty=>%d", path, rc);
	}

	return rc;
}

#ifdef TBD
typedef class ArduinoUSB {
private:
    int fd;
	int rc;
	struct termios term_save;
    ofstream os;
public:
    ArduinoUSB(const char *path=SERIAL_PATH) {
		rc = 0;
        fd = open(SERIAL_PATH, O_RDONLY | O_NOCTTY | O_NONBLOCK );
        if (fd < 0) {
            cerr << "ERROR	: could not open " << SERIAL_PATH << endl;
			rc = fd;
        } else {
			tcgetattr(fd, &term_save);
		}
		if (rc == 0) {
			os.open(path);
			if (!os.is_open()) {
				cerr << "ERROR	: could not open " << path << " for output" << endl;
				close(fd);
				rc = -EIO;
			}
		}
    }
	~ArduinoUSB(){ 
		if (isOpen()) {
			tcsetattr(fd, TCSANOW, &term_save); // restore
			close(fd);
		}
	}
	bool configure(int baud=B19200) {
		// Save settings and configure serial port
		struct termios term_firestep;
		bzero(&term_firestep, sizeof(term_firestep));
		term_firestep.c_cflag = baud | CRTSCTS | CS8 | CLOCAL | CREAD;
		term_firestep.c_iflag = IGNPAR | ICRNL;
		term_firestep.c_oflag = 0;
		term_firestep.c_lflag = ICANON; // no echo, no signal

		/* See /usr/include/termios.h */
		term_firestep.c_cc[VINTR]    = 0;     /* Ctrl-c */
		term_firestep.c_cc[VQUIT]    = 0;     /* Ctrl-\ */
		term_firestep.c_cc[VERASE]   = 0;     /* del */
		term_firestep.c_cc[VKILL]    = 0;     /* @ */
		term_firestep.c_cc[VEOF]     = 4;     /* Ctrl-d */
		term_firestep.c_cc[VTIME]    = 0;     /* inter-character timer unused */
		term_firestep.c_cc[VMIN]     = 1;     /* blocking read until 1 character arrives */
		term_firestep.c_cc[VSWTC]    = 0;     /* '\0' */
		term_firestep.c_cc[VSTART]   = 0;     /* Ctrl-q */
		term_firestep.c_cc[VSTOP]    = 0;     /* Ctrl-s */
		term_firestep.c_cc[VSUSP]    = 0;     /* Ctrl-z */
		term_firestep.c_cc[VEOL]     = 0;     /* '\0' */
		term_firestep.c_cc[VREPRINT] = 0;     /* Ctrl-r */
		term_firestep.c_cc[VDISCARD] = 0;     /* Ctrl-u */
		term_firestep.c_cc[VWERASE]  = 0;     /* Ctrl-w */
		term_firestep.c_cc[VLNEXT]   = 0;     /* Ctrl-v */
		term_firestep.c_cc[VEOL2]    = 0;     /* '\0' */

		tcflush(fd, TCIFLUSH); // clear data
		tcsetattr(fd, TCSANOW, &term_firestep); // apply new
	}
	bool isOpen() {
		return rc == 0 ? true : false;
	}
	string readln(int32_t msTimeout=100) {
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
				break;
			}
		} while (!isEOL && millis() < msIdle);
		return line;
	}
	void write(string line) {
		os << line;
		os.flush();
	}
} ArduinoUSB;
#endif

string readLine(istream &serial, int32_t msTimeout=100) {
    string line;
    bool isDone = false;
    long msIdle = millis() + msTimeout;
    do {
        int c = serial.peek();
        if (c != EOF) {
            msIdle = millis() + msTimeout;
        }
        switch (c) {
        case '\r':
            serial.get(); // ignore
            break;
        case '\n':
            line += (char) serial.get();
            isDone = true;
            break;
        case EOF:
            serial.clear();
            break;
        case 0:
            cout << "NULL" << endl;
            break;
        default:
            line += (char) serial.get();
            break;
        }
    } while (!isDone && millis() < msIdle);
    return line;
}

int pipe_io(const char *serialPath=SERIAL_PATH, int32_t msResponse=3*1000) {
	ArduinoUSB usb(serialPath);
	if (!usb.isOpen()) {
        cerr << "ERROR	: could not open " << serialPath << " for input" << endl;
        return -EIO;
    }
    cerr << "READING..." << endl;

    string ignore = usb.readln(5000);
    while (ignore.size()) {
        cerr << "IGNORE	: " << ignore;
        ignore = usb.readln();
    }
    for (;;) {
        cerr << "CMD	: ";
        string request = readLine(cin);
        if (request == "quit\n") {
            cerr << "OK	: quit" << endl;
            break;
        } else if (request.size() > 0) {
            cerr << "WRITE	: " << request ;
            ASSERT(string::npos != request.find("\n"));
			usb.write(request);
        } else {
            // cin should block and this code never runs
            cerr << "IDLE	: ..." << endl;
            usleep(1000*1000);
        }
        string response = usb.readln(msResponse);
        for (int i=0; response.size()==0 && i<3; i++) {
            cerr << "STATUS	: waiting " << msResponse << "ms..." << endl;
            //response = readLine(is, msResponse);
            response = usb.readln(msResponse);
        }
        if (response.size() > 0) {
            cout << response;
        } else {
            cerr << "ERROR	: timeout" << endl;
            break;
        }
    }

    cout << "END	: closing " << SERIAL_PATH << endl;
}


int parse_args(int argc, char *argv[]) {
    int rc = 0;
    bool logging = false;
    int logLvl = FIRELOG_INFO;
    for (int iArg=0; iArg<argc; iArg++) {
        if (strcmp("-h", argv[iArg])==0 || strcmp("--help", argv[iArg])==0) {
            return help();
        } else if (strcmp("--logerror", argv[iArg])==0) {
            logging = true;
            logLvl = FIRELOG_ERROR;
        } else if (strcmp("--logwarn", argv[iArg])==0) {
            logging = true;
            logLvl = FIRELOG_WARN;
        } else if (strcmp("--loginfo", argv[iArg])==0) {
            logging = true;
            logLvl = FIRELOG_INFO;
        } else if (strcmp("--logdebug", argv[iArg])==0) {
            logging = true;
            logLvl = FIRELOG_DEBUG;
        } else if (strcmp("--logtrace", argv[iArg])==0) {
            logging = true;
            logLvl = FIRELOG_TRACE;
        }
    }
    if (logging) {
        firelog_init("firestep.log", logLvl);
    }
    return rc;
}

int main(int argc, char *argv[]) {
    LOGINFO3("INFO	: firestep command line client v%d.%d.%d",
             VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
    int rc = parse_args(argc, argv);

    init_serial();
    pipe_io();
//	serial_terminal();

    return rc;
}

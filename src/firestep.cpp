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

int terminal(bool prompt=true, const char *serialPath=SERIAL_PATH, int32_t msResponse=10*1000) {
	ArduinoUSB usb(serialPath);
	if (prompt) {
		char ver[100];
		snprintf(ver, sizeof(ver), "firestep v%d.%d.%d", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH); 
		cerr << "STATUS	; " << ver << " serial console" << endl;
		cerr << "STATUS	: initializing serial port..." << endl;
	}
	usb.init_stty();
	if (!usb.open()) {
		string msg = "ERROR	: could not open ";
		msg += serialPath;
		msg += " for input";
        cerr << msg << endl;
		LOGERROR1("terminal() %s", msg.c_str());
        return -EIO;
    }

    string ignore = usb.readln(5000);
    while (ignore.size()) {
		if (prompt) {
			cerr << "START	: " << ignore;
		}
        ignore = usb.readln();
    }
    for (;;) {
		if (prompt) {
			cerr << "CMD	: ";
		}
		string request;
		cin >> request;
        if (request == "quit") {
			if (prompt) {
				cerr << "OK	: quit" << endl;
			}
			LOGDEBUG("terminal() quit");
            break;
        } else if (request.size() > 0) {
			usb.writeln(request);
			LOGDEBUG2("terminal() bytes:%ld write:%s", (long) request.size(), request.c_str());
        } else { // EOF for file stdin
			if (prompt) {
				cerr << "EOF" << endl;
			}
			LOGDEBUG("terminal() EOF");
			break;
        }
        string response = usb.readln(msResponse);
        for (int i=0; response.size()==0 && i<4; i++) {
			if (prompt) {
				cerr << "STATUS	: wait " << msResponse << "ms..." << endl;
			}
			LOGDEBUG1("terminal() wait %ldms", (long) msResponse);
            response = usb.readln(msResponse);
        }
        if (response.size() > 0) {
            cout << response;
			LOGDEBUG1("terminal() read:%s", response.c_str());
        } else {
			if (prompt) {
				cerr << "ERROR	: timeout" << endl;
			}
			LOGERROR1("terminal(%s) timeout", serialPath);
            break;
        }
    }

	if (prompt) {
		cout << "END	: closing " << serialPath << endl;
	}
	LOGINFO1("terminal() closing %s", serialPath);
}

int parse_args(int argc, char *argv[], bool &prompt) {
    int rc = 0;
    bool logging = false;
    int logLvl = FIRELOG_INFO;
    for (int iArg=0; iArg<argc; iArg++) {
        if (strcmp("-h", argv[iArg])==0 || strcmp("--help", argv[iArg])==0) {
            return help();
        } else if (strcmp("-p",argv[iArg])==0 || strcmp("--prompt", argv[iArg])==0) {
			prompt = true;
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
	bool prompt = false;
    int rc = parse_args(argc, argv, prompt);

    terminal(prompt);

    return rc;
}

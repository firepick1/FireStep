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

using namespace firestep;

int help(int rc=0) {
    cerr << "HELP	: firestep command line client v"
         << VERSION_MAJOR <<"."<< VERSION_MINOR <<"."<< VERSION_PATCH << endl;
    return rc;
}

typedef class FireStepClient {
private:
	string serialPath;
	int32_t msResponse; // response timeout
	ArduinoUSB usb;
public:
	FireStepClient(const char *serialPath=SERIAL_PATH, int32_t msResponse=10*1000);
	int console(bool prompt=true);
} FireStepClient;

FireStepClient::FireStepClient(const char *serialPath, int32_t msResponse) 
	: serialPath(serialPath), msResponse(msResponse), usb(serialPath) 
{
}

int FireStepClient::console(bool prompt) {
	if (prompt) {
		cerr << "START	: initializing serial port..." << endl;
	}
	int rc = usb.init_stty();
	if (rc == 0) {
		rc = usb.open();
	}
	if (rc != 0) {
		return rc;
    }
    string ignore = usb.readln(5000);
    while (ignore.size()) {
		LOGINFO1("console() start:%s", ignore.c_str());
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
			LOGDEBUG("console() quit");
            break;
        } else if (request.size() > 0) {
			usb.writeln(request);
			LOGDEBUG2("console() bytes:%ld write:%s", (long) request.size(), request.c_str());
        } else { // EOF for file stdin
			if (prompt) {
				cerr << "EOF" << endl;
			}
			LOGDEBUG("console() EOF");
			break;
        }
        string response = usb.readln(msResponse);
        for (int i=0; response.size()==0 && i<4; i++) {
			if (prompt) {
				cerr << "STATUS	: wait " << msResponse << "ms..." << endl;
			}
			LOGDEBUG1("console() wait %ldms", (long) msResponse);
            response = usb.readln(msResponse);
        }
        if (response.size() > 0) {
            cout << response;
			LOGDEBUG1("console() read:%s", response.c_str());
        } else {
			if (prompt) {
				cerr << "ERROR	: timeout" << endl;
			}
			LOGERROR("console() timeout");
            break;
        }
    }

	if (prompt) {
		cout << "END	: closing serial port" << endl;
	}
	LOGINFO("console() closing serial port");
}

int parse_args(int argc, char *argv[], bool &prompt, bool &logging, string &json) {
    int rc = 0;
    int logLvl = FIRELOG_INFO;
    for (int iArg=0; iArg<argc; iArg++) {
        if (strcmp("-h", argv[iArg])==0 || strcmp("--help", argv[iArg])==0) {
            return help();
        } else if (strcmp("-e",argv[iArg])==0 || strcmp("--expr", argv[iArg])==0) {
			if (++iArg >= argc) {
				cerr << "ERROR	: expected FireStep JSON expression" << endl;
			}
			json = argv[iArg];
			return help(-EINVAL);
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
	bool logging = false;
	string json;
    int rc = parse_args(argc, argv, prompt, logging, json);

	char ver[100];
	snprintf(ver, sizeof(ver), "firestep v%d.%d.%d", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH); 
	LOGINFO1("%s", ver);
	if (prompt) {
		cerr << "STATUS	: " << ver << " serial console" << endl;
	}

	FireStepClient fsc;
    fsc.console(prompt);

    return rc;
}

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
#include "FireStepClient.h"

using namespace firestep;

int help(int rc=0) {
    cerr << "HELP	: firestep command line client v"
         << VERSION_MAJOR <<"."<< VERSION_MINOR <<"."<< VERSION_PATCH << endl;
	cerr << "HELP	:" << endl;
	cerr << "HELP	: EXAMPLES" << endl;
	cerr << "HELP	:   Launch interactive console with prompt" << endl;
	cerr << "HELP	:     firestep -p" << endl;
	cerr << "HELP	:     firestep --prompt" << endl;
	cerr << "HELP	:   Reset FireStep " << endl;
	cerr << "HELP	:     firestep -r" << endl;
	cerr << "HELP	:     firestep --reset" << endl;
	cerr << "HELP	:   Process single JSON command " << endl;
	cerr << "HELP	:     firestep -e '{\"hom\":\"\"}'"  << endl;
	cerr << "HELP	:     firestep --expr '{\"hom\":\"\"}'"  << endl;
	cerr << "HELP	:   Process one command per line from JSON file" << endl;
	cerr << "HELP	:     firestep < test/multiline.json" << endl;
	cerr << "HELP	:   Show this help text" << endl;
	cerr << "HELP	:     firestep -h" << endl;
	cerr << "HELP	:     firestep --help" << endl;
    return rc;
}

#ifdef tbd
typedef class FireStepClient {
private:
    bool prompt;
    string serialPath;
    int32_t msResponse; // response timeout
    ArduinoUSB usb;
public:
    FireStepClient(bool prompt=true, const char *serialPath=SERIAL_PATH, int32_t msResponse=10*1000);
    int reset();
    int console();
    int sendJson(string json);
} FireStepClient;

FireStepClient::FireStepClient(bool prompt, const char *serialPath, int32_t msResponse)
    : prompt(prompt),serialPath(serialPath), msResponse(msResponse), usb(serialPath)
{
}

int FireStepClient::reset() {
    int rc = 0;
    if (usb.isOpen()) {
        rc = usb.close();
    }
    if (rc == 0) {
        rc = usb.configure(true);
    }
    if (rc != 0) {
        cerr << "ERROR	: could not configure serial port " << serialPath << endl;
        LOGERROR("FireStepClient::reset(hup) failed");
        return rc;
    }

    rc = usb.open();
    if (rc==0) { // clear out startup text
        string ignore = usb.readln(5000);
        while (ignore.size()) {
            LOGINFO1("FireStepClient::reset() start:%s", ignore.c_str());
            if (prompt) {
                cerr << "START	: " << ignore;
            }
            ignore = usb.readln();
        }
    }

    if (rc == 0) {
        usb.close();
    }
    if (rc == 0) {
        rc = usb.configure(false);
    }
    if (rc != 0) {
        cerr << "ERROR	: could not configure serial port " << serialPath << endl;
        LOGERROR("FireStepClient::reset(nohup) failed");
        return rc;
	}
	return rc;
}

int FireStepClient::sendJson(string request) {
    int rc = usb.open();
    if (rc != 0) {
        return rc;
    }

    usb.writeln(request);
    LOGDEBUG2("sendJson() bytes:%ld write:%s", (long) request.size(), request.c_str());

    string response = usb.readln(msResponse);
    for (int i=0; response.size()==0 && i<4; i++) {
        LOGDEBUG1("sendJson() wait %ldms", (long) msResponse);
        response = usb.readln(msResponse);
    }
    if (response.size() > 0) {
        cout << response;
        LOGDEBUG1("sendJson() read:%s", response.c_str());
    } else {
        LOGERROR("sendJson() timeout");
        return -ETIME;
    }

    return usb.close();
}

int FireStepClient::console() {
    int rc = usb.open();
    if (rc != 0) {
        return rc;
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

    return usb.close();
}
#endif

int parse_args(int argc, char *argv[], bool &prompt, bool &logging, string &json, bool& reset) {
    int rc = 0;
    int logLvl = FIRELOG_INFO;
    for (int iArg=0; iArg<argc; iArg++) {
        if (strcmp("-h", argv[iArg])==0 || strcmp("--help", argv[iArg])==0) {
            return help();
        } else if (strcmp("-e",argv[iArg])==0 || strcmp("--expr", argv[iArg])==0) {
            if (++iArg >= argc) {
                cerr << "ERROR	: expected FireStep JSON expression" << endl;
                return help(-EINVAL);
            }
            json = argv[iArg];
        } else if (strcmp("-r",argv[iArg])==0 || strcmp("--reset", argv[iArg])==0) {
            reset = true;
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
    char ver[100];
    snprintf(ver, sizeof(ver), "firestep command line client v%d.%d.%d",
             VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
    LOGINFO1("%s", ver);

    bool prompt = false;
    bool reset = false;
    bool logging = false;
    string json;
    int rc = parse_args(argc, argv, prompt, logging, json, reset);
    if (rc != 0) {
        return rc;
    }

    if (prompt) {
        cerr << "STATUS	: " << ver << endl;
    }

    FireStepClient fsc(prompt);
    if (rc == 0) {
		if (reset) {
			rc = fsc.reset();
		} else if (json.size()) {
            rc = fsc.sendJson(json.c_str());
        } else {
            rc = fsc.console();
        }
    }

	return rc;
}

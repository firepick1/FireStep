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
#include "FireStepClient.h"

using namespace std;
using namespace firestep;

FireStepClient::FireStepClient(bool prompt, const char *serialPath, int32_t msResponse)
    : prompt(prompt),serialPath(serialPath), msResponse(msResponse), usb(serialPath)
{
	LOGINFO1("%s", version().c_str());
}

int FireStepClient::reset() {
	ArduinoUSB usb(serialPath.c_str());
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
	LOGINFO("FireStepClient::reset() complete");
	return rc;
}

string FireStepClient::version(bool verbose) {
    char ver[100];
	if (verbose) {
		snprintf(ver, sizeof(ver), "firestep command line client v%d.%d.%d",
				 VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
	} else {
		snprintf(ver, sizeof(ver), "v%02d.%02d.%02d",
				 VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
	}
	return string(ver);
}

int FireStepClient::sendJson(string request) {
    int rc = usb.open();
    if (rc != 0) {
        return rc;
    }

    usb.writeln(request);
    LOGINFO2("sendJson() bytes:%ld write:%s", (long) request.size(), request.c_str());

    string response = usb.readln(msResponse);
    for (int i=0; response.size()==0 && i<4; i++) {
        LOGDEBUG1("sendJson() wait %ldms", (long) msResponse);
        response = usb.readln(msResponse);
    }
    if (response.size() > 0) {
        cout << response;
        LOGINFO1("sendJson() read:%s", response.c_str());
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
            LOGINFO2("console() bytes:%ld write:%s", (long) request.size(), request.c_str());
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
            LOGINFO1("console() read:%s", response.c_str());
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


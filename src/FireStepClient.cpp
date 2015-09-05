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
#include "FireStepClient.h"
#include "FireLog.h"
#include "FireUtils.h"
#include "version.h"

using namespace std;
using namespace firestep;

FireStepClient::FireStepClient(bool prompt, const char *serialPath, int32_t msResponse)
    : prompt(prompt),serialPath(serialPath), msResponse(msResponse), usb(serialPath)
{
	cin.unsetf(ios_base::skipws);
	LOGINFO1("%s", version().c_str());
}

FireStepClient::~FireStepClient() {
}

//////////////// IFireStep implementation ///////////////////////

int FireStepClient::startup() {
    int rc = usb.open();
	ready = rc == 0 ? true : false;
	return rc;
}

int FireStepClient::shutdown() {
	return usb.close();
}

string FireStepClient::executeCore(string &json) {
    if (!ready) {
        return osError(-EPROTO);
    }

	string response;
	int rc = send(json, response);
	if (rc != 0) {
		return osError(rc);
	}

	return response;
}

////////////////// serial implementation ///////////

string FireStepClient::readLine(istream &is) {
	string line;
	bool isEOL = false;
	while (!isEOL) {
		int c = is.get();
		if (c == EOF) {
			break;
		}
		switch (c) {
		case '\r':
			// ignore CR and expect LF
			break;
		case '\n':
			line += (char)c;
			isEOL = true;
			break;
		default:
			line += (char)c;
			break;
		}
	}
	return line;
}

int FireStepClient::reset() {
    int rc = 0;
    if (usb.isOpen()) {
		const char *msg ="FireStepClient::reset() sending LF to interrupt FireStep";
		cerr << "RESET	: " << msg << endl;
		LOGINFO1("%s", msg);
		usb.writeln(); // interrupt any current processing
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
    if (rc == 0) {
        rc = usb.close(); // hup
    }

    if (rc == 0) {
        rc = usb.configure(false);
    }
    if (rc != 0) {
        cerr << "ERROR	: could not configure serial port " << serialPath << endl;
        LOGERROR("FireStepClient::reset(nohup) failed");
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
        rc = usb.close(); // nohup
    }
	LOGINFO("FireStepClient::reset() complete");
	return rc;
}

string FireStepClient::version(bool verbose) {
    char ver[100];
	if (verbose) {
		snprintf(ver, sizeof(ver), "firestep - command line client v%.4f",
				 VERSION_MAJOR + VERSION_MINOR/100.0 + VERSION_PATCH/10000.0);
	} else {
		snprintf(ver, sizeof(ver), "v%.4f",
				 VERSION_MAJOR + VERSION_MINOR/100.0 + VERSION_PATCH/10000.0);
	}
	return string(ver);
}

int FireStepClient::send(std::string request, std::string &response) {
    if (!ready) {
        return -EPROTO;
    }

    usb.writeln(request);
    LOGINFO2("FireStepClient::send() bytes:%ld write:%s", (long) request.size(), request.c_str());

    response = usb.readln(msResponse);
    for (int i=0; response.size()==0 && i<4; i++) {
        LOGDEBUG1("FireStepClient::send() wait %ldms", (long) msResponse);
        response = usb.readln(msResponse);
    }
    if (response.size() > 0) {
        LOGINFO2("FireStepClient::send() bytes:%ld read:%s", (long) response.size(), response.c_str());
    } else {
        LOGERROR("FireStepClient::send() timeout");
        return -ETIME;
    }

    return usb.close();
}

int FireStepClient::sendJson(string request) {
	int rc = startup();
	if (rc != 0) {
		return rc;
	}
	string response;
	rc = send(request, response);
	if (rc != 0) {
		return rc;
	}
	cout << response;

	return shutdown();
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
        string request = readLine(cin);
		//cerr << "DEBUG: cin:"<< (cin.eof() ? "EOF" : "...") << " request:" << request.size() << endl;
        if (request.size() > 0) {
            usb.write(request);
            LOGINFO2("FireStepClient::console() bytes:%ld write:%s", (long) request.size(), request.c_str());
        } else { // EOF for file stdin
            if (prompt) {
                cerr << "EOF" << endl;
            }
            LOGINFO("FireStepClient::console() EOF");
            break;
        }
        string response = usb.readln(msResponse);
        for (int i=0; response.size()==0 && i<4; i++) {
            if (prompt) {
                cerr << "STATUS	: wait " << msResponse << "ms..." << endl;
            }
            LOGDEBUG1("FireStepClient::console() wait %ldms", (long) msResponse);
            response = usb.readln(msResponse);
        }
        if (response.size() > 0) {
            cout << response;
            LOGINFO2("FireStepClient::console() bytes:%ld read:%s", (long) response.size(), response.c_str());
        } else {
            if (prompt) {
                cerr << "ERROR	: timeout" << endl;
            }
            LOGERROR("FireStepClient::console() timeout");
            break;
        }
    }

    if (prompt) {
        cout << "END	: closing serial port" << endl;
    }
    LOGINFO("FireStepClient::console() closing serial port");

    return usb.close();
}


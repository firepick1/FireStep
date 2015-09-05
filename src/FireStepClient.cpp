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

//////////////// FireStepSerial ///////////////////////

FireStepSerial::FireStepSerial(const char *serialPath, int32_t msResponse)
    : serialPath(serialPath), msResponse(msResponse), usb(serialPath)
{
}

FireStepSerial::~FireStepSerial() {
}

int FireStepSerial::open() {
	ready = false;
	if (usb.open() != 0) {
		return STATUS_USB_OPEN;
	}

	ready = true;
	return 0;
}

int FireStepSerial::close() {
	if (usb.close() != 0) {
		return STATUS_USB_CLOSE;
	}
	return 0;
}

int FireStepSerial::executeCore(const string &jsonRequest, string &jsonResponse) {
	return send(jsonRequest, jsonResponse);
}

int FireStepSerial::send(std::string request, std::string &response) {
    if (!ready) {
        return errorResponse(STATUS_OPEN, response);
    }

    usb.writeln(request);
    LOGINFO2("FireStepSerial::send() bytes:%ld write:%s", (long) request.size(), request.c_str());

    response = usb.readln(msResponse);
    for (int i=0; response.size()==0 && i<4; i++) {
        LOGDEBUG1("FireStepSerial::send() wait %ldms", (long) msResponse);
        response = usb.readln(msResponse);
    }
    if (response.size() > 0) {
        LOGINFO2("FireStepSerial::send() bytes:%ld read:%s", (long) response.size(), response.c_str());
    } else {
        LOGERROR("FireStepSerial::send() timeout");
        return STATUS_USB_TIMEOUT;
    }
	return 0;
}

int FireStepSerial::reset() {
    if (usb.isOpen()) {
		const char *msg ="FireStepSerial::reset() sending LF to interrupt FireStep";
		cerr << "RESET	: " << msg << endl;
		LOGINFO1("%s", msg);
		usb.writeln(); // interrupt any current processing
		if (usb.close() != 0) { 
			msg = "FireStepSerial::reset() could not close serial port ";
			cerr << "ERROR	: " << msg << serialPath << endl;
			LOGERROR2("%s %s", msg, serialPath.c_str());
			return STATUS_USB_CLOSE;
		}
    }
	int rc = usb.configure(true);
    if (rc != 0) {
		const char *msg = "FireStepSerial::reset() could not configure serial port for hup";
		cerr << "ERROR	: " << msg << serialPath << endl;
		LOGERROR2("FireStepSerial::reset() %s %s", msg, serialPath.c_str());
        return STATUS_USB_CONFIGURE;
    }
	if (usb.open() != 0) {
		const char *msg = "FireStepSerial::reset() could not open serial port ";
		cerr << "ERROR	: " << msg << serialPath << endl;
		LOGERROR2("FireStepSerial::reset() %s %s", msg, serialPath.c_str());
		return STATUS_USB_OPEN;
	}
	if (usb.close() != 0) {
		const char *msg = "FireStepSerial::reset() could not close serial port ";
		cerr << "ERROR	: " << msg << serialPath << endl;
		LOGERROR2("FireStepSerial::reset() %s %s", msg, serialPath.c_str());
		return STATUS_USB_CLOSE;
    }

	if (usb.configure(false) != 0) {
		const char *msg = "FireStepSerial::reset() could not configure serial port for nohup";
		cerr << "ERROR	: " << msg << serialPath << endl;
		LOGERROR2("FireStepSerial::reset() %s %s", msg, serialPath.c_str());
        return STATUS_USB_CONFIGURE;
    }

	if (usb.open() != 0) {
		const char *msg = "FireStepSerial::reset() could not open serial port ";
		cerr << "ERROR	: " << msg << serialPath << endl;
		LOGERROR2("FireStepSerial::reset() %s %s", msg, serialPath.c_str());
		return STATUS_USB_OPEN;
	}
	
    // clear out startup text
	string ignore = usb.readln(5000);
	while (ignore.size()) {
		LOGINFO1("FireStepSerial::reset() ignore:%s", ignore.c_str());
		ignore = usb.readln();
	}

	if (usb.close() != 0) {
		const char *msg = "FireStepSerial::reset() could not close serial port ";
		cerr << "ERROR	: " << msg << serialPath << endl;
		return STATUS_USB_CLOSE;
    }

	LOGINFO("FireStepClient::reset() complete");

	return 0;
}

////////////////// FireStepClient ///////////

FireStepClient::FireStepClient(IFireStep *pFireStep, bool prompt)
    : pFireStep(pFireStep), prompt(prompt)
{
	if (pFireStep == NULL) {
		LOGERROR("FireStepClient(NULL) expected IFireStep");
		return;
	}
	cin.unsetf(ios_base::skipws);
	LOGINFO1("%s", version().c_str());
}

FireStepClient::~FireStepClient() {
}

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
	if (pFireStep == NULL) {
		LOGERROR("FireStepClient::reset() expected IFireStep");
		return -EINVAL;
	}
	return pFireStep->reset();
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

int FireStepClient::sendJson(string request) {
	if (pFireStep == NULL) {
		LOGERROR("FireStepClient::sendJson() expected IFireStep");
		return STATUS_IFIRESTEP;
	}

	int rc = pFireStep->open();
	if (rc != 0) {
		return rc;
	}
	
	string response;
	rc = pFireStep->execute(request, response);
	cout << response;
	if (rc != 0) {
		return rc;
	}

	return pFireStep->close();
}

int FireStepClient::console() {
	if (pFireStep == NULL) {
		LOGERROR("FireStepClient::console() expected IFireStep");
		return STATUS_IFIRESTEP;
	}

	int rc = pFireStep->open();
	if (rc != 0) {
		return rc;
	}

    while (rc == 0) {
        if (prompt) {
            cerr << "CMD	: ";
        }
        string request = readLine(cin);
		string response;
		rc = pFireStep->execute(request, response);
    }

    if (prompt) {
        cerr << "END	: closing serial port" << endl;
    }
    LOGINFO("FireStepClient::console() closing serial port");

    return pFireStep->close();
}


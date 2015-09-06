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

FireStepClient::FireStepClient(IFireStep *pFireStep, bool prompt)
    : pFireStep(pFireStep), prompt(prompt)
{
	ASSERTNONZERO(pFireStep);
	cin.unsetf(ios_base::skipws);
	LOGINFO1("%s", version().c_str());
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
    int rc = pFireStep->open();
    if (rc != 0) {
        return rc;
    }

	if (request.size() && request[request.size()-1] != '\n') {
		request += '\n';
	}
	string response;
	rc = pFireStep->execute(request, response);
	if (rc != 0) {
		if (prompt) {
			cerr << "ERROR	: " << rc << endl;
		}
		return rc;
	}
	cout << response;

    return pFireStep->close();
}

int FireStepClient::console() {
    int rc = pFireStep->open();
    if (rc != 0) {
        return rc;
    }

    for (;;) {
        if (prompt) {
            cerr << "CMD	: ";
        }
        string request = readLine(cin);
        if (request.size() == 0) { // EOF
            if (prompt) {
                cerr << "EOF" << endl;
            }
            LOGINFO("FireStepClient::console() EOF");
            break;
        }
		string response;
		rc = pFireStep->execute(request, response);
		if (rc != 0) {
			break;
		}
        if (response.size() == 0) {
            if (prompt) {
                cerr << "ERROR	: timeout" << endl;
            }
            break;
        }
		cout << response;
    }

    if (prompt) {
        cout << "END	: closing serial port" << endl;
    }
    LOGINFO("FireStepClient::console() closing serial port");

	if (rc == 0) {
		rc = pFireStep->close();
	}
    return rc;
}


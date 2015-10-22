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
    getline(is, line);
    if ( (is.rdstate() & std::ifstream::failbit ) != 0 ) {
        throw "console read failed";
    }
    if ( (is.rdstate() & std::ifstream::eofbit ) != 0 ) {
        cerr << "END\t: CTRL=D";
    } else if (line.compare("quit") == 0) {
        line = ""; // EOF
    } else {
        line.append("\n");
    }
    //cerr << line.size() << " bytes read" << endl;
        
	return line;
}

int FireStepClient::reset() {
	return pFireStep->reset();
}

string FireStepClient::version(bool verbose) {
    char ver[100];
	if (verbose) {
		snprintf(ver, sizeof(ver), "firestep - command line client v%.3f",
				 VERSION_MAJOR + VERSION_MINOR/100.0 + VERSION_PATCH/1000.0);
	} else {
		snprintf(ver, sizeof(ver), "v%.3f",
				 VERSION_MAJOR + VERSION_MINOR/100.0 + VERSION_PATCH/1000.0);
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
    //cerr << "INFO\t: isatty:" << (isatty(fileno(stdin)) ? "true" : "false") << endl;
    cerr << "INFO\t: Enter \"quit\" to exit" << endl;

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
			cerr << "ERROR	: timeout" << endl;
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


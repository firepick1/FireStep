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
#include "FireStepSerial.h"

using namespace std;
using namespace firestep;

FireStepSerial::FireStepSerial(const char *serialPath, int32_t msResponse) 
    : serialPath(serialPath), msResponse(msResponse), usb(serialPath)
{
}

FireStepSerial::~FireStepSerial() {
	close();
}

int FireStepSerial::executeCore(const std::string &request, std::string &response) {
	int retries = 8;
	int rc = 0;
	usb.write(request);
	response = usb.readln(msResponse);
	if (request.compare("\n") == 0) { // simple LF should be echoed
		for (int i=0; response.size() == 0 && i<retries; i++) {
			LOGDEBUG1("FireStepClient::console() wait %ldms", (long) msResponse);
			response = usb.readln(msResponse);
		}
	} else { // JSON response should end with }-SPACE-LF
		uint32_t nRead = 0;
		for (int i=0; response.find("} \n")==string::npos && (nRead != response.size() || i<retries); i++) {
			nRead = response.size();
			LOGDEBUG1("FireStepClient::console() wait %ldms", (long) msResponse);
			response += usb.readln(msResponse);
		}
	}

	return rc;
}

int FireStepSerial::reset() {
    int rc = 0;
    if (isOpen()) {
		const char *msg ="FireStepSerial::reset() sending LF to interrupt FireStep";
		cerr << "RESET	: " << msg << endl;
		LOGINFO1("%s", msg);
		usb.writeln(); // interrupt any current processing
        rc = close();
    }
    if (rc == 0) {
		LOGINFO("FireStepSerial::reset() configure(hup)");
        rc = usb.configure(true);
    }
    if (rc != 0) {
		const char *msg = "could not configure serial port (hup) ";
        cerr << "ERROR	: " << msg << serialPath << endl;
		LOGERROR2("%s%s", msg, serialPath.c_str());
        return rc;
    }
    rc = open();
    if (rc == 0) {
		LOGINFO("FireStepSerial::reset() configure(nohup)");
        rc = close(); // hup reset happens here
    }

    if (rc == 0) {
        rc = usb.configure(false);
    }
    if (rc != 0) {
		const char *msg = "could not configure serial port (nohup) ";
        cerr << "ERROR	: " << msg << serialPath << endl;
		LOGERROR2("%s%s", msg, serialPath.c_str());
        return rc;
	}
    rc = open(); 
    if (rc == 0) { // clear out startup text
        string ignore = usb.readln(5000);
		cerr << "RESET	: re-open " << serialPath << endl;
        while (ignore.size()) {
            LOGINFO1("FireStepSerial::reset() ignored:%ldB", (long) ignore.size());
            LOGDEBUG1("FireStepSerial::reset() ignore:%s", ignore.c_str());
			cerr << ignore ; 
            ignore = usb.readln();
        }
        rc = close(); // nohup
    }
	LOGINFO("FireStepSerial::reset() complete");
	return rc;
}

int FireStepSerial::open() {
    if (!isOpen() && usb.open() != 0) {
		const char *msg = "FireStepSerial::open() failed:";
		cerr << "ERROR	: " << msg << serialPath << endl;
		LOGERROR2("%s%s", msg, serialPath.c_str());
		return STATUS_USB_OPEN;
	}
	return IFireStep::open();
}

int FireStepSerial::close() {
    if (isOpen() && usb.close() != 0) {
		const char *msg = "FireStepSerial::close() failed:";
		cerr << "ERROR	: " << msg << serialPath << endl;
		LOGERROR2("%s%s", msg, serialPath.c_str());
        return STATUS_USB_CLOSE;
    }
    return IFireStep::close();
}


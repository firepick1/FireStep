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
    cerr << "HELP	: " << FireStepClient::version() << endl;
	cerr << "HELP	:" << endl;
	cerr << "HELP	: EXAMPLES" << endl;
	cerr << "HELP	:   Launch interactive console with prompt" << endl;
	cerr << "HELP	:     firestep -p" << endl;
	cerr << "HELP	:     firestep --prompt" << endl;
	cerr << "HELP	:   Reset FireStep, ignore all commands and return -EAGAIN" << endl;
	cerr << "HELP	:     firestep -r" << endl;
	cerr << "HELP	:     firestep --reset" << endl;
	cerr << "HELP	:   Specify serial device (default is /dev/ttyACM0)" << endl;
	cerr << "HELP	:     firestep -d /dev/ttyACM0 "  << endl;
	cerr << "HELP	:     firestep -device /dev/ttyACM0 "  << endl;
	cerr << "HELP	:   Process single JSON command " << endl;
	cerr << "HELP	:     firestep -e '{\"hom\":\"\"}'"  << endl;
	cerr << "HELP	:     firestep --expr '{\"hom\":\"\"}'"  << endl;
	cerr << "HELP	:   Process one command per line from JSON file" << endl;
	cerr << "HELP	:     firestep < test/multiline.json" << endl;
	cerr << "HELP	:   Show concise version, ignore all commands and return -EAGAIN" << endl;
	cerr << "HELP	:     firestep -v" << endl;
	cerr << "HELP	:     firestep --version" << endl;
	cerr << "HELP	:   Enable logging at given message threshold to firestep.log" << endl;
	cerr << "HELP	:     firestep --logerror" << endl;
	cerr << "HELP	:     firestep --logwarn" << endl;
	cerr << "HELP	:     firestep --loginfo" << endl;
	cerr << "HELP	:     firestep --logdebug" << endl;
	cerr << "HELP	:     firestep --logtrace" << endl;
	cerr << "HELP	:   Show this help text" << endl;
	cerr << "HELP	:     firestep -h" << endl;
	cerr << "HELP	:     firestep --help" << endl;
	cerr << "HELP	: " << endl;
    cerr << "HELP	: DESCRIPTION" << endl;
    cerr << "HELP	:   Establish serial connection to FireStep and provide" << endl;
    cerr << "HELP	:   extended Firestep commands via command line interface." << endl;
    cerr << "HELP	:   Commands can be executed individually, from a file, " << endl;
    cerr << "HELP	:   or from an optionally prompted interactive session." << endl;
	cerr << "HELP	: " << endl;
	cerr << "HELP	: SEE ALSO" << endl;
	cerr << "HELP	:   https://github.com/firepick1/FireStep/wiki/firestep-command-line" << endl;
    return rc;
}

int parse_args(int argc, char *argv[], bool &prompt, bool &logging, 
	string &json, bool& reset, string& device) 
{
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
        } else if (strcmp("-v",argv[iArg])==0 || strcmp("--version", argv[iArg])==0) {
			cerr << FireStepClient::version(false) << endl;
			return -EAGAIN;
        } else if (strcmp("-d",argv[iArg])==0 || strcmp("--device", argv[iArg])==0) {
            if (++iArg >= argc) {
                cerr << "ERROR	: expected serial path " << endl;
                return help(-EINVAL);
            }
			device = argv[iArg];
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
    bool prompt = false;
    bool reset = false;
    bool logging = false;
    string json;
	string device = FIRESTEP_SERIAL_PATH;
    int rc = parse_args(argc, argv, prompt, logging, json, reset, device);
    if (rc != 0) {
        return rc;
    }

	FireStepClient fsc(prompt, device.c_str());

    if (prompt) {
        cerr << "STATUS	: " << FireStepClient::version() << endl;
    }

	if (reset) {
		rc = fsc.reset();
		rc = -EAGAIN; // for some reason we need a new process to clear buffer
	} 
    if (rc == 0) {
		if (json.size()) {
            rc = fsc.sendJson(json.c_str());
        } else {
            rc = fsc.console();
        }
    }

	return rc;
}

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
	cerr << "HELP	:   Show concise version " << endl;
	cerr << "HELP	:     firestep -v" << endl;
	cerr << "HELP	:     firestep --version" << endl;
    return rc;
}

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
        } else if (strcmp("-v",argv[iArg])==0 || strcmp("--version", argv[iArg])==0) {
			cerr << FireStepClient::version(false) << endl;
			return -EAGAIN;
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
    int rc = parse_args(argc, argv, prompt, logging, json, reset);
    if (rc != 0) {
        return rc;
    }

	FireStepClient fsc(prompt);

    if (prompt) {
        cerr << "STATUS	: " << FireStepClient::version() << endl;
    }

    if (rc == 0) {
		if (reset) {
			rc = fsc.reset();
		} 
		
		if (json.size()) {
            rc = fsc.sendJson(json.c_str());
        } else {
            rc = fsc.console();
        }
    }

	return rc;
}

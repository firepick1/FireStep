#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <errno.h>
#include "ArduinoUSB.h"
#include "FireLog.h"
#include "FireUtils.h"
#include "version.h"

using namespace std;
using namespace firestep;

ArduinoUSB::ArduinoUSB(const char *path) {
    rc = 0;
    fd = open(SERIAL_PATH, O_RDONLY | O_NOCTTY | O_NONBLOCK );
    if (fd < 0) {
        cerr << "ERROR	: could not open " << SERIAL_PATH << endl;
        rc = fd;
    } else {
        tcgetattr(fd, &term_save);
    }
    if (rc == 0) {
        os.open(path);
        if (!os.is_open()) {
            cerr << "ERROR	: could not open " << path << " for output" << endl;
            close(fd);
            rc = -EIO;
        }
    }
}

ArduinoUSB::~ArduinoUSB() {
    if (isOpen()) {
        tcsetattr(fd, TCSANOW, &term_save); // restore
        close(fd);
    }
}

bool ArduinoUSB::configure(int baud) {
    // Save settings and configure serial port
    struct termios term_firestep;
    bzero(&term_firestep, sizeof(term_firestep));
    term_firestep.c_cflag = baud | CRTSCTS | CS8 | CLOCAL | CREAD;
    term_firestep.c_iflag = IGNPAR | ICRNL;
    term_firestep.c_oflag = 0;
    term_firestep.c_lflag = ICANON; // no echo, no signal

    /* See /usr/include/termios.h */
    term_firestep.c_cc[VINTR]    = 0;     /* Ctrl-c */
    term_firestep.c_cc[VQUIT]    = 0;     /* Ctrl-\ */
    term_firestep.c_cc[VERASE]   = 0;     /* del */
    term_firestep.c_cc[VKILL]    = 0;     /* @ */
    term_firestep.c_cc[VEOF]     = 4;     /* Ctrl-d */
    term_firestep.c_cc[VTIME]    = 0;     /* inter-character timer unused */
    term_firestep.c_cc[VMIN]     = 1;     /* blocking read until 1 character arrives */
    term_firestep.c_cc[VSWTC]    = 0;     /* '\0' */
    term_firestep.c_cc[VSTART]   = 0;     /* Ctrl-q */
    term_firestep.c_cc[VSTOP]    = 0;     /* Ctrl-s */
    term_firestep.c_cc[VSUSP]    = 0;     /* Ctrl-z */
    term_firestep.c_cc[VEOL]     = 0;     /* '\0' */
    term_firestep.c_cc[VREPRINT] = 0;     /* Ctrl-r */
    term_firestep.c_cc[VDISCARD] = 0;     /* Ctrl-u */
    term_firestep.c_cc[VWERASE]  = 0;     /* Ctrl-w */
    term_firestep.c_cc[VLNEXT]   = 0;     /* Ctrl-v */
    term_firestep.c_cc[VEOL2]    = 0;     /* '\0' */

    tcflush(fd, TCIFLUSH); // clear data
    tcsetattr(fd, TCSANOW, &term_firestep); // apply new
}

bool ArduinoUSB::isOpen() {
    return rc == 0 ? true : false;
}

string ArduinoUSB::readln(int32_t msTimeout) {
    string line;
    bool isEOL = false;
    int32_t msIdle = millis() + msTimeout;
    char buf[10];
    do {
        int res = read(fd, buf, 1);
        if (res == 1) {
            if (buf[0] == '\r') {
                line += "\n";
                isEOL = true;
            } else if (buf[0] == '\n') {
                // do nothing
            } else {
                line += buf[0];
                msIdle = millis() + msTimeout;
            }
        } else if (res == 0) {
            //cerr << "USB	: zero bytes read" << endl;
        }
        switch (errno) {
        case 0:
            // happiness
            break;
        case -EAGAIN:
            cerr << "USB	: EAGAIN" << endl;
            break;
        default:
            cerr << "USB	: ERR" << res << endl;
            break;
        }
    } while (!isEOL && millis() < msIdle);
    return line;
}
void ArduinoUSB::write(string line) {
    os << line;
    os.flush();
}


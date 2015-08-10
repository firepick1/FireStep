#include "ProgMem.h"

#ifndef ARDUINO
#define PROGMEM
#define strlen_P(src) strlen(src)
#define pgm_read_byte_near(src) (*(const char *)(src))
#endif

using namespace firestep;

const char src_help[] PROGMEM = {
	"["
	"{\"msg\":\"Program names are:\"},"
	"{\"msg\":\"  help  print this help text\"},"
	"{\"msg\":\"  test  print \"test\"},"
	"{\"msg\":\"  cal   calibrate Z-bowl error and Z-bed plane\"}"
	"]"
};

const char src_test1[] PROGMEM = {
	"{\"msg\":\"test A\"}"
};

const char src_test2[] PROGMEM = {
	"["
	"{\"msg\":\"test A\"},"
	"{\"msg\":\"test B\"}"
	"]"
};

const char src_calibrate[] PROGMEM = {
	"["
	"{\"hom\":\"\"},"
	"{\"prbz\":\"\"},"
	"{\"movrz\":10},"
	"{\"mrkwp\":1},"
	"{\"mov\":{ \"zm\":3,\"a\":0,\"d\":50}},"
	"{\"prbz\":\"\"},"
	"{\"mov\":{\"zm\":3,\"a\":60,\"d\":50}},"
	"{\"prbz\":\"\"},"
	"{\"mov\":{\"zm\":3,\"a\":120,\"d\":50}},"
	"{\"prbz\":\"\"},"
	"{\"mov\":{\"zm\":3,\"a\":180,\"d\":50}},"
	"{\"prbz\":\"\"},"
	"{\"mov\":{\"zm\":3,\"a\":240,\"d\":50}},"
	"{\"prbz\":\"\"},"
	"{\"mov\":{\"zm\":3,\"a\":300,\"d\":50}},"
	"{\"prbz\":\"\"},"
	"{\"mov\":{\"zm\":3,\"x\":0,\"y\":0}},"
	"{\"prbz\":\"\"},"
	"{\"movwp\":1},"
	"{\"prbd\":\"\",\"cal\":\"\"}"
	"]" 
};

const char *firestep::prog_src(const char *name) {
    if (strcmp("test", name) == 0) {
        return src_test2;
    } else if (strcmp("test1", name) == 0) {
        return src_test1;
    } else if (strcmp("test2", name) == 0) {
        return src_test2;
    } else if (strcmp("help", name) == 0) {
        return src_help;
    } else if (strcmp("cal", name) == 0) {
        return src_calibrate;
    }

	return src_help;
}

Status firestep::prog_dump(const char *name) {
	const char *src = prog_src(name);
	TESTCOUT2("prog_dump:", name, " src:", src);

    for (size_t i = 0; i<MAX_JSON; i++) {
        char c = pgm_read_byte_near(src + i);
		ASSERT(c == 0 || ' ' <= c && c <= '~');
		if (c) {
			Serial.print(c);
		} else {
			Serial.println();
			break;
		}
    }

	return STATUS_OK;
}

Status firestep::prog_load_cmd(const char *name, JsonCommand &jcmd) {
	string key(name); // name is volatile
	Status status = STATUS_OK;
    const char *src = prog_src(key.c_str());
	TESTCOUT2("prog_load:", key.c_str(), " src:", src);

    size_t len = strlen_P(src);
	if (len <= 0 || MAX_JSON <= len+1) {
		return STATUS_PROGRAM_SIZE;
	}

	///////// DANGER /////////
	// We will replace the currently running command with the program
	// name will no longer be valid
	jcmd.clear();
	///////// DANGER /////////
	
	char *buf = jcmd.allocate(len+1);
	ASSERT(buf);
    for (size_t i = 0; i < len; i++) {
        buf[i] = pgm_read_byte_near(src + i);
		ASSERT(' ' <= buf[i] && buf[i] <= '~');
    }
	buf[len] = 0;
	TESTCOUT3("prog_load_cmd:", key.c_str(), " buf:", buf, " status:", status);
	if (status != STATUS_OK) {
		TESTCOUT1("prog_load_cmd:", status);
		return status;
	}

	status = jcmd.parse(buf, STATUS_WAIT_IDLE);
	if (status < 0) {
		TESTCOUT2("prog_load_cmd:", key.c_str(), " parse error:", status); // should never happen
	} else {
		TESTCOUT2("prog_load_cmd:", key.c_str(), " parse status:", status); // STATUS_BUSY_PARSED 10
	}

	return status;
}


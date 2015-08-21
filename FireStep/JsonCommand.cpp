#include "Arduino.h"
#ifdef CMAKE
#include <cstring>
#include <cstdio>
#endif
#include "version.h"
#include "JsonCommand.h"
#include "ProgMem.h"

using namespace firestep;

JsonCommand::JsonCommand() {
    clear();
}

size_t JsonCommand::responseCapacity() {
    return jbResponse.capacity();
}

size_t JsonCommand::requestCapacity() {
    return jbRequest.capacity();
}

size_t JsonCommand::responseAvailable() {
    return jbResponse.capacity() - jbResponse.size();
}

void JsonCommand::addQueryAttr(JsonObject& node, const char *key) {
	char *buf = JsonCommand::allocate(strlen_P(key)+1);
	if (buf) {
		strcpy_P(buf, key);
		node[buf] = "";
	} else {
		TESTCOUT1("ERROR	: addQueryAttr OUT OF MEMORY ##############", key);
		DIE();
	}
}

void JsonCommand::responseClear() {
    jbResponse.clear();
    jResponseRoot = jbResponse.createObject();
    jResponseRoot["s"] = STATUS_EMPTY;
    jResponseRoot.asObject().createNestedObject("r");
}

size_t JsonCommand::requestAvailable() {
    return jbRequest.capacity() - jbRequest.size();
}

void JsonCommand::clear() {
    TESTCOUT1("JsonCommand::", "clear");
    parsed = false;
    cmdIndex = 0;
    memset(json, 0, sizeof(json));
    memset(error, 0, sizeof(error));
    pJsonFree = json;
    jbRequest.clear();
    responseClear();
}

const char * JsonCommand::getError() {
    return error;
}

Status JsonCommand::setError(Status status, const char *err) {
    snprintf(error, sizeof(error), "%s", err);
    jResponseRoot["s"] = status;
    jResponseRoot["e"] = error;
    return status;
}

Status JsonCommand::parseCore() {
    if (*json == 0) {
        return STATUS_WAIT_IDLE;	// empty command
    }
    JsonObject &jobj = jbRequest.parseObject(json);
    parsed = true;
    jRequestRoot = "?";
    if (jobj.success()) {
        int kids = 0;
        for (JsonObject::iterator it = jobj.begin(); it != jobj.end(); ++it) {
            if (!it->value.success()) {
                return STATUS_JSON_PARSE_ERROR;
            }
            kids++;
        }
        if (kids < 1) {
            return STATUS_JSON_MEM4;
        }
        jRequestRoot = jobj;
        jResponseRoot["r"] = jRequestRoot;
        TESTCOUT1("parseCore:", "object");
    } else {
        JsonArray &jarr = jbRequest.parseArray(json);
        if (!jarr.success()) {
            jResponseRoot["r"] = "?";
            if (requestAvailable() < 10) {
                //TESTCOUT1("requestAvailable:", requestAvailable());
                return STATUS_JSON_MEM1;
            }
            return STATUS_JSON_PARSE_ERROR;
        }
        jRequestRoot = jarr;
        jResponseRoot["r"] = jarr[0];
        TESTCOUT1("parseCore:", "array");
    }
    jResponseRoot["s"] = STATUS_BUSY_PARSED;

    return STATUS_BUSY_PARSED;
}

char * JsonCommand::allocate(size_t length) {
    if ((pJsonFree-json) + length > sizeof(json)) {
        return NULL;
    }
    char * result = pJsonFree;
    pJsonFree += length;
    return result;
}

Status JsonCommand::parseInput(const char *jsonIn, Status status) {
    //TESTCOUT2("parseInput:", (int) (jsonIn ? jsonIn[0] : 911), " parsed:", parsed);
    if (parsed) {
        return STATUS_BUSY_PARSED;
    }
    if (jsonIn && (jsonIn < json || json+MAX_JSON < jsonIn)) {
		size_t len = strlen(jsonIn)+1;
		char *buf = allocate(len);
		parsed = true;
		if (!buf) {
            return STATUS_JSON_TOO_LONG;
        }
		snprintf(buf, len, "%s", jsonIn);
        return parseCore();
    } else if (pJsonFree == json || status == STATUS_WAIT_EOL) {
        while (Serial.available()) {
            if (pJsonFree - json >= MAX_JSON - 1) {
                parsed = true;
                return STATUS_JSON_TOO_LONG;
            }
            char c = Serial.read();
            if (c == '\n') {
                return parseCore();
            } else {
                *pJsonFree++ = c;
            }
        }
        return STATUS_WAIT_EOL;
    } else {
        return parseCore();
    }
}

/**
 * Parse the JSON provided or a JSON line read from Serial
 * Return true if parsing is complete.
 * Check isValid() and getStatus() for parsing status.
 */
Status JsonCommand::parse(const char *jsonIn, Status statusIn) {
    //TESTCOUT1("parse:", (int) (jsonIn ? jsonIn[0] : 911));
    tStart = ticks();
    //TESTCOUT1("parse:", (int) (jsonIn ? jsonIn[0] : 911));
    Status status = parseInput(jsonIn, statusIn);

    if (status < 0) {
        char error[100];
        snprintf(error, sizeof(error), "{\"s\":%d}", status);
        Serial.println(error);
    }
    return status;
}

bool JsonCommand::isValid() {
    return parsed && jRequestRoot.success();
}


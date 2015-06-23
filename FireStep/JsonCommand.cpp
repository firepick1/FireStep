#include "Arduino.h"
#ifdef CMAKE
#include <cstring>
#include <cstdio>
#endif
#include "version.h"
#include "JsonCommand.h"

using namespace firestep;

JsonCommand::JsonCommand() {
    clear();
}

size_t JsonCommand::responseAvailable() {
    return jbResponse.capacity() - jbResponse.size();
}

size_t JsonCommand::requestAvailable() {
    return jbRequest.capacity() - jbRequest.size();
}

void JsonCommand::clear() {
    parsed = false;
	cmdIndex = 0;
    memset(json, 0, sizeof(json));
    memset(error, 0, sizeof(error));
    pJsonFree = json;
    jbRequest.clear();
    jbResponse.clear();
    jResponseRoot = jbResponse.createObject();
    jResponseRoot["s"] = STATUS_EMPTY;
    jResponseRoot.asObject().createNestedObject("r");
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
            return STATUS_JSON_MEM;
        }
        jRequestRoot = jobj;
		jResponseRoot["r"] = jRequestRoot;
    } else {
		JsonArray &jarr = jbRequest.parseArray(json);
		if (!jarr.success()) {
			jResponseRoot["r"] = "?";
			return STATUS_JSON_PARSE_ERROR;
		}
        jRequestRoot = jarr;
		jResponseRoot["r"] = jarr[0];
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
    if (parsed) {
        return STATUS_BUSY_PARSED;
    }
    if (jsonIn) {
        snprintf(json, sizeof(json), "%s", jsonIn);
        if (strcmp(jsonIn, json) != 0) {
            parsed = true;
            return STATUS_JSON_TOO_LONG;
        }
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
    tStart = ticks();
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


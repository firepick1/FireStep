#include "Arduino.h"
#include <cstring>
#include <cstdio>
#include "JCommand.h"

using namespace firestep;

JCommand::JCommand() {
    parsed = false;
    memset(json, 0, sizeof(json));
    pJsonFree = json;
	jsonResp = jsonBuffer.createObject();
	jsonResp["s"] = STATUS_EMPTY;
}

bool JCommand::parseCore() {
    jsonRoot = jsonBuffer.parseObject(json);
    if (jsonRoot.success()) {
        jsonResp["s"] = STATUS_JSON_PARSED;
		jsonResp["r"] = jsonRoot;
    } else {
        jsonResp["s"] = STATUS_JSON_SYNTAX_ERROR;
    }
	parsed = true;

    return true;
}

/**
 * Parse the JSON provided or a JSON line read from Serial
 * Return true if parsing is complete.
 * Check isValid() and getStatus() for parsing status.
 */
bool JCommand::parse(const char *jsonIn) {
    if (parsed) {
        return true;
    }
    if (jsonIn) {
        snprintf(json, sizeof(json), "%s", jsonIn);
        if (strcmp(jsonIn, json) != 0) {
            parsed = true;
            jsonResp["s"] = STATUS_JSON_TOO_LONG;
            return true;
        }
        return parseCore();
    }
    while (Serial.available()) {
        if (pJsonFree - json >= MAX_JSON - 1) {
            parsed = true;
            jsonResp["s"] = STATUS_JSON_TOO_LONG;
            return true;
        }
        char c = Serial.read();
        if (c == '\n') {
            return parseCore();
        } else {
            *pJsonFree++ = c;
        }
    }
    return false;
}

bool JCommand::isValid() {
    return parsed && jsonRoot.success();
}



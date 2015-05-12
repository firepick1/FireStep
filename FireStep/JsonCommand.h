#ifndef JSONCOMMAND_H
#define JSONCOMMAND_H

#include "version.h"
#include "Arduino.h"
#include "Thread.h"
#ifdef TEST
#include "ArduinoJson.h"
#else
#include <ArduinoJson.h>
#endif
#include "Status.h"
#include "Machine.h"

namespace firestep {

#define MAX_JSON 2024
#if defined(TEST)
#define JSON_REQUEST_BUFFER JSON_OBJECT_SIZE(25)
#else
#define JSON_REQUEST_BUFFER JSON_OBJECT_SIZE(150)
#endif
#define JSON_RESPONSE_BUFFER 200

typedef class JsonCommand {
    private:
		bool parsed;
        char json[MAX_JSON];
		char *pJsonFree;
        StaticJsonBuffer<JSON_REQUEST_BUFFER> jbRequest;
        StaticJsonBuffer<JSON_RESPONSE_BUFFER> jbResponse;
		JsonVariant jRequestRoot;
		JsonVariant jResponseRoot;
		char error[8];

	private:
		Status parseCore();
    public:
        JsonCommand();
		void clear();
		inline JsonVariant& requestRoot() { return jRequestRoot; }
		inline JsonObject & response() { return jResponseRoot; }
        Status parse(const char *jsonIn=NULL);
		bool isValid();
		inline Status getStatus() { return (Status) (long) jResponseRoot["s"]; }
		inline void setStatus(Status status) { jResponseRoot["s"] = status; }
		const char *getError();
		Status setError(Status status, const char *err);
		size_t requestAvailable();
		size_t responseAvailable();
} JsonCommand;

} // namespace firestep

#endif

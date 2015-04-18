#ifndef JCOMMAND_H
#define JCOMMAND_H

#include <string>
#include "ArduinoJson.h"

#define MAX_JSON 1024
#define JSON_BUFFER 1024

namespace firestep {

enum Status {
	STATUS_JSON_PARSED = -1,		// json parsed
	STATUS_EMPTY = -2,				// uninitialized JCommand
	STATUS_OK = 0,					// generic acknowledgement
	STATUS_ERROR = 1,				// generic error
	STATUS_COMPLETED = 4,			// operation is complete
	STATUS_JSON_SYNTAX_ERROR = 111,	// JSON input string is not well formed
	STATUS_JSON_TOO_LONG = 113,		// JSON exceeds buffer size
};

typedef class JCommand {
    private:
		bool parsed;
        char json[MAX_JSON];
		char *pJsonFree;
        StaticJsonBuffer<JSON_BUFFER> jsonBuffer;
		JsonVariant jsonRoot;
		JsonVariant jsonResp;

	private:
		bool parseCore();
    public:
        JCommand();
	public:
		inline JsonObject& createJsonObject() { return jsonBuffer.createObject(); }
	public:
		inline JsonVariant& root() { return jsonRoot; }
	public:
		inline JsonObject & response() { return jsonResp; }
    public:
        bool parse(const char *jsonIn=NULL);
	public:
		bool isValid();
	public:
		Status getStatus() { return (Status) (long) jsonResp["s"]; }
} JCommand;

} // namespace firestep

#endif

#ifndef JCOMMAND_H
#define JCOMMAND_H

#include <string>
#include "ArduinoJson.h"

#define MAX_JSON 1024
#define JSON_BUFFER 1024

namespace firestep {

enum Status {
	STATUS_OK = 0,					// operation completed successfully
	STATUS_JSON_PARSED = 1,			// json parsed, awaiting processing
	STATUS_EMPTY = -1,				// uninitialized JCommand
	STATUS_UNRECOGNIZED_NAME = -3,	// parse did'nt recognize thecommand
	STATUS_JSON_SYNTAX_ERROR = -4,	// JSON input string is not well formed
	STATUS_JSON_TOO_LONG = -5,		// JSON exceeds buffer size
	STATUS_FIELD_ERROR = -6,		// Internal error: could not process field
	STATUS_POSITION_ERROR = -7,		// Internal error: could not process position
	STATUS_AXIS_ERROR = -8,			// Internal error: could not process axis
	STATUS_SYS_ERROR = -9,			// Internal error: could not process system configuration
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
	public:
		void setStatus(Status status) { jsonResp["s"] = status; }
} JCommand;

} // namespace firestep

#endif

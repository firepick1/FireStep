#ifndef JSONCONTROLLER_H
#define JSONCONTROLLER_H

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include "FireLog.h"
#include "FireUtils.hpp"
#include "version.h"
#include "Arduino.h"
#include "SerialTypes.h"
#include "Thread.h"
#include "ArduinoJson.h"
#include "Machine.h"

namespace firestep {

#define MAX_JSON 1024
#define JSON_BUFFER 1024

enum Status {
	STATUS_OK = 0,					// operation completed successfully
	STATUS_JSON_PARSED = 1,			// json parsed, awaiting processing
	STATUS_PROCESSING = 2,			// json parsed, processing in progress
	STATUS_EMPTY = -1,				// uninitialized JsonCommand
	STATUS_JSON_BRACE_ERROR = -2,	// unbalanced JSON braces
	STATUS_JSON_BRACKET_ERROR = -3,	// unbalanced JSON braces
	STATUS_UNRECOGNIZED_NAME = -4,	// parse didn't recognize thecommand
	STATUS_JSON_SYNTAX_ERROR = -5,	// JSON input string is not well formed
	STATUS_JSON_TOO_LONG = -6,		// JSON exceeds buffer size
	STATUS_JSON_STROKE_ERROR = -20,	// Expected JSON object for stroke
	STATUS_S1_RANGE_ERROR = -21,	// stroke segment s1 value out of range [-127,127] 
	STATUS_S2_RANGE_ERROR = -22,	// stroke segment s2 value out of range [-127,127] 
	STATUS_S3_RANGE_ERROR = -23,	// stroke segment s3 value out of range [-127,127] 
	STATUS_S4_RANGE_ERROR = -24,	// stroke segment s4 value out of range [-127,127] 
	STATUS_S1S2LEN_ERROR = -25,		// stroke segment s1/s2 length mismatch
	STATUS_S1S3LEN_ERROR = -26,		// stroke segment s1/s3 length mismatch
	STATUS_S1S4LEN_ERROR = -27,		// stroke segment s1/s4 length mismatch
	STATUS_STROKE_NULL_ERROR = -28,	// stroke has no segments
	STATUS_POSITION_ERROR = -100,	// Internal error: could not process position
	STATUS_AXIS_ERROR = -101,		// Internal error: could not process axis
	STATUS_SYS_ERROR = -102,		// Internal error: could not process system configuration
	STATUS_S1_ERROR = -103,			// Internal error: could not process segment 
	STATUS_S2_ERROR = -104,			// Internal error: could not process segment 
	STATUS_S3_ERROR = -105,			// Internal error: could not process segment 
	STATUS_S4_ERROR = -106,			// Internal error: could not process segment 
	STATUS_FIELD_ERROR = -107,		// Internal error: could not process field
	STATUS_FIELD_RANGE_ERROR = -108,// Provided field value is out of range
	STATUS_FIELD_ARRAY_ERROR = -109,// Expected JSON field array value
	STATUS_FIELD_REQUIRED = -110,	// Expected JSON field value
	STATUS_JSON_ARRAY_LEN = -111,	// JSON array is too short
};

typedef class JsonCommand {
    private:
		bool parsed;
        char json[MAX_JSON];
		char *pJsonFree;
        StaticJsonBuffer<JSON_BUFFER> jsonBuffer;
		JsonVariant jsonRoot;
		JsonVariant jsonResp;
		char error[8];

	private:
		bool parseCore();
    public:
        JsonCommand();
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
		inline Status getStatus() { return (Status) (long) jsonResp["s"]; }
	public:
		inline void setStatus(Status status) { jsonResp["s"] = status; }
	public:
		const char *getError();
	public:
		void setError(const char *err);
} JsonCommand;

typedef class JsonController {
    private:
		Machine&	machine;
        CLOCK		lastClock;

	protected:
		Status processMachinePosition(JsonCommand &jcmd, JsonObject& jobj, const char* key);
		Status processMotor(JsonCommand &jcmd, JsonObject& jobj, const char* key, char group);
		Status processAxis(JsonCommand &jcmd, JsonObject& jobj, const char* key, char group);
		Status processStroke(JsonCommand &jcmd, JsonObject& jobj, const char* key);
		Status initializeStroke(JsonCommand &jcmd, JsonObject& stroke);
		bool traverseStroke(JsonCommand &jcmd, JsonObject &stroke);

    public:
        JsonController(Machine &machine);
	public: 
		void process(JsonCommand& jcmd);
} JsonController;

} // namespace firestep

#endif

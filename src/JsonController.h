#ifndef JSONCONTROLLER_H
#define JSONCONTROLLER_H

#include <string>
#include "version.h"
#include "Arduino.h"
#include "SerialTypes.h"
#include "Thread.h"
#include "ArduinoJson.h"
#include "Status.h"
#include "Machine.h"

namespace firestep {

#define MAX_JSON 1024
#define JSON_BUFFER 1024

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
        TICKS		clkStart;
		float		plannedTicks;

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

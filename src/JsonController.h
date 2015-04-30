#ifndef JSONCONTROLLER_H
#define JSONCONTROLLER_H

#include <string>
#include "version.h"
#include "Arduino.h"
#include "Thread.h"
#include "ArduinoJson.h"
#include "Status.h"
#include "Machine.h"

namespace firestep {

#define MAX_JSON 2024
#if defined(TEST)
#define JSON_REQUEST_BUFFER JSON_OBJECT_SIZE(20)
#else
#define JSON_REQUEST_BUFFER JSON_OBJECT_SIZE(100)
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
	public:
		//inline JsonObject& createJsonObject() { return jbResponse.createObject(); }
	public:
		inline JsonVariant& requestRoot() { return jRequestRoot; }
	public:
		inline JsonObject & response() { return jResponseRoot; }
    public:
        Status parse(const char *jsonIn=NULL);
	public:
		bool isValid();
	public:
		inline Status getStatus() { return (Status) (long) jResponseRoot["s"]; }
	public:
		inline void setStatus(Status status) { jResponseRoot["s"] = status; }
	public:
		const char *getError();
	public:
		Status setError(Status status, const char *err);
} JsonCommand;

typedef class JsonController {
    private:
		Machine&	machine;
        Ticks		clkStart;
		float		plannedTicks;

	protected:
		Status processStepperPosition(JsonCommand &jcmd, JsonObject& jobj, const char* key);
		Status processMotor(JsonCommand &jcmd, JsonObject& jobj, const char* key, char group);
		Status processAxis(JsonCommand &jcmd, JsonObject& jobj, const char* key, char group);
		Status processStroke(JsonCommand &jcmd, JsonObject& jobj, const char* key);
		Status processSys(JsonCommand& jcmd, JsonObject& jobj, const char* key);
		Status initializeStroke(JsonCommand &jcmd, JsonObject& stroke);
		Status traverseStroke(JsonCommand &jcmd, JsonObject &stroke);

    public:
        JsonController(Machine &machine);
	public: 
		void process(JsonCommand& jcmd);
} JsonController;

} // namespace firestep

#endif

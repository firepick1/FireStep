#include "Arduino.h"
#ifdef CMAKE
#include <cstring>
#include <cstdio>
#endif
#include "version.h"
#include "FPDController.h"
#include "ProcessField.h"

using namespace firestep;

FPDController::FPDController(Machine& machine)
    : JsonController(machine) {
}

Status FPDController::processSys(JsonCommand& jcmd, JsonObject& jobj, const char* key) {
    Status status = STATUS_OK;
    if (strcmp("sys", key) == 0) {
        const char *s;
        if ((s = jobj[key]) && *s == 0) {
            JsonObject& node = jobj.createNestedObject(key);
            node["ah"] = "";
            node["as"] = "";
			node["ch"] = "";
			node["eu"] = "";
            node["fr"] = "";
            node["hp"] = "";
            node["jp"] = "";
            node["lb"] = "";
            node["lh"] = "";
            node["lp"] = "";
            node["mv"] = "";
            node["om"] = "";
            node["pc"] = "";
            node["pi"] = "";
            node["sd"] = "";
            node["tc"] = "";
            node["to"] = "";
            node["tv"] = "";
            node["v"] = "";
        }
        JsonObject& kidObj = jobj[key];
        if (kidObj.success()) {
            for (JsonObject::iterator it = kidObj.begin(); it != kidObj.end(); ++it) {
                status = processSys(jcmd, kidObj, it->key);
                if (status != STATUS_OK) {
                    return status;
                }
            }
        }
    } else if (strcmp("ah", key) == 0 || strcmp("sysah", key) == 0) {
        status = processField<bool, bool>(jobj, key, machine.autoHome);
    } else if (strcmp("as", key) == 0 || strcmp("sysas", key) == 0) {
        status = processField<bool, bool>(jobj, key, machine.autoSync);
    } else if (strcmp("ch", key) == 0 || strcmp("sysch", key) == 0) {
		int32_t curHash = machine.hash();
		int32_t jsonHash = curHash;
		//TESTCOUT3("A curHash:", curHash, " jsonHash:", jsonHash, " jobj[key]:", (int32_t) jobj[key]);
        status = processField<int32_t, int32_t>(jobj, key, jsonHash);
		//TESTCOUT3("B curHash:", curHash, " jsonHash:", jsonHash, " jobj[key]:", (int32_t) jobj[key]);
		if (jsonHash != curHash) {
			machine.syncHash = jsonHash;
		}
    } else if (strcmp("eu", key) == 0 || strcmp("syseu", key) == 0) {
		bool euExisting = machine.isEEUserEnabled();
		bool euNew = euExisting;
        status = processField<bool, bool>(jobj, key, euNew);
		if (euNew != euExisting) {
			machine.enableEEUser(euNew);
		}
    } else if (strcmp("db", key) == 0 || strcmp("sysdb", key) == 0) {
        status = processField<uint8_t, long>(jobj, key, machine.debounce);
    } else if (strcmp("fr", key) == 0 || strcmp("sysfr", key) == 0) {
        leastFreeRam = min(leastFreeRam, freeRam());
        jobj[key] = leastFreeRam;
    } else if (strcmp("hp", key) == 0 || strcmp("syshp", key) == 0) {
        status = processField<int16_t, long>(jobj, key, machine.homingPulses);
    } else if (strcmp("jp", key) == 0 || strcmp("sysjp", key) == 0) {
        status = processField<bool, bool>(jobj, key, machine.jsonPrettyPrint);
    } else if (strcmp("lb", key) == 0 || strcmp("lb", key + 1) == 0) {
        status = processField<StepCoord, int32_t>(jobj, key, machine.latchBackoff);
    } else if (strcmp("lh", key) == 0 || strcmp("syslh", key) == 0) {
        status = processField<bool, bool>(jobj, key, machine.invertLim);
    } else if (strcmp("lp", key) == 0 || strcmp("syslp", key) == 0) {
        status = processField<int32_t, int32_t>(jobj, key, nLoops);
    } else if (strcmp("mv", key) == 0 || strcmp("sysmv", key) == 0) {
        status = processField<int32_t, int32_t>(jobj, key, machine.vMax);
    } else if (strcmp("om", key) == 0 || strcmp("sysom", key) == 0) {
        status = processField<OutputMode, int32_t>(jobj, key, machine.outputMode);
    } else if (strcmp("pc", key) == 0 || strcmp("syspc", key) == 0) {
        PinConfig pc = machine.getPinConfig();
        status = processField<PinConfig, int32_t>(jobj, key, pc);
        const char *s;
        if ((s = jobj.at(key)) && *s == 0) { // query
            // do nothing
        } else {
            machine.setPinConfig(pc);
        }
    } else if (strcmp("pi", key) == 0 || strcmp("syspi", key) == 0) {
        PinType pinStatus = machine.pinStatus;
        status = processField<PinType, int32_t>(jobj, key, pinStatus);
        if (pinStatus != machine.pinStatus) {
            machine.pinStatus = pinStatus;
            machine.pDisplay->setup(pinStatus);
        }
    } else if (strcmp("sd", key) == 0 || strcmp("syssd", key) == 0) {
        status = processField<DelayMics, int32_t>(jobj, key, machine.searchDelay);
    } else if (strcmp("to", key) == 0 || strcmp("systo", key) == 0) {
        Topology value = machine.topology;
        status = processField<Topology, int32_t>(jobj, key, value);
        if (value != machine.topology) {
            machine.topology = value;
            switch (machine.topology) {
            case MTO_RAW:
            default:
                break;
            case MTO_FPD:
                machine.delta.setup();
                if (machine.axis[0].home >= 0 &&
                        machine.axis[1].home >= 0 &&
                        machine.axis[2].home >= 0) {
                    // Delta always has negateve home limit switch
                    Step3D home = machine.delta.getHomePulses();
                    machine.axis[0].position += home.p1-machine.axis[0].home;
                    machine.axis[1].position += home.p2-machine.axis[1].home;
                    machine.axis[2].position += home.p3-machine.axis[2].home;
                    machine.axis[0].home = home.p1;
                    machine.axis[1].home = home.p2;
                    machine.axis[2].home = home.p3;
                }
                break;
            }
        }
    } else if (strcmp("tc", key) == 0 || strcmp("systc", key) == 0) {
        jobj[key] = threadClock.ticks;
    } else if (strcmp("tv", key) == 0 || strcmp("systv", key) == 0) {
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, machine.tvMax);
    } else if (strcmp("v", key) == 0 || strcmp("sysv", key) == 0) {
        jobj[key] = VERSION_MAJOR * 100 + VERSION_MINOR + VERSION_PATCH / 100.0;
    } else {
        return jcmd.setError(STATUS_UNRECOGNIZED_NAME, key);
    }
    return status;
}

Status FPDController::processObj(JsonCommand& jcmd, JsonObject&jobj) {
    JsonVariant node;
    node = jobj;
    Status status = STATUS_OK;

    for (JsonObject::iterator it = jobj.begin(); status >= 0 && it != jobj.end(); ++it) {
        if (strcmp("dvs", it->key) == 0) {
            status = processStroke(jcmd, jobj, it->key);
        } else if (strncmp("mov", it->key, 3) == 0) {
            status = PHMoveTo(machine).process(jcmd, jobj, it->key);
        } else if (strncmp("hom", it->key, 3) == 0) {
            status = processHome(jcmd, jobj, it->key);
        } else if (strncmp("tst", it->key, 3) == 0) {
            status = processTest(jcmd, jobj, it->key);
        } else if (strncmp("sys", it->key, 3) == 0) {
            status = processSys(jcmd, jobj, it->key);
        } else if (strncmp("dpy", it->key, 3) == 0) {
            status = processDisplay(jcmd, jobj, it->key);
        } else if (strncmp("mpo", it->key, 3) == 0) {
            switch (machine.topology) {
            case MTO_RAW:
            default:
                status = processPosition(jcmd, jobj, it->key);
                break;
            case MTO_FPD:
                status = processPosition_MTO_FPD(jcmd, jobj, it->key);
                break;
            }
        } else if (strncmp("io", it->key, 2) == 0) {
            status = processIO(jcmd, jobj, it->key);
        } else if (strncmp("eep", it->key, 3) == 0) {
            status = processEEPROM(jcmd, jobj, it->key);
        } else if (strncmp("dim", it->key, 3) == 0) {
            switch (machine.topology) {
            case MTO_RAW:
            default:
                status = jcmd.setError(STATUS_TOPOLOGY_NAME, it->key);
                break;
            case MTO_FPD:
                status = processDimension_MTO_FPD(jcmd, jobj, it->key);
                break;
            }
        } else if (strncmp("prb", it->key, 3) == 0) {
            switch (machine.topology) {
            case MTO_RAW:
            default:
                status = processProbe(jcmd, jobj, it->key);
                break;
            case MTO_FPD:
                status = processProbe_MTO_FPD(jcmd, jobj, it->key);
                break;
            }
		} else if (strcmp("idl", it->key) == 0) {
			int16_t ms = it->value;
			delay(ms);
		} else if (strcmp("cmt", it->key) == 0) {
			if (OUTPUT_CMT==(machine.outputMode&OUTPUT_CMT)) {
				const char *s = it->value;
				Serial.println(s);
			}
			status = STATUS_OK;
		} else if (strcmp("msg", it->key) == 0) {
			const char *s = it->value;
			Serial.println(s);
			status = STATUS_OK;
        } else {
            switch (it->key[0]) {
            case '1':
            case '2':
            case '3':
            case '4':
                status = processMotor(jcmd, jobj, it->key, it->key[0]);
                break;
            case 'x':
            case 'y':
            case 'z':
            case 'a':
            case 'b':
            case 'c':
                status = processAxis(jcmd, jobj, it->key, it->key[0]);
                break;
            default:
                status = jcmd.setError(STATUS_UNRECOGNIZED_NAME, it->key);
                break;
            }
        }
    }

    return status;
}

Status FPDController::process(JsonCommand& jcmd) {
    Status status = STATUS_OK;
    JsonVariant &jroot = jcmd.requestRoot();

    if (jroot.is<JsonObject&>()) {
        JsonObject& jobj = jroot;
        status = processObj(jcmd, jobj);
    } else if (jroot.is<JsonArray&>()) {
        JsonArray& jarr = jroot;
        if (jcmd.cmdIndex < jarr.size()) {
            JsonObject& jobj = jarr[jcmd.cmdIndex];
            jcmd.jResponseRoot["r"] = jobj;
            status = processObj(jcmd, jobj);
            //TESTCOUT3("FPDController::process(", (int) jcmd.cmdIndex+1,
            //" of ", jarr.size(), ") status:", status);
            if (status == STATUS_OK) {
                bool isLast = jcmd.cmdIndex >= jarr.size()-1;
                if (!isLast && OUTPUT_ARRAYN==(machine.outputMode&OUTPUT_ARRAYN)) {
					jcmd.setTicks();
                    sendResponse(jcmd, status);
                }
                status = STATUS_BUSY_PARSED;
                jcmd.cmdIndex++;
            }
        } else {
            status = STATUS_OK;
        }
    } else {
        status = STATUS_JSON_CMD;
    }

    jcmd.setTicks();
    jcmd.setStatus(status);

    if (!isProcessing(status)) {
        sendResponse(jcmd,status);
    }

    return status;
}

//////////////// MTO_FPD /////////
Status FPDController::processPosition_MTO_FPD(JsonCommand &jcmd, JsonObject& jobj, const char* key) {
    Status status = STATUS_OK;
    const char *axisStr = key + strlen(key) - 1;
    const char *s;
    if (strlen(key) == 3) {
        if ((s = jobj[key]) && *s == 0) {
            JsonObject& node = jobj.createNestedObject(key);
            node["1"] = "";
            node["2"] = "";
            node["3"] = "";
            node["4"] = "";
            node["x"] = "";
            node["y"] = "";
            node["z"] = "";
            if (!node.at("4").success()) {
                return jcmd.setError(STATUS_JSON_KEY, "4");
            }
        }
        JsonObject& kidObj = jobj[key];
        if (!kidObj.success()) {
            return jcmd.setError(STATUS_POSITION_ERROR, key);
        }
        for (JsonObject::iterator it = kidObj.begin(); it != kidObj.end(); ++it) {
            status = processPosition_MTO_FPD(jcmd, kidObj, it->key);
            if (status != STATUS_OK) {
                return status;
            }
        }
    } else if (strcmp("1", axisStr) == 0) {
        status = processField<StepCoord, int32_t>(jobj, key, machine.axis[0].position);
    } else if (strcmp("2", axisStr) == 0) {
        status = processField<StepCoord, int32_t>(jobj, key, machine.axis[1].position);
    } else if (strcmp("3", axisStr) == 0) {
        status = processField<StepCoord, int32_t>(jobj, key, machine.axis[2].position);
    } else if (strcmp("4", axisStr) == 0) {
        status = processField<StepCoord, int32_t>(jobj, key, machine.axis[3].position);
    } else if (strcmp("x", axisStr) == 0) {
        XYZ3D xyz(machine.getXYZ3D());
        if (!xyz.isValid()) {
            return jcmd.setError(STATUS_KINEMATIC_XYZ, key);
        }
        PH5TYPE value = xyz.x;
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, value);
        if (value != xyz.x) {
            status = jcmd.setError(STATUS_OUTPUT_FIELD, key);
        }
    } else if (strcmp("y", axisStr) == 0) {
        XYZ3D xyz(machine.getXYZ3D());
        if (!xyz.isValid()) {
            return jcmd.setError(STATUS_KINEMATIC_XYZ, key);
        }
        PH5TYPE value = xyz.y;
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, value);
        if (value != xyz.y) {
            status = jcmd.setError(STATUS_OUTPUT_FIELD, key);
        }
    } else if (strcmp("z", axisStr) == 0) {
        XYZ3D xyz(machine.getXYZ3D());
        if (!xyz.isValid()) {
            return jcmd.setError(STATUS_KINEMATIC_XYZ, key);
        }
        PH5TYPE value = xyz.z;
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, value);
        if (value != xyz.z) {
            status = jcmd.setError(STATUS_OUTPUT_FIELD, key);
        }
    } else {
        return jcmd.setError(STATUS_UNRECOGNIZED_NAME, key);
    }
    return status;
}

Status FPDController::initializeProbe_MTO_FPD(JsonCommand& jcmd, JsonObject& jobj,
        const char* key, bool clear)
{
    Status status = STATUS_OK;
    OpProbe &probe = machine.op.probe;

    if (clear) {
        Quad<StepCoord> curPos = machine.getMotorPosition();
        probe.setup(curPos);
    }
    Step3D probeEnd(probe.end.value[0], probe.end.value[1], probe.end.value[2]);
    XYZ3D xyzEnd = machine.delta.calcXYZ(probeEnd);
    const char *s;
    if (strcmp("prb", key) == 0) {
        if ((s = jobj[key]) && *s == 0) {
            JsonObject& node = jobj.createNestedObject(key);
            xyzEnd.z = machine.delta.getMinZ(xyzEnd.z, xyzEnd.y);
            node["1"] = "";
            node["2"] = "";
            node["3"] = "";
            node["4"] = "";
            node["ip"] = "";
            node["pn"] = machine.op.probe.pinProbe;
            node["sd"] = "";
            node["x"] = xyzEnd.x;
            node["y"] = xyzEnd.y;
            node["z"] = xyzEnd.z;
        }
        JsonObject& kidObj = jobj[key];
        if (kidObj.success()) {
            for (JsonObject::iterator it = kidObj.begin(); it != kidObj.end(); ++it) {
                status = initializeProbe_MTO_FPD(jcmd, kidObj, it->key, false);
                if (status < 0) {
                    return jcmd.setError(status, it->key);
                }
            }
            if (status == STATUS_BUSY_CALIBRATING && machine.op.probe.pinProbe==NOPIN) {
                return jcmd.setError(STATUS_FIELD_REQUIRED, "pn");
            }
        }
    } else if (strcmp("prbip", key) == 0 || strcmp("ip", key) == 0) {
        status = processField<bool, bool>(jobj, key, machine.op.probe.invertProbe);
    } else if (strcmp("prbpn", key) == 0 || strcmp("pn", key) == 0) {
        status = processField<PinType, int32_t>(jobj, key, machine.op.probe.pinProbe);
    } else if (strcmp("prbsd", key) == 0 || strcmp("sd", key) == 0) {
        status = processField<DelayMics, int32_t>(jobj, key, machine.searchDelay);
    } else if (strcmp("prbx", key) == 0 || strcmp("x", key) == 0) {
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, xyzEnd.x);
    } else if (strcmp("prby", key) == 0 || strcmp("y", key) == 0) {
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, xyzEnd.y);
    } else if (strcmp("prbz", key) == 0 || strcmp("z", key) == 0) {
        machine.op.probe.dataSource = PDS_Z;
        xyzEnd.z = machine.delta.getMinZ(xyzEnd.x, xyzEnd.y);
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, xyzEnd.z);
    } else {
        MotorIndex iMotor = machine.motorOfName(key + (strlen(key) - 1));
        if (iMotor != INDEX_NONE) {
            if ((s = jobj[key]) && *s == 0) {
                // query is fine
            } else {
                return jcmd.setError(STATUS_OUTPUT_FIELD, key);
            }
        } else {
            return jcmd.setError(STATUS_UNRECOGNIZED_NAME, key);
        }
    }

    // This code only works for probes along a single cartesian axis
    Step3D pEnd = machine.delta.calcPulses(xyzEnd);
    machine.op.probe.end.value[0] = pEnd.p1;
    machine.op.probe.end.value[1] = pEnd.p2;
    machine.op.probe.end.value[2] = pEnd.p3;
    TESTCOUT3("pEnd:", pEnd.p1, ", ", pEnd.p2, ", ", pEnd.p3);
    machine.op.probe.maxDelta = 0;
    for (MotorIndex iMotor=0; iMotor<3; iMotor++) {
        StepCoord delta = machine.op.probe.end.value[iMotor] - machine.op.probe.start.value[iMotor];
        machine.op.probe.maxDelta = max((StepCoord)abs(machine.op.probe.maxDelta), delta);
    }

    return status == STATUS_OK ? STATUS_BUSY_CALIBRATING : status;
}

Status FPDController::finalizeProbe_MTO_FPD(JsonCommand& jcmd, JsonObject& jobj, const char* key) {
    XYZ3D xyz = machine.getXYZ3D();
    if (!xyz.isValid()) {
        return jcmd.setError(STATUS_KINEMATIC_XYZ, key);
    }
    if (strcmp("prbx",key) == 0 || strcmp("x",key) == 0) {
        jobj[key] = xyz.x;
    } else if (strcmp("prby",key) == 0 || strcmp("y",key) == 0) {
        jobj[key] = xyz.y;
    } else if (strcmp("prbz",key) == 0 || strcmp("z",key) == 0) {
        jobj[key] = xyz.z;
    } else if (strcmp("1",key) == 0) {
        jobj[key] = machine.getMotorAxis(0).position;
    } else if (strcmp("2",key) == 0) {
        jobj[key] = machine.getMotorAxis(1).position;
    } else if (strcmp("3",key) == 0) {
        jobj[key] = machine.getMotorAxis(2).position;
    } else if (strcmp("4",key) == 0) {
        jobj[key] = machine.getMotorAxis(3).position;
    }
    return STATUS_OK;
}

Status FPDController::processProbe_MTO_FPD(JsonCommand& jcmd, JsonObject& jobj, const char* key) {
    Status status = jcmd.getStatus();
    switch (status) {
    case STATUS_BUSY_PARSED:
        status = initializeProbe_MTO_FPD(jcmd, jobj, key, true);
        break;
    case STATUS_BUSY_OK:
    case STATUS_BUSY_CALIBRATING:
        status = machine.probe(status);
        if (status == STATUS_OK) {
            if (jobj[key].is<JsonObject&>()) {
                JsonObject &kidObj = jobj[key];
                for (JsonObject::iterator it = kidObj.begin(); status == STATUS_OK && it != kidObj.end(); ++it) {
                    status = finalizeProbe_MTO_FPD(jcmd, kidObj, it->key);
                }
            } else {
                status = finalizeProbe_MTO_FPD(jcmd, jobj, key);
            }
        }
        break;
    default:
        ASSERT(false);
        return jcmd.setError(STATUS_STATE, key);
    }
    return status;
}

Status FPDController::processDimension_MTO_FPD(JsonCommand& jcmd, JsonObject& jobj, const char* key) {
    Status status = STATUS_OK;
    if (strcmp("dim", key) == 0) {
        const char *s;
        if ((s = jobj[key]) && *s == 0) {
            JsonObject& node = jobj.createNestedObject(key);
            node["e"] = "";
            node["f"] = "";
            node["gr"] = "";
            node["ha1"] = "";
            node["ha2"] = "";
            node["ha3"] = "";
            node["mi"] = "";
            node["pd"] = "";
            node["re"] = "";
            node["rf"] = "";
            node["st"] = "";
        }
        JsonObject& kidObj = jobj[key];
        if (kidObj.success()) {
            for (JsonObject::iterator it = kidObj.begin(); it != kidObj.end(); ++it) {
                status = processDimension_MTO_FPD(jcmd, kidObj, it->key);
                if (status != STATUS_OK) {
                    return status;
                }
            }
        }
    } else if (strcmp("e", key) == 0 || strcmp("dime", key) == 0) {
        PH5TYPE value = machine.delta.getEffectorLength();
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, value);
        machine.delta.setEffectorLength(value);
    } else if (strcmp("f", key) == 0 || strcmp("dimf", key) == 0) {
        PH5TYPE value = machine.delta.getBaseArmLength();
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, value);
        machine.delta.setBaseArmLength(value);
    } else if (strcmp("gr", key) == 0 || strcmp("dimgr", key) == 0) {
        PH5TYPE value = machine.delta.getGearRatio();
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, value);
        machine.delta.setGearRatio(value);
    } else if (strcmp("ha1", key) == 0 || strcmp("dimha1", key) == 0) {
        Angle3D homeAngles = machine.delta.getHomeAngles();
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, homeAngles.theta1);
        machine.delta.setHomeAngles(homeAngles);
    } else if (strcmp("ha2", key) == 0 || strcmp("dimha2", key) == 0) {
        Angle3D homeAngles = machine.delta.getHomeAngles();
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, homeAngles.theta2);
        machine.delta.setHomeAngles(homeAngles);
    } else if (strcmp("ha3", key) == 0 || strcmp("dimha3", key) == 0) {
        Angle3D homeAngles = machine.delta.getHomeAngles();
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, homeAngles.theta3);
        machine.delta.setHomeAngles(homeAngles);
    } else if (strcmp("mi", key) == 0 || strcmp("dimmi", key) == 0) {
        int16_t value = machine.delta.getMicrosteps();
        status = processField<int16_t, int16_t>(jobj, key, value);
        machine.delta.setMicrosteps(value);
    } else if (strcmp("pd", key) == 0 || strcmp("dimpd", key) == 0) {
        const char *s;
        if ((s = jobj[key]) && *s == 0) {
            JsonArray &jarr = jobj.createNestedArray(key);
            for (int16_t i=0; i<PROBE_DATA; i++) {
                jarr.add(machine.op.probe.probeData[i]);
            }
        } else {
            status = jcmd.setError(STATUS_OUTPUT_FIELD, key);
        }
    } else if (strcmp("re", key) == 0 || strcmp("dimre", key) == 0) {
        PH5TYPE value = machine.delta.getEffectorTriangleSide();
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, value);
        machine.delta.setEffectorTriangleSide(value);
    } else if (strcmp("rf", key) == 0 || strcmp("dimrf", key) == 0) {
        PH5TYPE value = machine.delta.getBaseTriangleSide();
        status = processField<PH5TYPE, PH5TYPE>(jobj, key, value);
        machine.delta.setBaseTriangleSide(value);
    } else if (strcmp("st", key) == 0 || strcmp("dimst", key) == 0) {
        int16_t value = machine.delta.getSteps360();
        status = processField<int16_t, int16_t>(jobj, key, value);
        machine.delta.setSteps360(value);
    } else {
        return jcmd.setError(STATUS_UNRECOGNIZED_NAME, key);
    }
    return status;
}


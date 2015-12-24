#include <Arduino.h>
#ifdef CMAKE
#include <cstring>
#include <cstdio>
#endif
#include "ProcessField.h"

using namespace firestep;

namespace firestep {
template<>
Status processField<int32_t, int32_t>(JsonObject& jobj, const char *key, int32_t& field, bool *pUpdated) {
    Status status = STATUS_OK;
    const char *s;
    if ((s = jobj[key]) && *s == 0) { // query
        status = (jobj[key] = field).success() ? status : STATUS_FIELD_ERROR;
        if (pUpdated) {
            *pUpdated = false;
        }
    } else {
        field = jobj[key];
        if (pUpdated) {
            *pUpdated = true;
        }
    }
    return status;
}
}

Status firestep::processProbeField(Machine& machine, MotorIndex iMotor, JsonCommand &jcmd, JsonObject &jobj, const char *key) {
    Status status = processField<StepCoord, int32_t>(jobj, key, machine.op.probe.end.value[iMotor]);
    if (status == STATUS_OK) {
        Axis &a = machine.getMotorAxis(iMotor);
        if (!a.isEnabled()) {
            return jcmd.setError(STATUS_AXIS_DISABLED, key);
        }
        StepCoord delta = absval(machine.op.probe.end.value[iMotor] - a.position);
        machine.op.probe.maxDelta = maxval(machine.op.probe.maxDelta, delta);
    }
    return status;
}

Status firestep::processHomeField(Machine& machine, AxisIndex iAxis, JsonCommand &jcmd, JsonObject &jobj, const char *key) {
    Axis &a = machine.axis[iAxis];
    StepCoord value = machine.axis[iAxis].home;
    Status status = processField<StepCoord, int32_t>(jobj, key, value);
    if (a.isEnabled()) {
        machine.axis[iAxis].home = value;
        if (a.pinMin == NOPIN) {
            jobj[key] = a.position = a.home = value;
            a.homing = false;
        } else {
            jobj[key] = a.position = a.home = value;
            a.homing = true;
        }
    } else if (value == machine.axis[iAxis].home) {
        jobj[key] = a.home;
        a.homing = false;
    } else {
        return jcmd.setError(STATUS_AXIS_DISABLED, key);
    }

    return status;
}


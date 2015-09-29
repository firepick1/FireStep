#ifndef PROCESSFIELD_H
#define PROCESSFIELD_H

#include "JsonController.h"

namespace firestep {

template<class TF, class TJ>
Status processField(JsonObject& jobj, const char* key, TF& field, bool *pUpdated=NULL) {
    Status status = STATUS_OK;
    const char *s;
    if ((s = jobj[key]) && *s == 0) { // query
        status = (jobj[key] = (TJ) field).success() ? status : STATUS_FIELD_ERROR;
        if (pUpdated) {
            *pUpdated = false;
        }
    } else {
        TJ tjValue = jobj[key];
        double value = tjValue;
        TF tfValue = (TF) value;
        field = tfValue;
        if (pUpdated) {
            *pUpdated = true;
        }
        float diff = absval(tfValue - tjValue);
        if (diff > 1e-7) {
            TESTCOUT3("STATUS_VALUE_RANGE tfValue:", tfValue, " tjValue:", tjValue, " diff:", diff);
            return STATUS_VALUE_RANGE;
        }
        jobj[key] = (TJ) field;
    }
    return status;
}
template Status processField<int16_t, int32_t>(JsonObject& jobj, const char *key, int16_t& field, bool *pUpdated=NULL);
template Status processField<uint16_t, int32_t>(JsonObject& jobj, const char *key, uint16_t& field, bool *pUpdated=NULL);
template Status processField<uint8_t, int32_t>(JsonObject& jobj, const char *key, uint8_t& field, bool *pUpdated=NULL);
template Status processField<PH5TYPE, PH5TYPE>(JsonObject& jobj, const char *key, PH5TYPE& field, bool *pUpdated=NULL);
template Status processField<bool, bool>(JsonObject& jobj, const char *key, bool& field, bool *pUpdated=NULL);

Status processProbeField(Machine& machine, MotorIndex iMotor, JsonCommand &jcmd, JsonObject &jobj, const char *key);
Status processHomeField(Machine& machine, AxisIndex iAxis, JsonCommand &jcmd, JsonObject &jobj, const char *key);

} // namespace firestep
#endif

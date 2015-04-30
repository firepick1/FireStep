#ifndef STATUS_H
#define STATUS_H

namespace firestep {

enum Status {
	STATUS_OK = 0,					// operation completed successfully
	STATUS_JSON_PARSED = 1,			// json parsed, awaiting processing
	STATUS_PROCESSING = 2,			// json parsed, processing in progress
	STATUS_SERIAL_EOL_WAIT = 3,		// waiting for EOL on Serial
	STATUS_IDLE = 4,				// awaiting input
	STATUS_EMPTY = -1,				// uninitialized JsonCommand
	STATUS_JSON_BRACE_ERROR = -2,	// unbalanced JSON braces
	STATUS_JSON_BRACKET_ERROR = -3,	// unbalanced JSON braces
	STATUS_UNRECOGNIZED_NAME = -4,	// parse didn't recognize thecommand
	STATUS_JSON_PARSE_ERROR = -5,	// JSON invalid or too long
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
	STATUS_MOTOR_INDEX = -112,		// Internal error: motor index out of range
	STATUS_STEP_RANGE_ERROR = -113,	// Internal error: pulse step out of range [-1,0,1]
	STATUS_STROKE_END_ERROR = -114,	// Stroke delta/end-position mismatch
	STATUS_STROKE_MAXLEN = -115,	// Stroke maximum length exceeded
	STATUS_STROKE_PLANMICROS = -116,// Stroke planMicros < TICK_MICROSECONDS
	STATUS_STROKE_START = -117,		// Stroke start() must be called before traverse()
	STATUS_JSON_MEM = -118,			// Internal error: no more JSON memory
};

} // namespace firestep

#endif

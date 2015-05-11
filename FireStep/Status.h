#ifndef STATUS_H
#define STATUS_H

namespace firestep {

enum Status {
    STATUS_OK = 0,					// Operation completed successfully
    STATUS_BUSY_PARSED = 10,		// Json parsed, awaiting processing
    STATUS_BUSY = 11,				// Processing non-motion command
    STATUS_BUSY_MOVING = 12,		// Processing motion command
	STATUS_BUSY_SETUP = 13,			// Processing setup
    STATUS_WAIT_IDLE = 20,			// Awaiting input: inactive
    STATUS_WAIT_EOL = 21,			// Awaiting input: remainder of EOL-terminated command 
    STATUS_WAIT_CAMERA = 22,		// Awaiting input: camera ready display
    STATUS_WAIT_OPERATOR = 23,		// Awaiting input: operator attention required
    STATUS_WAIT_MOVING = 24,		// Awaiting input: show motion command display
    STATUS_WAIT_BUSY = 25,    		// Awaiting input: show non-motion command display
    STATUS_WAIT_CANCELLED = 26, 	// Awaiting input: command interrupted by serial input
    STATUS_EMPTY = -1,				// Uninitialized JsonCommand
    STATUS_JSON_BRACE_ERROR = -2,	// Unbalanced JSON braces
    STATUS_JSON_BRACKET_ERROR = -3,	// Unbalanced JSON braces
    STATUS_UNRECOGNIZED_NAME = -4,	// Parse didn't recognize thecommand
    STATUS_JSON_PARSE_ERROR = -5,	// JSON invalid or too long
    STATUS_JSON_TOO_LONG = -6,		// JSON exceeds buffer size
    STATUS_JSON_OBJECT = -7,		// JSON object expected
    STATUS_JSON_POSITIVE = -8,		// JSON value >= 0 expected
    STATUS_JSON_POSITIVE1 = -9,		// JSON value >= 1 expected
    STATUS_JSON_STROKE_ERROR = -20,	// Expected JSON object for stroke
    STATUS_S1_RANGE_ERROR = -21,	// Stroke segment s1 value out of range [-127,127]
    STATUS_S2_RANGE_ERROR = -22,	// Stroke segment s2 value out of range [-127,127]
    STATUS_S3_RANGE_ERROR = -23,	// Stroke segment s3 value out of range [-127,127]
    STATUS_S4_RANGE_ERROR = -24,	// Stroke segment s4 value out of range [-127,127]
    STATUS_S1S2LEN_ERROR = -25,		// Stroke segment s1/s2 length mismatch
    STATUS_S1S3LEN_ERROR = -26,		// Stroke segment s1/s3 length mismatch
    STATUS_S1S4LEN_ERROR = -27,		// Stroke segment s1/s4 length mismatch
    STATUS_STROKE_NULL_ERROR = -28,	// Stroke has no segments
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
    STATUS_WAIT_ERROR = -119,		// Display error indicator
	STATUS_AXIS_DISABLED = -120,	// Motion requested for disabled axis
	STATUS_NOPIN = -121,			// Attempt to change NOPIN
	STATUS_ESTOP = -122,			// Emergency hardware stop
	STATUS_SERIAL_CANCEL = -123,	// Command cancelled by serial input
	STATUS_TRAVEL_MAX = -124,		// Travel would exceed maximum
	STATUS_TRAVEL_MIN = -125,		// Travel would be below minimum
	STATUS_LIMIT_MAX = -126,		// Minimum limit switch tripped
	STATUS_LIMIT_MIN = -127,		// Maximum limit switch tripped
	STATUS_STATE = -128,			// Internal error: unknown processing state
	STATUS_MOTOR_ERROR = -129,		// Internal error: invalid motor index
};

inline bool isProcessing(Status status) {
	switch (status) {
		case STATUS_BUSY:
		case STATUS_BUSY_MOVING:
		case STATUS_BUSY_PARSED:
		case STATUS_BUSY_SETUP:
			return true;
		default:
			return false;
	}
}

} // namespace firestep

#endif

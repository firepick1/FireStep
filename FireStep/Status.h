#ifndef STATUS_H
#define STATUS_H

namespace firestep {

enum Status {
    STATUS_OK = 0,					// Operation completed successfully
    STATUS_BUSY_PARSED = 10,		// Json parsed, awaiting processing
    STATUS_BUSY = 11,				// Processing non-motion command
    STATUS_BUSY_MOVING = 12,		// Processing motion command
    STATUS_BUSY_SETUP = 13,			// Processing setup
    STATUS_BUSY_OK = 14,			// sub-command completed successfully
    STATUS_BUSY_EEPROM = 15,		// Startup EEPROM command queued for processing
    STATUS_BUSY_CALIBRATING = 16,	// Processing calibration command
    STATUS_WAIT_IDLE = 20,			// Awaiting input: inactive
    STATUS_WAIT_EOL = 21,			// Awaiting input: remainder of EOL-terminated command
    STATUS_WAIT_CAMERA = 22,		// Awaiting input: camera ready display
    STATUS_WAIT_OPERATOR = 23,		// Awaiting input: operator attention required
    STATUS_WAIT_MOVING = 24,		// Awaiting input: show motion command display
    STATUS_WAIT_BUSY = 25,    		// Awaiting input: show non-motion command display
    STATUS_WAIT_CANCELLED = 26, 	// Awaiting input: command interrupted by serial input
    STATUS_EMPTY = -1,				// Uninitialized JsonCommand

    // internal error
    STATUS_POSITION_ERROR = -100,	// Internal error: could not process position
    STATUS_AXIS_ERROR = -101,		// Internal error: could not process axis
    STATUS_SYS_ERROR = -102,		// Internal error: could not process system configuration
    STATUS_S1_ERROR = -103,			// Internal error: could not process segment
    STATUS_S2_ERROR = -104,			// Internal error: could not process segment
    STATUS_S3_ERROR = -105,			// Internal error: could not process segment
    STATUS_S4_ERROR = -106,			// Internal error: could not process segment
    STATUS_MOTOR_INDEX = -112,		// Internal error: motor index out of range
    STATUS_STEP_RANGE_ERROR = -113,	// Internal error: pulse step out of range [-1,0,1]
    STATUS_JSON_MEM1 = -115,		// Internal error: no more JSON memory (request)
    STATUS_JSON_MEM2 = -116,		// Internal error: no more JSON memory (response)
    STATUS_JSON_MEM3 = -117,		// Internal error: no more JSON memory (eeprom)
    STATUS_JSON_MEM4 = -118,		// Internal error: no more JSON memory (eeprom)
    STATUS_WAIT_ERROR = -119,		// Display error indicator
    STATUS_AXIS_DISABLED = -120,	// Motion requested for disabled axis
    STATUS_NOPIN = -121,			// Attempt to change NOPIN
    STATUS_MOTOR_ERROR = -129,		// Internal error: invalid motor index
    STATUS_NOT_IMPLEMENTED = -130,	// Proposed by not yet implemented
    STATUS_NO_MOTOR = -131,			// Axis must be mapped to motor
    STATUS_PIN_CONFIG = -132,		// Invalid pin configuration
    STATUS_VALUE_RANGE = -133,		// Provided value out of range
    STATUS_STATE = -134,			// Internal error: invalid state
    STATUS_CORE_PIN = -135,			// Attempt to assign a core pin for custom io
    STATUS_NO_SUCH_PIN = -136,		// pin number out of range
    STATUS_EEPROM_ADDR = -137,		// EEPROM address out of range
    STATUS_EEPROM_JSON = -138,		// No JSON to execute at EEPROM address
    STATUS_PROBE_PIN = -139,		// No probe pin specified
    STATUS_KINEMATIC_XYZ = -140,	// Could not solve XYZ cartesian kinematics
    STATUS_USER_EEPROM = -141,		// user EEPROM address out of range [2000,EEPROM_END)
    STATUS_CAL_HOME1 = -142,		// Home calibration probe data invalid: zCenter/zRim too far apart
    STATUS_MARK_INDEX = -143,		// Invalid mark index. Expected [1,MARK_COUNT]
    STATUS_MARK_AXIS = -144,		// Invalid mark axis. Expected: 1,2,3,4,x,y,z,a,b, or c
    STATUS_CAL_BED = -145,			// Calibration probe data insufficient for bed plane calculation
    STATUS_UNKNOWN_PROGRAM = -146,	// Unknown prg identifier
    STATUS_PROGRAM_SIZE = -147,		// Program to large to execute
    STATUS_ZBOWL_GEAR = -148,		// Z-bowl error exceeds amount allowed for gear ratio error
    STATUS_CAL_DEGREES = -149,		// Specify calibration angle from horizontal (down is positive degrees)
    STATUS_CAL_POSITION_0 = -150,	// Arm position cannot be 0 for arm gear ratio calibration
    STATUS_DELTA_HOME = -151,		// Delta axis home coordinates must be identical
	STATUS_INVALID_Z = -152,		// Invalid z-coordinate requested
	STATUS_NO_EEPROM = -153,		// Microprocessor has no EEPROM

    // stroke
    STATUS_STROKE_SEGPULSES = -200,	// Stroke has too many pulses per segment [-127,127]
    STATUS_STROKE_END_ERROR = -201,	// Stroke delta/end-position mismatch
    STATUS_STROKE_MAXLEN = -202,	// Stroke maximum length exceeded
    STATUS_STROKE_TIME = -203,		// Stroke planMicros < TICK_MICROSECONDS
    STATUS_STROKE_START = -204,		// Stroke start() must be called before traverse()
    STATUS_STROKE_NULL_ERROR = -205,// Stroke has no segments

    // JSON parsing
    STATUS_JSON_BRACE_ERROR = -400,	// Unbalanced JSON braces
    STATUS_JSON_BRACKET_ERROR = -401,// Unbalanced JSON braces
    STATUS_UNRECOGNIZED_NAME = -402,// Parse didn't recognize thecommand
    STATUS_JSON_PARSE_ERROR = -403,	// JSON invalid or too long
    STATUS_JSON_TOO_LONG = -404,	// JSON exceeds buffer size
    STATUS_JSON_OBJECT = -407,		// JSON object expected
    STATUS_JSON_POSITIVE = -408,	// JSON value >= 0 expected
    STATUS_JSON_POSITIVE1 = -409,	// JSON value >= 1 expected
    STATUS_JSON_KEY = -410,			// JSON buffer overflow: could not create JSON objecdt key
    STATUS_JSON_STROKE_ERROR = -411,// Expected JSON object for stroke
    STATUS_RANGE_ERROR = -412,		// Stroke segment s1 value out of range [-127,127]
    STATUS_S1S2LEN_ERROR = -413,	// Stroke segment s1/s2 length mismatch
    STATUS_S1S3LEN_ERROR = -414,	// Stroke segment s1/s3 length mismatch
    STATUS_S1S4LEN_ERROR = -415,	// Stroke segment s1/s4 length mismatch
    STATUS_FIELD_ERROR = -416,		// Internal error: could not process field
    STATUS_FIELD_RANGE_ERROR = -417,// Provided field value is out of range
    STATUS_FIELD_ARRAY_ERROR = -418,// Expected JSON field array value
    STATUS_FIELD_REQUIRED = -419,	// Expected JSON field value
    STATUS_JSON_ARRAY_LEN = -420,	// JSON array is too short
    STATUS_OUTPUT_FIELD = -421,		// JSON field is for output only
    STATUS_FIELD_HEX_ERROR = -422,	// Expected JSON field hex string value
    STATUS_TOPOLOGY_NAME = -423,	// JSON command not valid for current topology
    STATUS_JSON_CMD = -423,			// JSON command must be object or array
    STATUS_JSON_STRING = -424,		// JSON string expected
    STATUS_JSON_EEPROM = -425,		// JSON EEPROM string too long
    STATUS_JSON_EXEC = -426,		// No JSON command at EEPROM address
    STATUS_JSON_BOOL = -427,		// Expected JSON true or false
    STATUS_JSON_255 = -428,			// Expected JSON value between 0 and 255
    STATUS_JSON_DIGIT = -429,		// Expected numeric suffix for attribute
    STATUS_MTO_FIELD = -430,		// JSON field is not allowed in current machine topology

    // IFireStep
    STATUS_OPEN = -500,				// Expected prior call to IFireStep::open()
    STATUS_IFIRESTEP = -501,		// Expected non-null pointer to IFireStep instance
    STATUS_USB_OPEN = -502,			// ArduinoUSB::open() failed
    STATUS_USB_CLOSE = -503,		// ArduinoUSB::close() failed
    STATUS_TIMEOUT = -504,			// IFireStep::execute() timed out
    STATUS_USB_CONFIGURE = -505,	// Could not configure serial port
    STATUS_REQUEST_LF = -506,		// IFireStep::execute() request must end with LF

    // events
    STATUS_ESTOP = -900,			// Emergency hardware stop
    STATUS_SERIAL_CANCEL = -901,	// Command cancelled by serial input
    STATUS_TRAVEL_MIN = -902,		// Travel would be below minimum
    STATUS_TRAVEL_MAX = -903,		// Travel would exceed maximum
    STATUS_LIMIT_MIN = -904,		// Minimum limit switch tripped
    STATUS_LIMIT_MAX = -905,		// Maximum limit switch tripped
    STATUS_PROBE_FAILED = -906,		// Probe never contacted surface

    // Linux errors
    STATUS_LINUX = -1000,			// Linux error base
    STATUS_LINUX_EPERM = -1001,		// STATUS_LINUX - EPERM
    STATUS_LINUX_EINTR = -1004,		// STATUS_LINUX - EINTR
    STATUS_LINUX_EIO = -1005,		// STATUS_LINUX - EIO
    STATUS_LINUX_EAGAIN = -1011,	// STATUS_LINUX - EAGAIN
    STATUS_LINUX_EACCES = -1013,	// STATUS_LINUX - EACCES
    STATUS_LINUX_EINVAL = -1022,	// STATUS_LINUX - EINVAL
};

inline bool isProcessing(Status status) {
    switch (status) {
    case STATUS_BUSY:
    case STATUS_BUSY_MOVING:
    case STATUS_BUSY_CALIBRATING:
    case STATUS_BUSY_PARSED:
    case STATUS_BUSY_SETUP:
    case STATUS_BUSY_OK:
    case STATUS_BUSY_EEPROM:
        return true;
    default:
        return false;
    }
}

} // namespace firestep

#endif

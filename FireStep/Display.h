#ifndef DISPLAY_H
#define DISPLAY_H

namespace firestep {

enum DisplayStatus {
	// Awaiting serial input
    DISPLAY_WAIT_IDLE = 10, // Awaiting EOL-terminated command
	DISPLAY_WAIT_EOL = 11, // Awaiting remainder of EOL-terminated command
    DISPLAY_WAIT_CAMERA = 12, // Display for camera while awaiting further instructions
    DISPLAY_WAIT_BUSY = 13, // Display as if busy while awaiting further instructions

	// Awaiting user input
    DISPLAY_WAIT_OPERATOR = 20, // Awaiting EOL-terminated command initiated by operator
    DISPLAY_WAIT_ERROR = 21, // Could not proceed. Awaiting instructions

	// Ignoring input
    DISPLAY_BUSY = 30, // FireStep is busy but not moving
    DISPLAY_BUSY_MOVING = 31, // FireStep is busy and moving
};

typedef class Display {
	friend class JsonController;
	protected:
        uint8_t status;	// DisplayStatus
        uint8_t level; // 0:off, 255:brightest
    public:
        Display() : status(DISPLAY_BUSY), level(127) {}
		virtual void setup() { show(); }
		virtual void show() {}
		inline DisplayStatus getStatus() { return (DisplayStatus) status; }
		virtual void setStatus(DisplayStatus status=DISPLAY_WAIT_IDLE) {
			this->status = status;
			show();
		}
		inline uint8_t getLevel() { return level; }
		virtual void setLevel(uint8_t level=127) {
			this->level = level;
			show();
		}
} Display;

} // namespace firestep

#endif

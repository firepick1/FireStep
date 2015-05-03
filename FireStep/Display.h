#ifndef DISPLAY_H
#define DISPLAY_H

namespace firestep {

enum DisplayStatus {
	// Awaiting user input
    DISPLAY_WAIT_IDLE = 10,	
	DISPLAY_WAIT_EOL = 11,
    DISPLAY_WAIT_OPERATOR = 12,
    DISPLAY_WAIT_CAMERA = 13,	
    DISPLAY_WAIT_ERROR = 19,

	// Ignoring user input
    DISPLAY_BUSY_SETUP = 21,	
    DISPLAY_BUSY = 22,	
    DISPLAY_BUSY_MOVING = 23,	
};

typedef class Display {
	friend class JsonController;
	protected:
        uint8_t status;	// DisplayStatus
        uint8_t level; // 0:off, 255:brightest
    public:
        Display() : status(DISPLAY_BUSY_SETUP), level(127) {}
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

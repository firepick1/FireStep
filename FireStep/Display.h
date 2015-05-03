#ifndef DISPLAY_H
#define DISPLAY_H

namespace firestep {

enum DisplayStatus {
    DISPLAY_IDLE = 0,	
    DISPLAY_ERROR = 1,
    DISPLAY_OPERATOR = 2,
    DISPLAY_PROCESSING = 3,	
    DISPLAY_MOVING = 4,	
    DISPLAY_CAMERA = 5,	
};

enum DisplayLevel {
	DISPLAY_OFF = 0,
	DISPLAY_LOW = 4,
	DISPLAY_NORMAL = 8,
	DISPLAY_NIGH = 12,
	DISPLAY_HIGHEST = 16,
};

typedef class Display {
	friend class JsonController;
	protected:
        uint8_t status;	// DisplayStatus
        uint8_t level; // DisplayLevel
    public:
        Display() : status(DISPLAY_IDLE), level(DISPLAY_NORMAL) {}
		virtual void show() {}
		inline DisplayStatus getStatus() { return (DisplayStatus) status; }
		virtual void setStatus(DisplayStatus status=DISPLAY_IDLE) {
			this->status = status;
			show();
		}
		inline DisplayLevel getLevel() { return (DisplayLevel) level; }
		virtual void setLevel(DisplayLevel level=DISPLAY_NORMAL) {
			this->level = level;
			show();
		}
} Display;

} // namespace firestep

#endif

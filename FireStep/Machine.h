#ifndef MACHINE_H
#define MACHINE_H

#include "Stroke.h"
#include "Display.h"
#include "pins/pins.h"

namespace firestep {

// #define THROTTLE_SPEED /* Throttles driver speed from high (255) to low (0) */

#define A4988_PULSE_DELAY 	DELAY500NS;DELAY500NS
#define DRV8825_PULSE_DELAY DELAY500NS;DELAY500NS;DELAY500NS;DELAY500NS
#define STEPPER_PULSE_DELAY DRV8825_PULSE_DELAY
#define DELTA_COUNT 120
#define MOTOR_COUNT 4
#define AXIS_COUNT 6

#ifndef DELAY500NS
#define DELAY500NS \
  asm("nop");asm("nop");asm("nop");asm("nop"); asm("nop");asm("nop");asm("nop");asm("nop");
#endif

typedef struct Motor {
    uint8_t	axisMap; 	// index into axis array
    Motor() : axisMap(0) {}
} Motor;

enum AxisMode {
    MODE_DISABLE = 0,
    MODE_STANDARD = 1,
};

typedef class Axis {
    public:
        uint8_t 	mode;
        PinType 	pinStep;
        PinType 	pinDir;
        PinType 	pinMin;
        PinType 	pinMax;
        PinType 	pinEnable;
        StepCoord 	travelMin;
        StepCoord 	travelMax;
        StepCoord 	searchVelocity;
        StepCoord 	position;
        float		stepAngle;
        uint8_t		microsteps;
        bool		invertDir;
        uint8_t 	powerManagementMode;
        bool		atMin;
        bool		atMax;

    public:
        Axis();
        void readLimits(bool invertLim);
} Axis;

typedef class Machine : public QuadStepper {
        friend class JsonController;
    private:
        bool	invertLim;
        Display nullDisplay;
    public:
        Display	*pDisplay;
        Motor motor[MOTOR_COUNT];
        Axis axis[AXIS_COUNT];
        Stroke stroke;

    public:
        Machine();
        void init();
        virtual Status step(const Quad<StepCoord> &pulse);

        Quad<StepCoord> motorPosition();
} Machine;

} // namespace firestep

#endif

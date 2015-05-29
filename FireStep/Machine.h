#ifndef MACHINE_H
#define MACHINE_H

#include "Stroke.h"
#include "Display.h"
#include "pins.h"

extern void test_Home();

namespace firestep {

// #define THROTTLE_SPEED /* Throttles driver speed from high (255) to low (0) */

// A stepper pulse cycle requires 3 digitalWrite()'s for
// step direction, pulse high, and pulse low.
// Arduino 16-MHz digitalWrite pair takes 3.833 microseconds,
// so a full stepper pulse cycle should take ~5.7 microseconds:
//   http://skpang.co.uk/blog/archives/323
// A4983 stepper driver pulse cycle requires 2 microseconds
// DRV8825 stepper driver requires 3.8 microseconds
//   http://www.ti.com/lit/ds/symlink/drv8825.pdf
// Therefore, for currently fashionable stepper driver chips,
// Arduino digitalWrite() is slow enough to be its own delay (feature!)
// If you need longer pulse time, just add a delay:
// #define PULSE_DELAY DELAY500NS /* increase pulse cycle by 1 microsecond */
#define PULSE_DELAY /* no delay */

#define A4988_PULSE_DELAY 	DELAY500NS;DELAY500NS
#define DRV8825_PULSE_DELAY DELAY500NS;DELAY500NS;DELAY500NS;DELAY500NS
#define STEPPER_PULSE_DELAY DRV8825_PULSE_DELAY
#define DELTA_COUNT 120
#define MOTOR_COUNT 4
#define AXIS_COUNT 6
#define PIN_ENABLE LOW
#define PIN_DISABLE HIGH
#define MICROSTEPS_DEFAULT 16
#define INDEX_NONE -1

#ifndef DELAY500NS
#define DELAY500NS \
  asm("nop");asm("nop");asm("nop");asm("nop"); asm("nop");asm("nop");asm("nop");asm("nop");
#endif

typedef int16_t DelayMics; // delay microseconds
#ifdef TEST
extern int32_t delayMicsTotal;
#endif

/**
 * inline replacement for Arduino delayMicroseconds()
 */
inline void delayMics(int32_t usDelay) {
    if (usDelay > 0) {
#ifdef TEST
        delayMicsTotal += usDelay;
#endif
        while (usDelay-- > 0) {
            DELAY500NS;
            DELAY500NS;
        }
    }
}

enum AxisIndexValue {
    X_AXIS = 0,
    Y_AXIS = 1,
    Z_AXIS = 2,
    A_AXIS = 3,
    B_AXIS = 4,
    C_AXIS = 5,
    NO_AXIS = INDEX_NONE
};

typedef class Axis {
        friend void ::test_Home();
        friend class Machine;
    private:
        bool		enabled; // true: stepper drivers are enabled and powered
    public:
        PinType 	pinStep; // step pin
        PinType 	pinDir;	// step direction pin
        PinType 	pinMin; // homing minimum limit switch
        PinType 	pinMax;	// maximum limit switch (optional)
        PinType 	pinEnable; // stepper driver enable pin (nENBL)
        StepCoord	home; // home position
        StepCoord 	travelMin; // soft minimum travel limit
        StepCoord 	travelMax; // soft maximum travel limit
        StepCoord 	position; // current position (pulses)
        StepCoord 	latchBackoff; // pulses to send for backing off limit switch
        DelayMics	usDelay; // minimum time between stepper pulses
        DelayMics 	searchDelay; // limit switch search velocity (pulse delay microseconds)
        DelayMics	idleSnooze; // idle enable-off snooze delay (microseconds)
        float		stepAngle; // 1.8:200 steps/rev; 0.9:400 steps/rev
        uint8_t		microsteps;	// normally 1,2,4,8,16 or 32
        bool		dirHIGH; // advance on HIGH
        bool        advancing; // current direction
        bool		atMin; // minimum limit switch (last value read)
        bool		atMax; // maximum limit switch (last value read)
        bool		homing; // true:axis is active for homing

        Axis() :
            pinStep(NOPIN),
            pinDir(NOPIN),
            pinMin(NOPIN),
            pinMax(NOPIN),
            pinEnable(NOPIN),
            home(0),
            travelMin(0),
            travelMax(32767),
            position(0),
            latchBackoff(MICROSTEPS_DEFAULT),
            usDelay(0), // Suggest 80us (12.8kHz) for microsteps 1
            searchDelay(80), // a slow, cautious but accurate speed
            idleSnooze(0), // 0:disabled; 1000:weak, noisy, cooler
            stepAngle(1.8),
            microsteps(MICROSTEPS_DEFAULT),
            dirHIGH(true), // true:advance on HIGH; false:advance on LOW
            advancing(false),
            atMin(false),
            atMax(false),
            enabled(false),
            homing(false)
        {};
        Status enable(bool active);
        bool isEnabled() {
            return enabled;
        }
        inline Status pinMode(PinType pin, int mode) {
            if (pin == NOPIN) {
                return STATUS_NOPIN;
            }
            ::pinMode(pin, mode);
            return STATUS_OK;
        }
        inline void pulse(bool advance) {
            if (advance != advancing) {
                advancing = advance;
                digitalWrite(pinDir, (advance == dirHIGH) ? HIGH : LOW);
            }
            digitalWrite(pinStep, HIGH);
            PULSE_DELAY;
            digitalWrite(pinStep, LOW);
        }
        inline Status readAtMin(bool invertLim) {
            if (pinMin == NOPIN) {
                return STATUS_NOPIN;
            }
            bool minHigh = digitalRead(pinMin);
            bool atMinNew = (invertLim == !minHigh);
            if (atMinNew != atMin) {
                atMin = atMinNew;
            }
            return STATUS_OK;
        }
        inline Status readAtMax(bool invertLim) {
            if (pinMax == NOPIN) {
                return STATUS_NOPIN;
            }
            bool maxHigh = digitalRead(pinMax);
            bool atMaxNew = (invertLim == !maxHigh);
            if (atMaxNew != atMax) {
                atMax = atMaxNew;
            }
            return STATUS_OK;
        }
} Axis;
typedef int8_t AxisIndex;
typedef int8_t MotorIndex;

typedef class Machine : public QuadStepper {
        friend void ::test_Home();
    private:
        bool	pinEnableHigh;
        Display nullDisplay;
        int8_t 	stepHome(int16_t pulsesPerAxis, int16_t searchDelay);
        Axis *	motorAxis[MOTOR_COUNT];
        AxisIndex	motor[MOTOR_COUNT];
        PinConfig	pinConfig;

    public:
        bool	invertLim;
        bool	jsonPrettyPrint;
        Display	*pDisplay;
        Axis axis[AXIS_COUNT];
        Stroke stroke;

    public:
        Machine();
        void enable(bool active);
        virtual Status step(const Quad<StepCoord> &pulse);
        Status pulse(Quad<StepCoord> &pulses);
        void setPin(PinType &pinDst, PinType pinSrc, int16_t mode, int16_t value = LOW);
        Quad<StepCoord> getMotorPosition();
        void setMotorPosition(const Quad<StepCoord> &position);
        virtual Status home();
        void idle();
        Status setAxisIndex(MotorIndex iMotor, AxisIndex iAxis);
        AxisIndex getAxisIndex(MotorIndex iMotor) {
			return motor[iMotor];
		}
        Axis& getMotorAxis(MotorIndex iMotor) {
            return axis[motor[iMotor]];
        }
        Status moveTo(Quad<StepCoord> destination, float seconds);
        Status moveDelta(Quad<StepCoord> delta, float seconds);
        MotorIndex motorOfName(const char* name);
        AxisIndex axisOfName(const char *name);
        Status setPinConfig(PinConfig pc);
        PinConfig getPinConfig() {
            return pinConfig;
        }
} Machine;

#ifdef TEST
extern int32_t delayMicsTotal;
#endif

} // namespace firestep

#endif

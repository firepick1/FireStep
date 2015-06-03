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
// #define PULSE_WIDTH_DELAY DELAY500NS /* increase pulse cycle by 1 microsecond */
#define PULSE_WIDTH_DELAY() /* no delay */

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
            travelMax(32000),	// 5 full 400-step revolutions @16-microsteps
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
            PULSE_WIDTH_DELAY();
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

inline void digitalWriteFast(uint8_t pin, uint8_t val)
{
	//uint8_t timer = digitalPinToTimer(pin);
	uint8_t bit = digitalPinToBitMask(pin);
	uint8_t port = digitalPinToPort(pin);
	volatile uint8_t *out;

	//if (port == NOT_A_PIN) return;

	// If the pin that support PWM output, we need to turn it off
	// before doing a digital write.
	//if (timer != NOT_ON_TIMER) turnOffPWM(timer);

	out = portOutputRegister(port);

	uint8_t oldSREG = SREG;
	cli();

	if (val == LOW) {
		*out &= ~bit;
	} else {
		*out |= bit;
	}

	SREG = oldSREG;
}

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
        virtual Status step(const Quad<StepDV> &pulse);
		inline void pulsePin1(int16_t pinStep) {
			digitalWriteFast(pinStep, HIGH);
			PULSE_WIDTH_DELAY();
			digitalWriteFast(pinStep, LOW);
		}
		inline int8_t pulsePin(int16_t pinStep, int8_t n) {
			switch (n) {
			case 0:
				pulsePin1(pinStep);
				pulsePin1(pinStep);
				pulsePin1(pinStep);
				pulsePin1(pinStep);
				return 4;
			case 3:
				pulsePin1(pinStep);
				pulsePin1(pinStep);
				pulsePin1(pinStep);
				return 3;
			case 2:
				pulsePin1(pinStep);
				pulsePin1(pinStep);
				return 2;
			case 1:
				pulsePin1(pinStep);
				return 1;
			}
		}
        inline Status stepFast(Quad<StepDV> &pulse) {
			Quad<StepDV> p(pulse);
			//TESTCOUT4("stepFast ", (int) p.value[0], ",", (int) p.value[1], ",", 
				//(int) p.value[2], ",", (int) p.value[3]);
			for (bool hasPulses=true; hasPulses;) {
				hasPulses = false;
				for (uint8_t i=0; i<QUAD_ELEMENTS; i++) {
					// emit 0-4 pulse burst per axis
					int16_t pinStep = motorAxis[i]->pinStep;
					int8_t pv = p.value[i];
					if (pv > 0) {
						p.value[i] -= pulsePin(pinStep, pv & (int8_t) 0x3);
						hasPulses = true;
					} else if (pv < 0) {
						p.value[i] += pulsePin(pinStep, -pv & (int8_t) 0x3);
						hasPulses = true;
					}
				}
				//TESTCOUT4("stepFast => ", (int) p.value[0], ",", (int) p.value[1], ",", 
					//(int) p.value[2], ",", (int) p.value[3]);
			}

            return STATUS_OK;
        }
        virtual Status stepDirection(const Quad<StepDV> &pulse);
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

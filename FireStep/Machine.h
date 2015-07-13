#ifndef MACHINE_H
#define MACHINE_H

#include "Stroke.h"
#include "Display.h"
#include "pins.h"

extern void test_Home();

namespace firestep {

// #define THROTTLE_SPEED /* Throttles driver speed from high (255) to low (0) */


#define LATCH_BACKOFF 200
#define DELTA_COUNT 120
#define MOTOR_COUNT 4
#define AXIS_COUNT 6
#define PIN_ENABLE LOW
#define PIN_DISABLE HIGH
#define MICROSTEPS_DEFAULT 16
#define INDEX_NONE -1

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
    DelayMics	usDelay; // minimum time between stepper pulses
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
        travelMin(-32000),  // -5 full 400-step revolutiosn @16-microsteps
        travelMax(32000),	// 5 full 400-step revolutions @16-microsteps
        position(0),
        usDelay(0), // Suggest 80us (12.8kHz) for microsteps 1
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
    inline void setAdvancing(bool advance) {
        if (advance != advancing) {
            advancing = advance;
            digitalWrite(pinDir, (advance == dirHIGH) ? HIGH : LOW);
        }
    }
    inline void pulse(bool advance) {
        setAdvancing(advance);
        pulseFast(pinStep);
    }
    inline Status readAtMin(bool invertLim) {
        if (pinMin == NOPIN) {
            return STATUS_NOPIN;
        }
        bool minHigh = digitalRead(pinMin);
        bool atMinNew = (invertLim == !minHigh);
        if (atMinNew != atMin) {
            TESTCOUT2("readAtMin() pinMin:", (int) pinMin, " atMinNew:", atMinNew);
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

typedef class OpProbe {
public:
    Quad<StepCoord> start; // probe starting point
    Quad<StepCoord> end; // probe goal
    StepCoord	maxDelta; // absolute value of maximum stepper displacement
    StepCoord	curDelta; // absolute value of current stepper displacement
    PinType 	pinProbe; // pin used for probe limit
    bool		probing;
    bool		invertProbe; // invert logic sense of probe

    OpProbe() {
        init(Quad<StepCoord>());
    }
    void init(Quad<StepCoord> position) {
        start = position;
        end = position;
        maxDelta = 0;
        curDelta = 0;
        pinProbe = NOPIN;
        probing = true;
        invertProbe = false;
    }
    StepCoord interpolate(MotorIndex iMotor) {
        float t = maxDelta ? (float)curDelta/(float)maxDelta : 0;
        return (StepCoord)(start.value[iMotor]*(1-t) + end.value[iMotor]*t + 0.5);
    }
} OpProbe;

typedef class Machine : public QuadStepper {
    friend void ::test_Home();
private:
    bool	 	pinEnableHigh;
    Display 	nullDisplay;
    StepCoord 	stepHome(StepCoord pulsesPerAxis, int16_t delay);
    Status	 	stepProbe(int16_t delay);
    Axis *		motorAxis[MOTOR_COUNT];
    AxisIndex	motor[MOTOR_COUNT];
    PinConfig	pinConfig;

protected:
    Status		setPinConfig_EMC02();
    Status 		setPinConfig_RAMPS1_4();
    void 		backoffHome(int16_t delay);

public:
    bool		invertLim;
    bool		jsonPrettyPrint;
    int32_t 	vMax; // maximum stroke velocity (pulses/second)
    PH5TYPE 	tvMax; // time to reach maximum velocity
    Display*	pDisplay;
    Axis 		axis[AXIS_COUNT];
    Stroke		stroke;
    int16_t		homingPulses;
    StepCoord	latchBackoff;
    DelayMics 	searchDelay; // limit switch search velocity (pulse delay microseconds)
    PinType		pinStatus;
    struct {
        OpProbe		probe;
    } op;

public:
    Machine();
    virtual	Status step(const Quad<StepDV> &pulse);
    bool isCorePin(int16_t pin);
    inline bool isAtLimit(PinType pin) {
        bool pinValue = digitalRead(pin);
        return (invertLim == !pinValue);
    }
    inline int8_t pulsePin(int16_t pinStep, int8_t n) {
        switch (n) {
        case 0:
            pulseFast(pinStep);
            pulseFast(pinStep);
            pulseFast(pinStep);
            pulseFast(pinStep);
            return 4;
        case 3:
            pulseFast(pinStep);
            pulseFast(pinStep);
            pulseFast(pinStep);
            return 3;
        case 2:
            pulseFast(pinStep);
            pulseFast(pinStep);
            return 2;
        case 1:
            pulseFast(pinStep);
            return 1;
        }
    }
    inline Status stepFast(Quad<StepDV> &pulse) {
        Quad<StepDV> p(pulse);
        //TESTCOUT4("stepFast ", (int) p.value[0], ",", (int) p.value[1], ",",
        //(int) p.value[2], ",", (int) p.value[3]);
        for (bool hasPulses = true; hasPulses;) {
            hasPulses = false;
            for (uint8_t i = 0; i < QUAD_ELEMENTS; i++) {
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
    virtual Status home(Status status);
    virtual Status probe(Status status);
    void idle();
    Status setAxisIndex(MotorIndex iMotor, AxisIndex iAxis);
    AxisIndex getAxisIndex(MotorIndex iMotor) {
        return motor[iMotor];
    }
    Axis& getMotorAxis(MotorIndex iMotor) {
        return axis[motor[iMotor]];
    }
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

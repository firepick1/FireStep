#ifndef MACHINE_H
#define MACHINE_H

#ifdef CMAKE
#include <cmath>
#endif
#include "Stroke.h"
#include "Display.h"
#include "DeltaCalculator.h"
#include "pins.h"
#include "IDuino.h"

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
#define PROBE_DATA 9
#define EEUSER 2000
#define EEUSER_ENABLED (EEUSER-1)
#define MARK_COUNT 9
#define MAX_ATTR_BYTES 30

#define FPD_GEAR_RATIO 9.52374
#define FPD_GEAR_RATIO_S "9.524"
#define LC_GEAR_RATIO	9.58600
#define LC_GEAR_RATIO_S	"9.58600"
#define TW_GEAR_RATIO	9.460206
#define TW_GEAR_RATIO_S	"9.460206"
#define FPD_GEAR_RATIO_S5 "9.52374"
#define FPD_SPE_ANGLE -54.617
#define FPD_SPE_ANGLE_S "-54.617"
#define FPD_SPE_RATIO -0.383
#define FPD_SPE_RATIO_S "-0.383"
#define FPD_SEARCH_DELAY 800
#define FPD_SEARCH_DELAY_S "800"
#define FPD_SEARCH_DELAY_400 400
#define FPD_SEARCH_DELAY_400_S "400"
#define FPD_HOME_ANGLE -67.202
#define FPD_HOME_ANGLE_S "-67.202"
#define FPD_HOME_PULSES -5689
#define FPD_HOME_PULSES_S "-5689"
//#define FPD_SPE_HOME_PULSES -5473
//#define FPD_SPE_HOME_PULSES_S "-5473"
#define FPD_SPE_HOME_PULSES -5394
#define FPD_SPE_HOME_PULSES_S "-5394"
#define FPD_SPE_CRITICAL_PULSES -4624
#define FPD_SPE_CRITICAL_PULSES_S "-4624"
#define FPD_FAST_SEARCH_PULSES 3
#define FPD_FAST_SEARCH_PULSES_S "3"
#define FPD_MICROSTEPS 16
#define FPD_STEPS360 200
#define FPD_MICROSTEPS_PER_DEGREE 84.655
#define FPD_DELTA_E		131.636 /* effector triangle side */
#define FPD_DELTA_E_S	"131.636"
#define FPD_DELTA_F		190.526 /* base triangle side */
#define FPD_DELTA_F_S	"190.526"
#define FPD_DELTA_RE_S	"270.000"
#define FPD_DELTA_RE	270.000 /* effector arm length (mm) */
#define FPD_DELTA_RE_S	"270.000"
#define TW_DELTA_RE 	268.000
#define TW_DELTA_RE_S 	"268.000"
#define LC_DELTA_RE 	270.669
#define LC_DELTA_RE_S 	"270.669"
#define FPD_DELTA_RF	90.000 /* pulley arm length (mm) */
#define FPD_DELTA_RF_S	"90.000"
#define LC_DELTA_RF		89.750 /* pulley arm length (mm) */
#define LC_DELTA_RF_S	"89.750"

typedef int16_t DelayMics; // delay microseconds
#ifdef TEST
extern int32_t delayMicsTotal;
#endif

enum OutputMode {
    OUTPUT_ARRAY1=0, //  JSON command arrays only return last command response
    OUTPUT_ARRAYN=1, // JSON command arrays return all command responses
    OUTPUT_CMT=2, // Write comments
};

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

enum Topology {
    MTO_RAW = 0, // Raw stepper coordinates in microstep pulses
    MTO_FPD = 1, // Rotational delta with FirePick Delta dimensions
};

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
    char		id;		// a single character describing the axis

public: // configuration
    StepCoord	latchBackoff; // high speed limit switch compensation
    StepCoord	home; // home position
    StepCoord 	travelMin; // soft minimum travel limit
    StepCoord 	travelMax; // soft maximum travel limit
    DelayMics	usDelay; // minimum time between stepper pulses
    DelayMics	idleSnooze; // idle enable-off snooze delay (microseconds)
    float		stepAngle; // 1.8:200 steps/rev; 0.9:400 steps/rev
    uint8_t		microsteps;	// normally 1,2,4,8,16 or 32
    bool		dirHIGH; // advance on HIGH
    PinType 	pinStep; // step pin
    PinType 	pinDir;	// step direction pin
    PinType 	pinMin; // homing minimum limit switch
    PinType 	pinMax;	// maximum limit switch (optional)
    PinType 	pinEnable; // stepper driver enable pin (nENBL)
    bool		atMin; // minimum limit switch (last value read)
    bool		atMax; // maximum limit switch (last value read)
    bool        advancing; // current direction
    bool		homing; // true:axis is active for homing
    StepCoord 	position; // current position (pulses)

public:
    Axis() :
        enabled(false),
        pinStep(NOPIN),
        pinDir(NOPIN),
        pinMin(NOPIN),
        pinMax(NOPIN),
        pinEnable(NOPIN),
        home(0),
        latchBackoff(LATCH_BACKOFF),
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
        homing(false)
    {};

    int32_t hash();
    Status enable(bool active);
    char * saveConfig(char *out, size_t maxLen);
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

enum ProbeDataSource {
    PDS_NONE = 0, //  no data source
    PDS_Z = 1, // Cartesian Z-coordinate
};

typedef class OpProbe {
public:
    Quad<StepCoord> start; // probe starting point
    Quad<StepCoord> end; // probe goal
    StepCoord		maxDelta; // absolute value of maximum stepper displacement
    StepCoord		curDelta; // absolute value of current stepper displacement
    PinType 		pinProbe; // pin used for probe limit
    ProbeDataSource	dataSource;
    bool			probing;
    bool			invertProbe; // invert logic sense of probe
    PH5TYPE			probeData[PROBE_DATA];

    OpProbe() : pinProbe(NOPIN), invertProbe(false) {
        setup(Quad<StepCoord>());
        memset(probeData, 0, sizeof(probeData));
    }
    void setup(Quad<StepCoord> posStart) {
        setup(posStart, posStart);
    }
    void setup(Quad<StepCoord> posStart, Quad<StepCoord> posEnd) {
        start = posStart;
        end = posEnd;
        maxDelta = 0;
        for (QuadIndex i=0; i<QUAD_ELEMENTS; i++) {
            maxDelta = maxval(maxDelta, (StepCoord) absval(end.value[i] - start.value[i]));
        }
        curDelta = 0;
        dataSource = PDS_NONE;
        if (pinProbe == NOPIN) {
            probing = false;
        } else {
            pinMode(pinProbe, INPUT);
            probing = true;
        }
    }
    StepCoord interpolate(MotorIndex iMotor) {
        float t = maxDelta ? (float)curDelta/(float)maxDelta : 0;
        return (StepCoord)(start.value[iMotor]*(1-t) + end.value[iMotor]*t + 0.5);
    }
    void archiveData(PH5TYPE data) {
        for (int16_t i=PROBE_DATA; --i>0; ) {
            probeData[i] = probeData[i-1];
        }
        probeData[0] = data;
    }
} OpProbe;

typedef class ZPlane {
public:
    PH5TYPE a;
    PH5TYPE b;
    PH5TYPE c;

    ZPlane(PH5TYPE a=0, PH5TYPE b=0, PH5TYPE c=0) :a(a), b(b), c(c) {}
    ZPlane& operator=(const ZPlane that) {
        a = that.a;
        b = that.b;
        c = that.c;
        return *this;
    }
    bool initialize(XYZ3D p1, XYZ3D p2, XYZ3D p3);
    PH5TYPE calcZ(PH5TYPE x, PH5TYPE y) {
        return a*x + b*y + c;
    }
    inline PH5TYPE getXScale() {
        return a;
    }
    inline PH5TYPE getYScale() {
        return b;
    }
    inline PH5TYPE getZOffset() {
        return c;
    }
    inline void setZOffset(PH5TYPE value) {
        c = value;
    }
} ZPlane;

typedef class Machine : public QuadStepper {
    friend void ::test_Home();

private:
    PH5TYPE		homeAngle; // recorded home angle
    IDuinoPtr	pDuino;

public:
    PinConfig	pinConfig;
    bool		autoHome;
    bool	 	pinEnableHigh;
    bool		invertLim;
    bool		jsonPrettyPrint;
    bool		autoSync; // auto-save configuration to EEPROM
    uint8_t		debounce;
    AxisIndex	motor[MOTOR_COUNT];
    Display 	nullDisplay;
    DeltaCalculator delta;
    int32_t 	vMax; // maximum stroke velocity (pulses/second)
    PH5TYPE 	tvMax; // time to reach maximum velocity
    PH5TYPE		marks[MARK_COUNT];
    int16_t		fastSearchPulses;
    DelayMics 	searchDelay; // limit switch search velocity (pulse delay microseconds)
    PinType		pinStatus;
    Topology	topology;
    OutputMode	outputMode;
    struct {
        OpProbe		probe;
    } op;
    int32_t		syncHash;
    ZPlane		bed;

public:
    Axis 		axis[AXIS_COUNT];
    Display*	pDisplay;
    Axis *		motorAxis[MOTOR_COUNT];
    Stroke		stroke;

protected:
    Status	 	stepProbe(int16_t delay);
    Status		setPinConfig_EMC02();
    Status 		setPinConfig_RAMPS1_4();
    void 		backoffHome(int16_t delay);
    StepCoord 	stepHome(StepCoord pulsesPerAxis, int16_t delay);

public:
    Machine(IDuinoPtr pDuino=NULL);
    void setup(PinConfig cfg);
    int32_t hash();
    virtual	Status step(const Quad<StepDV> &pulse);
    bool isCorePin(int16_t pin);
    inline bool isAtLimit(PinType pin) {
        uint8_t highCount = digitalRead(pin) ? 1 : 0;
        for (uint8_t i=0; i<debounce; i++) {
            highCount += digitalRead(pin) ? 1 : 0;
        }
        return (invertLim == !(highCount > debounce/2));
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
    virtual Status probe(Status status, DelayMics delay=-1);
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
    char * saveSysConfig(char *out, size_t maxLen);
    char * saveDimConfig(char *out, size_t maxLen);
    Status idle(Status status);
    Status sync(Status status);
    void enableEEUser(bool enable);
    bool isEEUserEnabled();
    void loadDeltaCalculator(); // initialize from raw axis
    PH5TYPE getHomeAngle() {
        return homeAngle;
    }
    void setHomeAngle(PH5TYPE degrees);
    void setHomeAngleFromPulses(StepCoord pulseCount);
} Machine;

#ifdef TEST
extern int32_t delayMicsTotal;
#endif

char * saveConfigValue(const char *key, const char *value, char *out);
char * saveConfigValue(const char *key, bool value, char *out);
char * saveConfigValue(const char *key, int32_t value, char *out);
char * saveConfigValue(const char *key, int16_t value, char *out);
char * saveConfigValue(const char *key, PH5TYPE value, char *out, uint8_t places=2);

} // namespace firestep

#endif

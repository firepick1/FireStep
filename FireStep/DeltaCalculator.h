#ifndef DELTACALCULATOR_H
#define DELTACALCULATOR_H

#include "Stroke.h"

namespace firestep {

#define NO_SOLUTION ((PH5TYPE)1E20)

enum DeltaAxis {
    DELTA_AXIS_ALL = -1,
    DELTA_AXIS_1 = 0,
    DELTA_AXIS_2 = 1,
    DELTA_AXIS_3 = 2
};

typedef class Step3D {
private:
    bool valid;
public:
    StepCoord p1;
    StepCoord p2;
    StepCoord p3;
    Step3D(StepCoord p1, StepCoord p2, StepCoord p3) : valid(true), p1(p1), p2(p2), p3(p3) {}
    Step3D(bool valid=true, PH5TYPE v=0): valid(valid), p1(v), p2(v), p3(v) {}
    inline bool isValid() {
        return valid;
    }
} Step3D;

typedef class XYZ3D {
private:
    bool valid;
public:
    PH5TYPE x;
    PH5TYPE y;
    PH5TYPE z;
    XYZ3D(PH5TYPE x, PH5TYPE y, PH5TYPE z) : valid(true), x(x), y(y), z(z) {}
    XYZ3D(bool valid=true,PH5TYPE v=0): valid(valid), x(v), y(v), z(v) {}
    bool operator==(const XYZ3D& that) {
        return x == that.x && y == that.y && z == that.z && valid == that.valid;
    }
    bool operator!=(const XYZ3D& that) {
        return x != that.x || y != that.y || z != that.z || valid != that.valid;
    }
    inline bool isValid() {
        return valid;
    }
} XYZ3D;

typedef struct Angle3D {
private:
    bool valid;
public:
    PH5TYPE theta1;
    PH5TYPE theta2;
    PH5TYPE theta3;
    Angle3D(PH5TYPE theta1, PH5TYPE theta2, PH5TYPE theta3)
        : valid(true), theta1(theta1), theta2(theta2), theta3(theta3) {}
    Angle3D(bool valid=true, PH5TYPE v=0): valid(valid), theta1(v), theta2(v), theta3(v) {}
    inline bool isValid() {
        return valid;
    }
} Angle3D;

typedef class DeltaCalculator {
protected:
    PH5TYPE f;
    PH5TYPE e;
    PH5TYPE rf;
    PH5TYPE re;
    PH5TYPE acr; // arm clearance radius at pulley
    int16_t steps360;
    int16_t microsteps;
    PH5TYPE degreesPerPulse[3];
    PH5TYPE dz;
    PH5TYPE homeAngle;
    PH5TYPE spAngle;
    PH5TYPE spRatio;
    static PH5TYPE sqrt3;
    static PH5TYPE sin120;
    static PH5TYPE cos120;
    static PH5TYPE tan60;
    static PH5TYPE sin30;
    static PH5TYPE tan30;
    static PH5TYPE tan30_half;
    static PH5TYPE pi;
    static PH5TYPE dtr;

protected:
    PH5TYPE calcZBowlErrorFromEParam(Step3D center, Step3D rim, PH5TYPE eK, PH5TYPE &param);
    PH5TYPE calcZBowlErrorFromEParam(PH5TYPE zCenter, PH5TYPE radius, PH5TYPE eK, PH5TYPE &param);

public:
    DeltaCalculator();
    void useEffectorOrigin();
    inline PH5TYPE getSPEAngle() {
        return spAngle;
    }
    inline void setSPEAngle(PH5TYPE value) {
        spAngle = value;
    }
    inline PH5TYPE getSPERatio() {
        return spRatio;
    }
    inline void setSPERatio(PH5TYPE value) {
        spRatio = value;
    }
    inline PH5TYPE getBaseTriangleSide() {
        return f;
    }
    inline void setBaseTriangleSide(PH5TYPE value) {
        f = value;
    }
    inline PH5TYPE getEffectorTriangleSide() {
        return e;
    }
    inline void setEffectorTriangleSide(PH5TYPE value) {
        e = value;
    }
    inline PH5TYPE getBaseArmLength() {
        return rf;
    }
    inline void setBaseArmLength(PH5TYPE value) {
        rf = value;
    }
    inline PH5TYPE getEffectorLength() {
        return re;
    }
    inline void setEffectorLength(PH5TYPE value) {
        re = value;
    }
    inline PH5TYPE getArmClearanceRadius() {
        return acr;
    }
    inline void setArmClearanceRadius(PH5TYPE value) {
        acr = value;
    }
    inline int16_t getSteps360() {
        return steps360;
    }
    void setSteps360(int16_t value);
    inline int16_t getMicrosteps() {
        return microsteps;
    }
    void setMicrosteps(int16_t value);
    inline PH5TYPE getDegreesPerPulse(DeltaAxis axis=DELTA_AXIS_ALL) {
        PH5TYPE dpp;
        if (axis == DELTA_AXIS_ALL) {
            dpp = (degreesPerPulse[0]+degreesPerPulse[1]+degreesPerPulse[2])/3;
        } else {
            dpp = degreesPerPulse[axis];
        }

        //TESTCOUT2("getDegreesPerPulse:", dpp, " axis:", axis);
        return dpp;
    }
    void setDegreesPerPulse(PH5TYPE value, DeltaAxis axis=DELTA_AXIS_ALL);
    inline PH5TYPE getGearRatio(DeltaAxis axis=DELTA_AXIS_ALL) {
        PH5TYPE gearRatio = 360/(microsteps*steps360*getDegreesPerPulse(axis));
        if (axis == DELTA_AXIS_ALL) {
            PH5TYPE gearRatio = ( getGearRatio(DELTA_AXIS_1)+
                                  getGearRatio(DELTA_AXIS_2)+
                                  getGearRatio(DELTA_AXIS_3))/3;
            return gearRatio;
        } else {
            PH5TYPE gearRatio = 360/(microsteps*steps360*getDegreesPerPulse(axis));
            return gearRatio;
        }
    }
    void setGearRatio(PH5TYPE value, DeltaAxis axis=DELTA_AXIS_ALL);
    inline PH5TYPE getZOffset() {
        // Z distance from base to effector with arms level (0 degrees)
        // DEPENDENCIES: ZOffset(e,f,re,rf)
        return dz;
    }
    PH5TYPE getMinZ(PH5TYPE x=0, PH5TYPE y=0);  // lowest possible point at given XY
    PH5TYPE getMinDegrees(); // base/effector arms are colinear here (which is usually bad mechanically)
    PH5TYPE getDefaultHomeAngle(); // calculated using arm clearance radius
    inline PH5TYPE getHomeAngle() {
        return homeAngle;
    }
    inline void setHomeAngle(PH5TYPE value) {
        homeAngle = value;
    }
    StepCoord getHomePulses();
    void setHomePulses(StepCoord value);
	StepCoord calcSPEPulses(PH5TYPE armAngle, DeltaAxis axis=DELTA_AXIS_ALL);
	PH5TYPE calcSPEAngle(StepCoord pulses, DeltaAxis axis=DELTA_AXIS_ALL);
    Step3D calcPulses(XYZ3D xyz);
    Angle3D calcAngles(XYZ3D xyz);
    XYZ3D calcXYZ(Step3D pulses);
    XYZ3D calcXYZ(Angle3D angles);
    PH5TYPE calcAngleYZ(PH5TYPE x, PH5TYPE y, PH5TYPE z);
    PH5TYPE calcZBowlErrorFromGearRatio(Step3D center, Step3D rim, PH5TYPE gearRatio);
    PH5TYPE calcZBowlErrorFromGearRatio(PH5TYPE zCenter, PH5TYPE radius, PH5TYPE gearRatio);
    PH5TYPE calcZBowlGearRatio(PH5TYPE zCenter, PH5TYPE zRim, PH5TYPE radius);
    PH5TYPE calcZBowlErrorFromETheta(Step3D center, Step3D rim, PH5TYPE eTheta);
    PH5TYPE calcZBowlErrorFromETheta(PH5TYPE zCenter, PH5TYPE radius, PH5TYPE eTheta);
    PH5TYPE calcZBowlETheta(PH5TYPE zCenter, PH5TYPE zRim, PH5TYPE radius);
    int32_t hash();
    static StepCoord roundStep(PH5TYPE value);
    PH5TYPE calcZBowlErrorFromE_f(PH5TYPE zCenter, PH5TYPE radius, PH5TYPE eK);
    PH5TYPE calcZBowlErrorFromE_e(PH5TYPE zCenter, PH5TYPE radius, PH5TYPE eK);
    PH5TYPE calcZBowlErrorFromE_rf(PH5TYPE zCenter, PH5TYPE radius, PH5TYPE eK);
    PH5TYPE calcZBowlErrorFromE_re(PH5TYPE zCenter, PH5TYPE radius, PH5TYPE eK);
} DeltaCalculator;

} // firestep

#endif

#ifndef DELTACALCULATOR_H
#define DELTACALCULATOR_H

#include "Stroke.h"

namespace firestep {

#define NO_SOLUTION ((PH5TYPE)1E20)

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
    int16_t steps360;
    int16_t microsteps;
    PH5TYPE gearRatio;
    PH5TYPE dz;
    Angle3D eTheta;
    static PH5TYPE sqrt3;
    static PH5TYPE sin120;
    static PH5TYPE cos120;
    static PH5TYPE tan60;
    static PH5TYPE sin30;
    static PH5TYPE tan30;
    static PH5TYPE tan30_half;
    static PH5TYPE pi;
    static PH5TYPE dtr;
public:
    DeltaCalculator();
    void setup();
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
    inline int16_t getSteps360() {
        return steps360;
    }
    inline void setSteps360(int16_t value) {
        steps360 = value;
    }
    inline int16_t getMicrosteps() {
        return microsteps;
    }
    inline void setMicrosteps(int16_t value) {
        microsteps = value;
    }
    inline PH5TYPE getGearRatio() {
        return gearRatio;
    }
    inline void setGearRatio(PH5TYPE value) {
        gearRatio = value;
    }
    inline Angle3D getHomingError() {
        return eTheta;
    }
    inline void setHomingError(Angle3D value) {
        eTheta = value;
    }
    inline PH5TYPE degreePulses() {
        return steps360 * microsteps * gearRatio / 360.0;
    }
    inline PH5TYPE getZOffset() { // Z distance from base to effector with arms level (0 degrees)
        return dz;
    }
    PH5TYPE getMinZ(PH5TYPE x=0, PH5TYPE y=0);  // lowest possible point at given XY
    PH5TYPE getMinDegrees(); // base/effector arms are colinear here (which is usually bad mechanically)
    Step3D getHomePulses();
    Angle3D getHomeAngles();
    void setHomeAngles(Angle3D value);
    Step3D calcPulses(XYZ3D xyz);
    Angle3D calcAngles(XYZ3D xyz);
    XYZ3D calcXYZ(Step3D pulses);
    XYZ3D calcXYZ(Angle3D angles);
    PH5TYPE calcAngleYZ(PH5TYPE x, PH5TYPE y, PH5TYPE z);
} DeltaCalculator;

} // firestep

#endif

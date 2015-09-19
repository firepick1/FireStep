#ifdef CMAKE
#include <cstring>
#include <cmath>
#endif
#include "Arduino.h"
#include "Machine.h"
#include "AnalogRead.h"
#include "version.h"
#include "IDuino.h"
#include "DeltaCalculator.h"

using namespace firestep;
using namespace ph5;

PH5TYPE DeltaCalculator::sqrt3 = sqrt(3.0);
PH5TYPE DeltaCalculator::sin120 = sqrt3 / 2.0;
PH5TYPE DeltaCalculator::cos120 = -0.5;
PH5TYPE DeltaCalculator::tan60 = sqrt3;
PH5TYPE DeltaCalculator::sin30 = 0.5;
PH5TYPE DeltaCalculator::tan30 = 1 / sqrt3;
PH5TYPE DeltaCalculator::tan30_half = tan30 / 2.0;
PH5TYPE DeltaCalculator::pi = PI;
PH5TYPE DeltaCalculator::dtr = pi / 180.0;

StepCoord DeltaCalculator::roundStep(PH5TYPE value) {
    return (StepCoord)(value + (value < 0 ? -0.5 : +0.5));
}

DeltaCalculator::DeltaCalculator()
    : e(131.636), // effector equilateral triangle side (mm)
      f(190.526), // base equilateral triangle side (mm)
      re(270.000), // effector arm length (mm)
      rf(90.000), // base arm length (mm)
      acr(24.15), // arm clearance radius (mm)
      steps360(200),
      microsteps(16),
      spAngle(FPD_SPE_ANGLE), // sliced pulley critical angle
      spRatio(0), // sliced pulley angular error per degree beyond critical angle (default is no SPE)
      dz(0)
{
    setGearRatio(FPD_GEAR_RATIO);
    homeAngle = getDefaultHomeAngle();
}

void DeltaCalculator::useEffectorOrigin() {
    dz = 0;
    XYZ3D xyz = calcXYZ(Angle3D());
    ASSERT(xyz.isValid());

    TESTCOUT1("xyz:", xyz.isValid());
    dz = -xyz.z; // use effector origin instead of base origin at zero degrees
    TESTCOUT3("DeltaCalculator.dx:", xyz.x, " dy:", xyz.y, " dz:", dz);
    TESTCOUT2("DeltaCalculator.pulseDegrees:", getDegreesPerPulse(), " minZ:", getMinZ());
}

PH5TYPE DeltaCalculator::getMinDegrees() {
    return getDefaultHomeAngle();
}

void DeltaCalculator::setMicrosteps(int16_t value) {
    PH5TYPE scale = ((PH5TYPE)microsteps)/value;
    microsteps = value;
    degreesPerPulse[0] *= scale;
    degreesPerPulse[1] *= scale;
    degreesPerPulse[2] *= scale;
}

void DeltaCalculator::setSteps360(int16_t value) {
    PH5TYPE scale = ((PH5TYPE)steps360)/value;
    steps360 = value;
    degreesPerPulse[0] *= scale;
    degreesPerPulse[1] *= scale;
    degreesPerPulse[2] *= scale;
}

void DeltaCalculator::setGearRatio(PH5TYPE value, DeltaAxis axis) {
    setDegreesPerPulse(360/(microsteps*steps360*value), axis);
	// NOTE: setting the gear ratio will affect the home pulses,
	// and client applications need to be aware of this. This
	// method should NOT set the home angle, which is an extrinsic
	// input to FPD kinematics.
}

void DeltaCalculator::setDegreesPerPulse(PH5TYPE value, DeltaAxis axis) {
    StepCoord pulses = getHomePulses();
    if (axis == DELTA_AXIS_ALL) {
        degreesPerPulse[0] =
            degreesPerPulse[1] =
                degreesPerPulse[2] = value;
    } else {
        degreesPerPulse[axis] = value;
    }
	// NOTE: setting the degrees per pulse ratio will affect the home pulses,
	// and client applications need to be aware of this. This
	// method should NOT set the home angle, which is an extrinsic
	// input to FPD kinematics.
}

PH5TYPE DeltaCalculator::getDefaultHomeAngle() {
    PH5TYPE armsParallel = 180*asin((f-e)/(re*sqrt3))/pi - 90;
    PH5TYPE clearanceAngle = 180*asin(acr/rf)/pi;
    PH5TYPE rawDegrees =  armsParallel + clearanceAngle;
    PH5TYPE dpp = getDegreesPerPulse();
    StepCoord minPulses = rawDegrees / dpp;
    PH5TYPE degrees =  minPulses*dpp;	// digitize min degrees to stepper pulses
    //TESTCOUT2("getDefaultHomeAngle:", degrees, " raw:", rawDegrees);
    return degrees;
}

PH5TYPE DeltaCalculator::calcSPEAngle(StepCoord pulses, DeltaAxis axis) {
    PH5TYPE dpp = getDegreesPerPulse(axis);
    StepCoord criticalPulses = roundStep(spAngle/dpp);
    StepCoord spePulses = pulses - criticalPulses;
    PH5TYPE angle = pulses * getDegreesPerPulse(axis);
    if (spePulses < 0) {
        angle -= spRatio * spePulses * dpp;
    }

    return angle;
}

StepCoord DeltaCalculator::calcSPEPulses(PH5TYPE armAngle, DeltaAxis axis) {
    PH5TYPE pulseAngle = armAngle;
    PH5TYPE speAngle = armAngle - spAngle;
    if (speAngle < 0) {
        pulseAngle += speAngle * spRatio;
    }
    return roundStep(pulseAngle/getDegreesPerPulse(axis));
}

StepCoord DeltaCalculator::getHomePulses() {
    return calcSPEPulses(homeAngle);
}

void DeltaCalculator::setHomePulses(StepCoord pulses) {
    PH5TYPE angle = pulses*getDegreesPerPulse();
    PH5TYPE speAngle = angle - spAngle;
    if (speAngle < 0) {
        angle += speAngle * spRatio;
    }
    setHomeAngle(angle);
}

PH5TYPE DeltaCalculator::calcAngleYZ(PH5TYPE X, PH5TYPE Y, PH5TYPE Z) {
    PH5TYPE y1 = -tan30_half * f; // f/2 * tg 30
    Y -= tan30_half * e; // shift center to edge
    // z = a + b*y
    PH5TYPE a = (X * X + Y * Y + Z * Z + rf * rf - re * re - y1 * y1) / (2.0 * Z);
    PH5TYPE b = (y1 - Y) / Z;
    // discriminant
    PH5TYPE d = -(a + b * y1) * (a + b * y1) + rf * (b * b * rf + rf);
    if (d < 0) {
        //TESTCOUT3("***NO_SOLUTION*** DeltaCalculator calcAngleYZ X:", X, " Y:", Y, " Z:", Z);
        return NO_SOLUTION;
    }
    PH5TYPE yj = (y1 - a * b - sqrt(d)) / (b * b + 1.0); // choosing outer point
    PH5TYPE zj = a + b * yj;

    return 180.0 * atan(-zj / (y1 - yj)) / pi + ((yj > y1) ? 180.0 : 0.0);
}

Step3D DeltaCalculator::calcPulses(XYZ3D xyz) {
    Angle3D angles = calcAngles(xyz);
    if (!angles.isValid()) {
        return Step3D(false, NO_SOLUTION);
    }

    Step3D pulses(
        calcSPEPulses(angles.theta1, DELTA_AXIS_1),
        calcSPEPulses(angles.theta2, DELTA_AXIS_2),
        calcSPEPulses(angles.theta3, DELTA_AXIS_3)
    );
    return pulses;
}

Angle3D DeltaCalculator::calcAngles(XYZ3D xyz) {
    if (!xyz.isValid()) {
        return Angle3D(false, NO_SOLUTION);
    }
    PH5TYPE x = xyz.x;
    PH5TYPE y = xyz.y;
    PH5TYPE z = xyz.z - dz;
    Angle3D angles(
        calcAngleYZ(x, y, z),
        calcAngleYZ(x * cos120 + y * sin120, y * cos120 - x * sin120, z),
        calcAngleYZ(x * cos120 - y * sin120, y * cos120 + x * sin120, z)
    );
    if (angles.theta1 == NO_SOLUTION ||
            angles.theta2 == NO_SOLUTION ||
            angles.theta3 == NO_SOLUTION) {
        //TESTCOUT1("calcAngles:","NO_SOLUTION");
        return Angle3D(false, NO_SOLUTION);
    }
    //angles.theta1 += eTheta.theta1;
    //angles.theta2 += eTheta.theta2;
    //angles.theta3 += eTheta.theta3;
    return angles;
}

XYZ3D DeltaCalculator::calcXYZ(Step3D pulses) {
    if (!pulses.isValid()) {
        return XYZ3D(false, NO_SOLUTION);
    }
    Angle3D angles(
        pulses.p1 * getDegreesPerPulse(DELTA_AXIS_1),
        pulses.p2 * getDegreesPerPulse(DELTA_AXIS_2),
        pulses.p3 * getDegreesPerPulse(DELTA_AXIS_3)
    );
    return calcXYZ(angles);
}

PH5TYPE DeltaCalculator::getMinZ(PH5TYPE x, PH5TYPE y) {
    XYZ3D xyz = calcXYZ(Angle3D(90,90,90));
    xyz.x = x;
    xyz.y = y;
    TESTCOUT3("getMinZ xyz:", xyz.x, " y:", xyz.y, " z:", xyz.z);
    Step3D pulses = calcPulses(xyz);
    while (!pulses.isValid()) {
        xyz.z += 1; // TODO: actually calculate instead of flailing around
        pulses = calcPulses(xyz);
    }
    TESTCOUT3("getMinZ pulses:", pulses.p1, ", ", pulses.p2, ", ", pulses.p3);
    return xyz.z;
}

XYZ3D DeltaCalculator::calcXYZ(Angle3D angles) {
    XYZ3D xyz;
    PH5TYPE t = (f - e) * tan30 / 2;
    //PH5TYPE theta1 = (angles.theta1 - eTheta.theta1) * dtr;
    //PH5TYPE theta2 = (angles.theta2 - eTheta.theta2) * dtr;
    //PH5TYPE theta3 = (angles.theta3 - eTheta.theta3) * dtr;
    PH5TYPE theta1 = (angles.theta1) * dtr;
    PH5TYPE theta2 = (angles.theta2) * dtr;
    PH5TYPE theta3 = (angles.theta3) * dtr;
    PH5TYPE y1 = -(t + rf * cos(theta1));
    PH5TYPE z1 = -rf * sin(theta1);
    PH5TYPE y2 = (t + rf * cos(theta2)) * sin30;
    PH5TYPE x2 = y2 * tan60;
    PH5TYPE z2 = -rf * sin(theta2);
    PH5TYPE y3 = (t + rf * cos(theta3)) * sin30;
    PH5TYPE x3 = -y3 * tan60;
    PH5TYPE z3 = -rf * sin(theta3);
    PH5TYPE dnm = (y2 - y1) * x3 - (y3 - y1) * x2;
    PH5TYPE w1 = y1 * y1 + z1 * z1;
    PH5TYPE w2 = x2 * x2 + y2 * y2 + z2 * z2;
    PH5TYPE w3 = x3 * x3 + y3 * y3 + z3 * z3;
    // x = (a1*z + b1)/dnm
    PH5TYPE a1 = (z2 - z1) * (y3 - y1) - (z3 - z1) * (y2 - y1);
    PH5TYPE b1 = -((w2 - w1) * (y3 - y1) - (w3 - w1) * (y2 - y1)) / 2.0;
    // y = (a2*z + b2)/dnm
    PH5TYPE a2 = -(z2 - z1) * x3 + (z3 - z1) * x2;
    PH5TYPE b2 = ((w2 - w1) * x3 - (w3 - w1) * x2) / 2.0;
    // a*z^2 + b*z + c = 0
    PH5TYPE a = a1 * a1 + a2 * a2 + dnm * dnm;
    PH5TYPE b = 2.0 * (a1 * b1 + a2 * (b2 - y1 * dnm) - z1 * dnm * dnm);
    PH5TYPE c = (b2 - y1 * dnm) * (b2 - y1 * dnm) + b1 * b1 + dnm * dnm * (z1 * z1 - re * re);
    // discriminant
    PH5TYPE d = b * b - 4.0 * a * c;
    if (d < 0) { // point exists
        TESTCOUT3("DeltaCalculator calcXYZ() negative discriminant angles:", angles.theta1,
                  ", ", angles.theta2, ", ", angles.theta3);
        return XYZ3D(false, NO_SOLUTION);
    }
    PH5TYPE z = -0.5 * (b + sqrt(d)) / a;
    ASSERT(!isnan(dz));
    XYZ3D result(
        (a1 * z + b1) / dnm,
        (a2 * z + b2) / dnm,
        z + dz
    );

    return result;
}

int32_t DeltaCalculator::hash() {
    int32_t result = 0
                     ^ (*(uint32_t *)(void*)& f)
                     ^ (*(uint32_t *)(void*)& e)
                     ^ (*(uint32_t *)(void*)& rf)
                     ^ (*(uint32_t *)(void*)& re)
                     ^ ((int32_t)steps360 << 0)
                     ^ ((int32_t)microsteps << 1)
                     //^ (*(uint32_t *)(void*)& gearRatio)
                     ^ (*(uint32_t *)(void*)& degreesPerPulse[0])
                     ^ (*(uint32_t *)(void*)& degreesPerPulse[1])
                     ^ (*(uint32_t *)(void*)& degreesPerPulse[2])
                     ^ (*(uint32_t *)(void*)& dz)
                     ^ (*(uint32_t *)(void*)& homeAngle)
                     ;
    return result;
}

PH5TYPE DeltaCalculator::calcZBowlErrorFromETheta(Step3D center, Step3D rim, PH5TYPE eTheta) {
    StepCoord  ePulses = roundStep(eTheta / getDegreesPerPulse());
    XYZ3D xyzCtr = calcXYZ(Step3D(
                               center.p1 - ePulses,
                               center.p2 - ePulses,
                               center.p3 - ePulses
                           ));
    XYZ3D xyzRim = calcXYZ(Step3D(
                               rim.p1 - ePulses,
                               rim.p2 - ePulses,
                               rim.p3 - ePulses
                           ));
    PH5TYPE error = xyzRim.z - xyzCtr.z;
    return error;
}

PH5TYPE DeltaCalculator::calcZBowlErrorFromETheta(PH5TYPE zCenter, PH5TYPE radius, PH5TYPE eTheta) {
    XYZ3D xyzCtr(0,0,zCenter);
    XYZ3D xyzRim(radius, 0, zCenter);
    Step3D center = calcPulses(xyzCtr);
    Step3D rim = calcPulses(xyzRim);
    return calcZBowlErrorFromETheta(center, rim, eTheta);
}

PH5TYPE DeltaCalculator::calcZBowlETheta(PH5TYPE zCenter, PH5TYPE zRim, PH5TYPE radius) {
    XYZ3D xyzCtr(0, 0, zCenter);
    XYZ3D xyzRim(radius, 0, zCenter);
    Step3D center = calcPulses(xyzCtr);
    Step3D rim = calcPulses(xyzRim);

    // Newton Raphson: calculate slope@eTheta0 = ZBowl error/degree
    PH5TYPE eDegrees = 0;
    PH5TYPE goal_zErr = zRim - zCenter;
    PH5TYPE dTheta = 0.1;
    PH5TYPE best_dGoal = absval(goal_zErr);
    PH5TYPE best_eDegrees = eDegrees;
    for (int16_t i=0; i<10; i++) {
        PH5TYPE zErr = calcZBowlErrorFromETheta(center, rim, eDegrees);
        PH5TYPE zErr_delta = calcZBowlErrorFromETheta(center, rim, eDegrees+dTheta);
        PH5TYPE slope = (zErr_delta-zErr)/dTheta;
        if (absval(slope) < 0.01) {
            break;
        }
        PH5TYPE dGoal = zErr - goal_zErr;
        TESTCOUT3("dGoal:", dGoal, " eDegrees:", eDegrees, " slope:", slope);
        if (absval(dGoal) < best_dGoal) {
            best_dGoal = absval(dGoal);
            best_eDegrees = eDegrees;
            TESTCOUT2("best_dGoal:", best_dGoal, " best_eDegrees:", best_eDegrees);
        }
        eDegrees -= dGoal / slope;
    }

    TESTCOUT1("calcZBowlETheta best_eDegrees:", best_eDegrees);
    return best_eDegrees;
}

PH5TYPE DeltaCalculator::calcZBowlErrorFromGearRatio(Step3D center, Step3D rim, PH5TYPE newRatio) {
    PH5TYPE saveGearRatio = getGearRatio();
    PH5TYPE dGearRatio = newRatio - saveGearRatio;
    setGearRatio(getGearRatio(DELTA_AXIS_1)+dGearRatio, DELTA_AXIS_1);
    setGearRatio(getGearRatio(DELTA_AXIS_2)+dGearRatio, DELTA_AXIS_2);
    setGearRatio(getGearRatio(DELTA_AXIS_3)+dGearRatio, DELTA_AXIS_3);
    XYZ3D xyzCtr = calcXYZ(Step3D(center.p1, center.p2, center.p3));
    XYZ3D xyzRim = calcXYZ(Step3D(rim.p1, rim.p2, rim.p3));
    setGearRatio(getGearRatio(DELTA_AXIS_1)-dGearRatio, DELTA_AXIS_1);
    setGearRatio(getGearRatio(DELTA_AXIS_2)-dGearRatio, DELTA_AXIS_2);
    setGearRatio(getGearRatio(DELTA_AXIS_3)-dGearRatio, DELTA_AXIS_3);
    PH5TYPE error = xyzRim.z - xyzCtr.z;
    return error;
}

PH5TYPE DeltaCalculator::calcZBowlErrorFromGearRatio(PH5TYPE zCenter, PH5TYPE radius, PH5TYPE newRatio) {
    XYZ3D xyzCtr(0,0,zCenter);
    XYZ3D xyzRim(radius, 0, zCenter);
    Step3D center = calcPulses(xyzCtr);
    Step3D rim = calcPulses(xyzRim);
    return calcZBowlErrorFromGearRatio(center, rim, newRatio);
}

PH5TYPE DeltaCalculator::calcZBowlGearRatio(PH5TYPE zCenter, PH5TYPE zRim, PH5TYPE radius) {
    XYZ3D xyzCtr(0, 0, zCenter);
    XYZ3D xyzRim(radius, 0, zCenter);
    Step3D center = calcPulses(xyzCtr);
    Step3D rim = calcPulses(xyzRim);

    PH5TYPE eGearCur = 0; // gear ratio error

    // Newton Raphson: calculate slope@eTheta0 = ZBowl error/degree
    PH5TYPE eGear = eGearCur;
    PH5TYPE goalZError = zRim - zCenter;
    PH5TYPE dGear = 0.01;
    PH5TYPE newGearRatio = getGearRatio();
    PH5TYPE bestGearRatio = newGearRatio;
    PH5TYPE best_dGoal = absval(goalZError);

    for (int16_t i=0; i<25; i++) {
        PH5TYPE zError = calcZBowlErrorFromGearRatio(center, rim, newGearRatio);
        PH5TYPE zErrorDelta = calcZBowlErrorFromGearRatio(center, rim, newGearRatio+dGear);
        PH5TYPE slope = (zErrorDelta-zError)/dGear;
        PH5TYPE dGoal = zError - goalZError;
        if (absval(dGoal) < best_dGoal) {
            bestGearRatio = newGearRatio;
            best_dGoal = absval(dGoal);
            TESTCOUT2("bestGearRatio:", bestGearRatio, " best_dGoal:", best_dGoal);
        }
        eGear -= dGoal / slope;
        TESTCOUT4("dGoal:", dGoal, " newGearRatio:", newGearRatio, " slope:", slope, " zError:", zError);
        newGearRatio = newGearRatio + eGear;
        //dGear /= 2;
    }
    return bestGearRatio;
}

PH5TYPE DeltaCalculator::calcZBowlErrorFromEParam(Step3D center, Step3D rim, PH5TYPE eK, PH5TYPE &param) {
    PH5TYPE saveParam = param;
    param *= eK;
    XYZ3D xyzCtr = calcXYZ(center);
    XYZ3D xyzRim = calcXYZ(rim);
    param = saveParam;
    return xyzRim.z - xyzCtr.z;
}

PH5TYPE DeltaCalculator::calcZBowlErrorFromEParam(PH5TYPE zCenter, PH5TYPE radius, PH5TYPE eK, PH5TYPE &param) {
    XYZ3D xyzCtr(0,0,zCenter);
    XYZ3D xyzRim(radius, 0, zCenter);
    Step3D center = calcPulses(xyzCtr);
    Step3D rim = calcPulses(xyzRim);
    return calcZBowlErrorFromEParam(center, rim, eK, param);
}

PH5TYPE DeltaCalculator::calcZBowlErrorFromE_f(PH5TYPE zCenter, PH5TYPE radius, PH5TYPE eK) {
    return calcZBowlErrorFromEParam(zCenter, radius, eK, f);
}

PH5TYPE DeltaCalculator::calcZBowlErrorFromE_e(PH5TYPE zCenter, PH5TYPE radius, PH5TYPE eK) {
    return calcZBowlErrorFromEParam(zCenter, radius, eK, e);
}

PH5TYPE DeltaCalculator::calcZBowlErrorFromE_rf(PH5TYPE zCenter, PH5TYPE radius, PH5TYPE eK) {
    return calcZBowlErrorFromEParam(zCenter, radius, eK, rf);
}

PH5TYPE DeltaCalculator::calcZBowlErrorFromE_re(PH5TYPE zCenter, PH5TYPE radius, PH5TYPE eK) {
    return calcZBowlErrorFromEParam(zCenter, radius, eK, re);
}

#ifdef CMAKE
#include <cstring>
#include <cmath>
#endif
#include "Arduino.h"
#include "Machine.h"
#include "AnalogRead.h"
#include "version.h"
#include "MCU.h"
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
PH5TYPE DeltaCalculator::pi = 3.14159265359;
PH5TYPE DeltaCalculator::dtr = pi / 180.0;

StepCoord roundStep(PH5TYPE value) {
    return (StepCoord)(value + (value < 0 ? -0.5 : +0.5));
}

DeltaCalculator::DeltaCalculator()
    : e(131.636), // effector equilateral triangle side
      f(190.526), // base equilateral triangle side
      re(270.000), // effector arm length
      rf(90.000), // base arm length
      steps360(200),
      microsteps(16),
      gearRatio(150/16.0),
      dz(0)
{
}

void DeltaCalculator::setup() {
    XYZ3D xyz = calcXYZ(Angle3D());
    ASSERT(xyz.isValid());

    TESTCOUT1("xyz:", xyz.isValid());
    dz = -xyz.z; // use effector origin instead of base origin at zero degrees
    TESTCOUT3("DeltaCalculator.dx:", xyz.x, " dy:", xyz.y, " dz:", dz);
    TESTCOUT2("DeltaCalculator.degreePulses:", degreePulses(), " minZ:", getMinZ());
}

PH5TYPE DeltaCalculator::getMinDegrees() {
    PH5TYPE crf = f / sqrt3; // base circumcircle radius
    PH5TYPE minDegrees = 180*asin(crf/(re-rf))/pi - 90;
    TESTCOUT3("minDegrees:", minDegrees, " crf:", crf, " re-rf:", re-rf);
    return minDegrees;
}

void DeltaCalculator::setHomeAngles(Angle3D value) {
    PH5TYPE minDegrees = getMinDegrees();

    eTheta.theta1 = value.theta1 - minDegrees;
    eTheta.theta2 = value.theta2 - minDegrees;
    eTheta.theta3 = value.theta3 - minDegrees;
}

Angle3D DeltaCalculator::getHomeAngles() {
    PH5TYPE minDegrees = getMinDegrees();

    return Angle3D(
               minDegrees+eTheta.theta1,
               minDegrees+eTheta.theta2,
               minDegrees+eTheta.theta3
           );
}

Step3D DeltaCalculator::getHomePulses() {
    Angle3D angles(getHomeAngles());
    PH5TYPE dp = degreePulses();
    Step3D pulses(
        roundStep(angles.theta1*dp),
        roundStep(angles.theta2*dp),
        roundStep(angles.theta3*dp)
    );
    TESTCOUT3("DeltaCalculator.homeAngle:", angles.theta1, " valid:", angles.isValid(), " pulses:", pulses.p1);
    return pulses;
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
    PH5TYPE dp = degreePulses();
    Step3D pulses(
        roundStep(angles.theta1*dp),
        roundStep(angles.theta2*dp),
        roundStep(angles.theta3*dp)
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
    angles.theta1 += eTheta.theta1;
    angles.theta2 += eTheta.theta2;
    angles.theta3 += eTheta.theta3;
    return angles;
}

XYZ3D DeltaCalculator::calcXYZ(Step3D pulses) {
    if (!pulses.isValid()) {
        return XYZ3D(false, NO_SOLUTION);
    }
    PH5TYPE dp = degreePulses();
    Angle3D angles(
        pulses.p1/dp,
        pulses.p2/dp,
        pulses.p3/dp
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
    PH5TYPE theta1 = (angles.theta1 - eTheta.theta1) * dtr;
    PH5TYPE theta2 = (angles.theta2 - eTheta.theta2) * dtr;
    PH5TYPE theta3 = (angles.theta3 - eTheta.theta3) * dtr;
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
                     ^ (*(uint32_t *)(void*)& gearRatio)
                     ^ (*(uint32_t *)(void*)& dz)
                     ^ (*(uint32_t *)(void*)& eTheta.theta1)
                     ^ (*(uint32_t *)(void*)& eTheta.theta2)
                     ^ (*(uint32_t *)(void*)& eTheta.theta3)
                     ;
    return result;
}

PH5TYPE DeltaCalculator::calcZBowlError(Step3D center, Step3D rim, PH5TYPE eTheta) {
    Angle3D eThetaSave = this->eTheta;
    this->eTheta.theta1 = this->eTheta.theta2 = this->eTheta.theta3 = eTheta;
    XYZ3D xyzCtr = calcXYZ(center);
    XYZ3D xyzRim = calcXYZ(rim);
    PH5TYPE error = xyzRim.z - xyzCtr.z;
    this->eTheta = eThetaSave;
    return error;
}

PH5TYPE DeltaCalculator::calcZBowlError(PH5TYPE zCenter, PH5TYPE radius, PH5TYPE eTheta) {
    XYZ3D xyzCtr(0,0,zCenter);
    XYZ3D xyzRim(radius, 0, zCenter);
    Angle3D eThetaSave = this->eTheta;
    this->eTheta.theta1 = this->eTheta.theta2 = this->eTheta.theta3 = 0;
    Step3D center = calcPulses(xyzCtr);
    Step3D rim = calcPulses(xyzRim);
    this->eTheta = eThetaSave;
    return calcZBowlError(center, rim, eTheta);
}

PH5TYPE DeltaCalculator::calcZBowlETheta(PH5TYPE zCenter, PH5TYPE zRim, PH5TYPE radius) {
    XYZ3D xyzCtr(0,0,zCenter);
    XYZ3D xyzRim(radius, 0, zCenter);
    Step3D center = calcPulses(xyzCtr);
    Step3D rim = calcPulses(xyzRim);
    PH5TYPE eThetaCur = (eTheta.theta1+eTheta.theta2+eTheta.theta3)/3.0;

    // Newton Raphson: calculate slope@eTheta0 = ZBowl error/degree
    PH5TYPE eDegrees = eThetaCur;
    PH5TYPE zError0 = zRim - zCenter;
    PH5TYPE zErrorNext = 0;
    PH5TYPE dzError = zErrorNext - zError0;
    PH5TYPE dTheta = 1;
    for (int16_t i=0; dzError && i<6; i++) {
        PH5TYPE slope = (calcZBowlError(center, rim, eDegrees+dTheta)-zErrorNext)/dTheta;
        TESTCOUT3("eDegrees:", eDegrees, " dzError:", dzError, " slope:", slope);
        eDegrees -= dzError / slope;
        zErrorNext = calcZBowlError(center, rim, eDegrees);
        dzError = zErrorNext - zError0;
        dTheta /= 2;
    }

    return eDegrees;
}

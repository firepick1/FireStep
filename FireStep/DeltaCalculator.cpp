#ifdef CMAKE
#include <cstring>
#include <cmath>
#include <float.h>
#endif
#include "Arduino.h"
#include "Machine.h"
#include "AnalogRead.h"
#include "version.h"
#ifdef TEST
#include "../test/FireUtils.hpp"
#endif
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

StepCoord round(PH5TYPE value) { return (StepCoord)(value + (value < 0 ? -0.5 : +0.5)); }

DeltaCalculator::DeltaCalculator()
    : e(131.636), // effector equilateral triangle side
      f(190.526), // base equilateral triangle side
      re(270.000), // effector arm length
      rf(90.000), // base arm length
      steps360(200),
      microsteps(16),
      gearRatio(150/16.0)
{
	StepCoord hp = round(-67.2 * degreePulses());
	homePulses = Step3D(hp,hp,hp);

    dz = 0;
    XYZ3D xyz = calcXYZ(Angle3D());
    dz = -xyz.z;
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
		TESTCOUT3("DeltaCalculator calcAngleYZ X:", X, " Y:", Y, " Z:", Z);
		return FLT_MAX;
    }
    PH5TYPE yj = (y1 - a * b - sqrt(d)) / (b * b + 1.0); // choosing outer point
    PH5TYPE zj = a + b * yj;

    return 180.0 * atan(-zj / (y1 - yj)) / pi + ((yj > y1) ? 180.0 : 0.0);
}

Step3D DeltaCalculator::calcPulses(XYZ3D xyz) {
    Step3D pulses;
	Angle3D angles = calcAngles(xyz);
	if (!angles.isValid()) {
		return Step3D(false);
	}
    PH5TYPE dp = degreePulses();
	pulses.p1 = round(angles.theta1*dp);
	pulses.p2 = round(angles.theta2*dp);
	pulses.p3 = round(angles.theta3*dp);
	return pulses;
}

Angle3D DeltaCalculator::calcAngles(XYZ3D xyz) {
	PH5TYPE x = xyz.x;
	PH5TYPE y = xyz.y;
	PH5TYPE z = xyz.z - dz;
	Angle3D angles(
		calcAngleYZ(x, y, z),
		calcAngleYZ(x * cos120 + y * sin120, y * cos120 - x * sin120, z),
		calcAngleYZ(x * cos120 - y * sin120, y * cos120 + x * sin120, z)
	);
	if (angles.theta1 == FLT_MAX ||
		angles.theta2 == FLT_MAX ||
		angles.theta3 == FLT_MAX) {
		return Angle3D(false);
	}
	angles.theta1 += eTheta.theta1;
	angles.theta2 += eTheta.theta2;
	angles.theta3 += eTheta.theta3;
	return angles;
}

XYZ3D DeltaCalculator::calcXYZ(Step3D pulses) {
	if (!pulses.isValid()) {
		return XYZ3D(false);
	}
	PH5TYPE dp = degreePulses();
	Angle3D angles(
		pulses.p1/dp,
		pulses.p2/dp,
		pulses.p3/dp
	);
	return calcXYZ(angles);
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
        TESTCOUT3("DeltaCalculator calcXYZ() negative discriminant angles:", angles.theta1, ", ", angles.theta2, ", ", angles.theta3);
        return XYZ3D(false);
    }
    PH5TYPE z = -0.5 * (b + sqrt(d)) / a;
    return XYZ3D(
        (a1 * z + b1) / dnm,
        (a2 * z + b2) / dnm,
        z + dz
    );
}


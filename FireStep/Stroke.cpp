#ifdef CMAKE
#include <cstring>
#endif

#include "Stroke.h"

using namespace firestep;
using namespace ph5;

template class Quad<int16_t>;
template class Quad<int32_t>;

Stroke::Stroke() 
{
	clear();
}

Ticks Stroke::getTotalTicks() {
	return dtTotal;
}

float Stroke::getTimePlanned() {
	return dtTotal / (float) TICKS_PER_SECOND;
}

void Stroke::setTimePlanned(float seconds) {
	ASSERT(seconds > 0);
	dtTotal = MS_TICKS_REAL(seconds * 1000);
}

void Stroke::clear() {
	length = 0;
	maxEndPulses = 16;
	scale = 1;
	curSeg = 0;
	tStart = 0;
	dtTotal = 0;
	dPos = dEndPos = Quad<StepCoord>();
	vPeak = 0;
}

SegIndex Stroke::goalSegment(Ticks t) {
	if (t < tStart || length == 0 || dtTotal==0) {
		return 0;
	}
	Ticks dt = t - tStart;
	if (dt >= dtTotal) {
		return length-1;
	}
	Ticks dtl = dt * length;
	return (dtl + length-1) / dtTotal;
}

Ticks Stroke::goalStartTicks(Ticks t) {
	if (t < tStart || length == 0 || dtTotal==0) {
		return 0;
	}
	Ticks dt = t - tStart;
	if (dt >= dtTotal) {
		return (dtTotal*(length-1))/length;
	}
	Ticks dtl = dt * length;
	SegIndex s = (dtl + length-1) / dtTotal;
	Ticks dtEnd = (s*dtTotal)/length;

	return dtEnd;
}

Ticks Stroke::goalEndTicks(Ticks t) {
	if (t < tStart || length == 0 || dtTotal==0) {
		return 0;
	}
	Ticks dt = t - tStart;
	if (dt >= dtTotal) {
		return dtTotal;
	}
	Ticks dtl = dt * length;
	SegIndex s = (dtl + length-1) / dtTotal;
	Ticks dtEnd = ((s+1)*dtTotal)/length;

	return dtEnd;
}

Quad<StepCoord> Stroke::goalPos(Ticks t) {
	SegIndex sGoal = goalSegment(t);
	Quad<StepCoord> dGoal;
	Ticks dtSegStart = goalStartTicks(t);
	Ticks dtSegEnd = goalEndTicks(t);
	Ticks dtSeg = dtSegEnd - dtSegStart;
	Ticks dt = t - tStart;
	if (dt <= 0 || dtTotal <= 0 || length <= 0 || dtSeg <= 0) {
		// do nothing
	} else if (dtTotal <= dt && !dEndPos.isZero()) {
		TESTCOUT1("goalPos:", dEndPos.toString());
		dGoal = dEndPos;
	} else {
		dt = min(dtTotal, dt);
		Ticks tNum = (dt>dtSegEnd ? dtSegEnd:dt) - dtSegStart;
		for (QuadIndex iMotor=0; iMotor<QUAD_ELEMENTS; iMotor++) {
			StepCoord v = 0; // segment velocity
			StepCoord pos = 0;
			for (SegIndex s=0; s<sGoal; s++) {
				v += scale * (StepCoord) seg[s].value[iMotor];
				pos += v;
			}
			v += scale * (StepCoord) seg[sGoal].value[iMotor];
			dGoal.value[iMotor] = pos + (tNum * (int32_t)v) / dtSeg;
		}
	}
	return dGoal;
}

#ifdef CMAKE
template<class T> T abs(T a) { return a < 0 ? -a : a; };
#endif

Status Stroke::start(Ticks tStart) {
    this->tStart = tStart;

    if (dtTotal <= 0) {
        return STATUS_STROKE_TIME;
    }

    dPos = 0;
    if (dEndPos.isZero()) {
		dEndPos = goalPos(tStart + dtTotal);
	} else {
        Quad<StepCoord> almostEnd = goalPos(tStart + dtTotal - 1);
        for (QuadIndex i = 0; i < 4; i++) {
            if (maxEndPulses < abs(dEndPos.value[i] - almostEnd.value[i])) {
				TESTCOUT4("Stroke::start() STATUS_STROKE_END_ERROR dEndPos[", i,
					"]:", dEndPos.value[i],
					" last interpolated value:", almostEnd.value[i],
					" delta:", (dEndPos.value[i] - almostEnd.value[i]));
                return STATUS_STROKE_END_ERROR;
            }
        }
    }
	TESTCOUT2("Stroke::start() dEndPos:", dEndPos.toString(), " dtTotal:", dtTotal);
    return STATUS_OK;
}

bool Stroke::isDone() {
    return dPos == dEndPos;
}

Status Stroke::traverse(Ticks tCurrent, QuadStepper &stepper) {
    Quad<StepCoord> dGoal = goalPos(tCurrent);
    if (tStart <= 0) {
        return STATUS_STROKE_START;
    }
#ifdef TEST
	Ticks endTicks = tCurrent - (tStart+dtTotal);
	if (endTicks > -5) {
		TESTCOUT2("traverse(", endTicks, ") ", dGoal.toString());
	}
#endif
	
    Status status = STATUS_BUSY_MOVING;
	Quad<StepDV> pulse;
#define SLOWx
#ifdef SLOW
	for (bool done=true; ; done=true) {
		for (QuadIndex i = 0; i < QUAD_ELEMENTS; i++) {
            StepCoord dp = dGoal.value[i] - dPos.value[i];
			if (dp) {
				done = false;
				if (dp < 0) {
					pulse.value[i] = -1;
					dPos.value[i]--;
				} else {
					pulse.value[i] = 1;
					dPos.value[i]++;
				}
			} else {
				pulse.value[i] = 0;
			}
        }
		if (done) {
			break;
		}
        status = stepper.step(pulse);
		if (status < 0) {
            return status;	// abnormal return
        }
    }
#else
#define PULSE_BLOCK 32 /* pulses emitted without limit checks */
	Quad<StepCoord> dPosSeg = dGoal - dPos;
	for (bool done=true; ; done=true) {
		for (QuadIndex i = 0; i < QUAD_ELEMENTS; i++) {
			StepCoord dp = dPosSeg.value[i];
			if (dp < -PULSE_BLOCK) {
				pulse.value[i] = -PULSE_BLOCK;
				done = false;
			} else if (dp > PULSE_BLOCK) {
				pulse.value[i] = PULSE_BLOCK;
				done = false;
			} else if (dp) {
				pulse.value[i] = dp;
				done = false;
			} else {
				pulse.value[i] = 0;
			}
			dPosSeg.value[i] -= (StepCoord) pulse.value[i];
			dPos.value[i] += (StepCoord) pulse.value[i];
		}
		if (done) {
			break;
		}
		if (0 > (status = stepper.stepDirection(pulse))) { 
			return status;
		}
		if (0 > (status = stepper.stepFast(pulse))) {
			return status;
		}
	}
#endif
	status = (tCurrent >= tStart + dtTotal) ? STATUS_OK : STATUS_BUSY_MOVING;
    return status;
}

int16_t Stroke::append(Quad<StepDV> dv) {
    if (length >= SEGMENT_COUNT) {
        return STATUS_STROKE_MAXLEN;
    }
    seg[length++] = dv;

    return length;
}

/////////////////// StrokeBuilder ////////////////

StrokeBuilder::StrokeBuilder(int32_t vMax, float vMaxSeconds,
                             int16_t minSegments, int16_t maxSegments)
    : vMax(vMax), vMaxSeconds(vMaxSeconds),
      minSegments(minSegments), maxSegments(maxSegments) {
	if (maxSegments == 0 || SEGMENT_COUNT <= maxSegments) {
		maxSegments = SEGMENT_COUNT-1;
	}
}

/**
 * Build a rest-to-rest stroke with minimum jerk that continuously accelerates
 * until it reaches the end position located at relPos from current position
 * in the fastest possible time subject to the minimum jerk constraint
 */
Status StrokeBuilder::buildLine(Stroke & stroke, Quad<StepCoord> relPos) {
    PH5TYPE K[QUAD_ELEMENTS];
    PH5TYPE Ksqrt[QUAD_ELEMENTS];
	StepCoord pulses = 0;
    for (QuadIndex i = 0; i < QUAD_ELEMENTS; i++) {
        K[i] = relPos.value[i] / 6400.0;
		TESTCOUT2("K[", i, "]:", K[i]);
		Ksqrt[i] = sqrt(abs(K[i]));
		pulses = max(abs(relPos.value[i]), pulses);
    }
	TESTCOUT1("buildLine:", relPos.toString());
#define Z6400 56.568542495
    PHVECTOR<Complex<PH5TYPE> > z[QUAD_ELEMENTS];
    PHVECTOR<Complex<PH5TYPE> > q[QUAD_ELEMENTS];
    for (QuadIndex i = 0; i < QUAD_ELEMENTS; i++) {
		z[i].push_back(Complex<PH5TYPE>());
		if (K[i] < 0) {
			z[i].push_back(Complex<PH5TYPE>(0,Z6400 * Ksqrt[i]));
			z[i].push_back(Complex<PH5TYPE>(0,Z6400 * Ksqrt[i]));
		} else {
			z[i].push_back(Complex<PH5TYPE>(Z6400 * Ksqrt[i]));
			z[i].push_back(Complex<PH5TYPE>(Z6400 * Ksqrt[i]));
		}
        q[i].push_back(Complex<PH5TYPE>());
        q[i].push_back(Complex<PH5TYPE>(3200 * K[i]));
        q[i].push_back(Complex<PH5TYPE>(6400 * K[i]));
    }
    PH5Curve<PH5TYPE> ph[] = {
        PH5Curve<PH5TYPE>(z[0], q[0]),
        PH5Curve<PH5TYPE>(z[1], q[1]),
        PH5Curve<PH5TYPE>(z[2], q[2]),
        PH5Curve<PH5TYPE>(z[3], q[3])
    };
	TESTCOUT2("ph[0].r(0.5).Re:", ph[0].r(0.5).Re(), " Im:", ph[0].r(0.5).Im());
	TESTCOUT2("ph[0].r(1).Re:", ph[0].r(1).Re(), " Im:", ph[0].r(1).Im());
	TESTCOUT2("ph vMax:", vMax, " vMaxSeconds:", vMaxSeconds);

    PHFeed<PH5TYPE> phf[] = {
        PHFeed<PH5TYPE>(ph[0], vMax, vMaxSeconds),
        PHFeed<PH5TYPE>(ph[1], vMax, vMaxSeconds),
        PHFeed<PH5TYPE>(ph[2], vMax, vMaxSeconds),
        PHFeed<PH5TYPE>(ph[3], vMax, vMaxSeconds)
    };
    PH5TYPE tS = 0;
    QuadIndex iMax = 0;
    for (QuadIndex i = 0; i < QUAD_ELEMENTS; i++) {
		PH5TYPE tSi = phf[i].get_tS();
        if (K[i] != 0) { // active axis
			TESTCOUT3("phf[", i, "] s(1):", phf[0].s(1), " tS:", tSi);
			if (K[i] != 0 && tSi > tS) {
				iMax = i;
				tS = tSi;
			}
        }
    }
    stroke.clear();
    PH5TYPE E = 0;

    int16_t N = 1000 * tS / 40; // 40ms/segment
	int16_t minSegs = minSegments;
	if (minSegs == 0) {
		minSegs = 5; // minimum number of acceleration segments
		minSegs = (float)minSegs * (float)abs(pulses) / (vMax * vMaxSeconds);
		TESTCOUT4("pulses:", pulses, " minSegs:", minSegs, " vMax:", vMax, " vMaxSeconds:", vMaxSeconds);
		int16_t minSegsK = abs(pulses) / 200.0;
		if (minSegs < minSegsK) {
			TESTCOUT1("minSegsK:", minSegsK);
			minSegs = minSegsK;
		}
		minSegs = max((int16_t)16, min((int16_t)(SEGMENT_COUNT-1), minSegs));
	}

    N = max(minSegs, min(maxSegments, (int16_t)N)); 
	if (N >= SEGMENT_COUNT) {
        return STATUS_STROKE_MAXLEN;
	}
    Quad<StepCoord> s;
    Quad<StepCoord> v;
    Quad<StepCoord> sNew;
    Quad<StepCoord> vNew;
    Quad<StepCoord> dv;
    Quad<StepDV> segment;
	E = phf[iMax].Ekt(E, 0);
	stroke.length = N;
    for (int16_t iSeg = 1; iSeg <= N; iSeg++) {
        PH5TYPE fSeg = iSeg / (PH5TYPE)N;
        E = phf[iMax].Ekt(E, fSeg);
        for (QuadIndex i = 0; i < QUAD_ELEMENTS; i++) {
			PH5TYPE pos = ph[i].r(E).Re();
            sNew.value[i] = pos < 0 ? pos-0.5 : pos+0.5;
			vNew.value[i] = sNew.value[i] - s.value[i];
			stroke.vPeak = max(stroke.vPeak, (int32_t)abs(vNew.value[i]));
			dv.value[i] = vNew.value[i] - v.value[i];
        }
        TESTCOUT4("fSeg:", fSeg, " sNew:", sNew.toString(), 
			" vNew:", vNew.toString(), " dv:", dv.toString());
        for (QuadIndex i = 0; i < QUAD_ELEMENTS; i++) {
            if (dv.value[i] < (StepCoord) - 127 || (StepCoord) 127 < dv.value[i]) {
				TESTCOUT1(" STATUS_STROKE_SEGPULSES pulses", dv.value[i]);
                return STATUS_STROKE_SEGPULSES;
            }
            segment.value[i] = dv.value[i];
        }
		stroke.seg[iSeg-1] = segment;
        v = vNew;
        s = sNew;
    }
    TESTCOUT3(" N:", N, " tS:", tS, " dEndPos:", stroke.dEndPos.toString());
    stroke.setTimePlanned(tS);

    return STATUS_OK;
}


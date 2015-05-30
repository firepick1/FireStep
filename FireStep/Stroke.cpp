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

float Stroke::getTotalTime() {
	return dtTotal / (float) TICKS_PER_SECOND;
}

void Stroke::setTotalTime(float seconds) {
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
#ifdef TEST_TRACE
	cout << "dtEnd:" << (int32_t) dtEnd
		<< " s:" << (int32_t) s 
		<< " dtl:" << dtl 
		<< " dt:" << dt
		<< " dtTotal:" << dtTotal
		<< " length:" << (int32_t) length
		<< endl;
#endif
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
#ifdef TRACE
	cout << "dtEnd:" << (int32_t) dtEnd
		<< " s:" << (int32_t) s 
		<< " dtl:" << dtl 
		<< " dt:" << dt
		<< " dtTotal:" << dtTotal
		<< " length:" << (int32_t) length
		<< endl;
#endif
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
#ifdef TEST
		cout << "goalPos:" << dEndPos.toString() << endl;
#endif
		dGoal = dEndPos;
	} else {
		dt = min(dtTotal, dt);
		Ticks tNum = (dt>dtSegEnd ? dtSegEnd:dt) - dtSegStart;
		for (int8_t iMotor=0; iMotor<QUAD_ELEMENTS; iMotor++) {
			int16_t v = 0;
			int16_t pos = 0;
			for (SegIndex s=0; s<sGoal; s++) {
				v += scale * (StepCoord) seg[s].value[iMotor];
				pos += v;
			}
			v += scale * (StepCoord) seg[sGoal].value[iMotor];
			int32_t dSeg = v;
			dSeg *= tNum;
			dSeg /= dtSeg;
			dGoal.value[iMotor] = pos+dSeg;
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
#ifdef TEST
				cout << "Stroke::start() STATUS_STROKE_END_ERROR " 
					<< " dEndPos[" << i << "]:" << dEndPos.value[i] 
					<< " last interpolated value:" << almostEnd.value[i]
					<< " delta:" << (dEndPos.value[i] - almostEnd.value[i])
					<< endl;
#endif
                return STATUS_STROKE_END_ERROR;
            }
        }
    }
#ifdef TEST
	cout << "Stroke::start() dEndPos:" << dEndPos.toString() << " dtTotal:" << dtTotal << endl;
#endif
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
		cout << "traverse(" << endTicks << ")"
			<< dGoal.toString()
			<< endl;
	}
#endif
    Status status = STATUS_BUSY_MOVING;
    while (dPos != dGoal) {
        StepCoord d[4];
        StepCoord dMax = 0;
        for (uint8_t i = 0; i < 4; i++) {
            d[i] = dGoal.value[i] - dPos.value[i];
            dMax = max(dMax, abs(d[i]));
        }
        if (dMax == 0) {
            break;
        }
        Quad<StepCoord> pulse;
        for (uint8_t i = 0; i < 4; i++) {
            if (abs(d[i]) != dMax) {
                continue;
            }
            pulse.value[i] = d[i] < 0 ? -1 : 1;
        }
        dPos += pulse;
        Status status = stepper.step(pulse);
        switch (status) {
        case STATUS_OK:	// operation complete
        case STATUS_BUSY_MOVING: // work in progress
            break;
        default:
            return status;	// abnormal return
        }
    }
    status = tCurrent >= tStart + dtTotal ? STATUS_OK : STATUS_BUSY_MOVING;
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

StrokeBuilder::StrokeBuilder(StepCoord vMax, float vMaxSeconds,
                             int16_t minSegments, int16_t maxSegments)
    : vMax(vMax), vMaxSeconds(vMaxSeconds),
      minSegments(minSegments), maxSegments(maxSegments) {
}

/**
 * Build a rest-to-rest stroke with minimum jerk that continuously accelerates
 * until it reaches the end position located at relPos from current position
 * in the fastest possible time subject to the minimum jerk constraint
 */
Status StrokeBuilder::buildLine(Stroke & stroke, Quad<StepCoord> relPos) {
    PH5TYPE K[QUAD_ELEMENTS];
    PH5TYPE Ksqrt[QUAD_ELEMENTS];
    for (int8_t i = 0; i < QUAD_ELEMENTS; i++) {
        K[i] = relPos.value[i] / 6400.0;
        Ksqrt[i] = sqrt(K[i]);
    }
#ifdef TEST
	cout << "buildLine(" << relPos.toString() << ")" << endl;
#endif
#define Z6400 56.568542495
    PHVECTOR<Complex<PH5TYPE> > z[QUAD_ELEMENTS];
    PHVECTOR<Complex<PH5TYPE> > q[QUAD_ELEMENTS];
    for (int8_t i = 0; i < QUAD_ELEMENTS; i++) {
        z[i].push_back(Complex<PH5TYPE>());
        z[i].push_back(Complex<PH5TYPE>(Z6400 * Ksqrt[i]));
        z[i].push_back(Complex<PH5TYPE>(Z6400 * Ksqrt[i]));
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
#ifdef TEST
	cout << "ph[0].r(1):" << ph[0].r(1).Re() << endl;
#endif
    PHFeed<PH5TYPE> phf[] = {
        PHFeed<PH5TYPE>(ph[0], vMax, vMaxSeconds),
        PHFeed<PH5TYPE>(ph[1], vMax, vMaxSeconds),
        PHFeed<PH5TYPE>(ph[2], vMax, vMaxSeconds),
        PHFeed<PH5TYPE>(ph[3], vMax, vMaxSeconds)
    };
    PH5TYPE tS = 0;
    int8_t iMax = 0;
    for (int8_t i = 0; i < QUAD_ELEMENTS; i++) {
        if (phf[i].get_tS() > tS) {
            iMax = i;
            tS = phf[i].get_tS();
        }
    }
    stroke.clear();
    PH5TYPE E = 0;
    int16_t N = 1000 * tS / 20; // ~20ms timeslice
    N = max(minSegments, min(maxSegments, (int16_t)N)); 
    Quad<StepCoord> s;
    Quad<StepCoord> v;
    Quad<StepCoord> sNew;
    Quad<StepCoord> vNew;
    Quad<StepCoord> dv;
    Quad<StepDV> segment;
	E = phf[iMax].Ekt(E, 0);
    for (int16_t iSeg = 1; iSeg <= N; iSeg++) {
        PH5TYPE fSeg = iSeg / (PH5TYPE)N;
        E = phf[iMax].Ekt(E, fSeg);
        for (int8_t i = 0; i < QUAD_ELEMENTS; i++) {
            sNew.value[i] = ph[i].r(E).Re() + 0.5;
        }
        vNew = sNew - s;
        dv = vNew - v;
#ifdef TEST
        cout << "fSeg:" << fSeg
			<< " sNew:" << sNew.toString()
			<< " vNew:" << vNew.toString()
			<< " dv:" << dv.toString()
			<< endl;
#endif
        for (int8_t i = 0; i < QUAD_ELEMENTS; i++) {
            if (dv.value[i] < (StepCoord) - 127 || (StepCoord) 127 < dv.value[i]) {
#ifdef TEST
				cout << " STATUS_STROKE_SEGPULSES pulses:" << dv.value[i] << endl;
#endif
                return STATUS_STROKE_SEGPULSES;
            }
            segment.value[i] = dv.value[i];
        }
		stroke.append(segment);
        v = vNew;
        s = sNew;
    }
#ifdef TEST
    cout << " N:" << N << " tS:" << tS 
		<< " dEndPos:" << stroke.dEndPos.toString() << endl;
#endif
    stroke.setTotalTime(tS);

    return STATUS_OK;
}


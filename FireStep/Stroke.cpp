#ifdef CMAKE
#include <cstring>
#endif

#include "Stroke.h"

using namespace firestep;
using namespace ph5;

template class Quad<int16_t>;
template class Quad<int32_t>;

Stroke::Stroke() {
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
    scale = 1;
    curSeg = 0;
    tStart = 0;
    dtTotal = 0;
    dPos = dEndPos = Quad<StepCoord>();
    vPeak = 0;
}

SegIndex Stroke::goalSegment(Ticks t) {
    if (t < tStart || length == 0 || dtTotal == 0) {
        return 0;
    }
    Ticks dt = t - tStart;
    if (dt >= dtTotal) {
        return length - 1;
    }
    Ticks dtl = dt * length;
    return (dtl + length - 1) / dtTotal;
}

Ticks Stroke::goalStartTicks(Ticks t) {
    if (t < tStart || length == 0 || dtTotal == 0) {
        return 0;
    }
    Ticks dt = t - tStart;
    if (dt >= dtTotal) {
        return (dtTotal * (length - 1)) / length;
    }
    Ticks dtl = dt * length;
    SegIndex s = (dtl + length - 1) / dtTotal;
    Ticks dtEnd = (s * dtTotal) / length;

    return dtEnd;
}

Ticks Stroke::goalEndTicks(Ticks t) {
    if (t < tStart || length == 0 || dtTotal == 0) {
        return 0;
    }
    Ticks dt = t - tStart;
    if (dt >= dtTotal) {
        return dtTotal;
    }
    Ticks dtl = dt * length;
    SegIndex s = (dtl + length - 1) / dtTotal;
    Ticks dtEnd = ((s + 1) * dtTotal) / length;

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
        Ticks tNum = (dt > dtSegEnd ? dtSegEnd : dt) - dtSegStart;
        for (QuadIndex iMotor = 0; iMotor < QUAD_ELEMENTS; iMotor++) {
            StepCoord v = 0; // segment velocity
            StepCoord pos = 0;
            for (SegIndex s = 0; s < sGoal; s++) {
                v += (StepCoord) seg[s].value[iMotor];
                pos += v*scale;
            }
            v += (StepCoord) seg[sGoal].value[iMotor];
            dGoal.value[iMotor] = pos + scale*((tNum * (int32_t)v) / dtSeg);
        }
    }
    return dGoal;
}

#ifdef CMAKE
template<class T> T abs(T a) {
    return a < 0 ? -a : a;
};
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
            if (STROKE_MAX_END_PULSES < abs(dEndPos.value[i] - almostEnd.value[i])) {
                TESTCOUT4("Stroke::start() STATUS_STROKE_END_ERROR dEndPos[", (int)i,
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
    Ticks endTicks = tCurrent - (tStart + dtTotal);
    if (endTicks > -5) {
        TESTCOUT2("traverse(", endTicks, ") ", dGoal.toString());
    }
#endif

    Status status = STATUS_BUSY_MOVING;
    Quad<StepDV> pulse;
#define PULSE_BLOCK 32 /* pulses emitted without limit checks */
    Quad<StepCoord> dPosSeg = dGoal - dPos;
    for (bool done = true; ; done = true) {
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
    if (tCurrent >= tStart + dtTotal) {
        TESTCOUT3("Stroke::traverse() tCurrent:", tCurrent, " tStart:", tStart, " dtTotal:", dtTotal);
        return STATUS_OK;
    }
    return STATUS_BUSY_MOVING;
}

int16_t Stroke::append(Quad<StepDV> dv) {
    if (length >= STROKE_SEGMENTS) {
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
    if (maxSegments == 0 || STROKE_SEGMENTS <= maxSegments) {
        maxSegments = STROKE_SEGMENTS - 1;
    }
}

/**
 * Create a line by scaling a known PH5Curve to match the requested linear
 * offset.
 */
Status StrokeBuilder::buildLine(Stroke & stroke, Quad<StepCoord> relPos) {
    PH5TYPE K[QUAD_ELEMENTS];
    PH5TYPE Ksqrt[QUAD_ELEMENTS];
    StepCoord pulses = 0;
    QuadIndex iMax = 0;

    // Determine scale K for each Quad dimension
    for (QuadIndex i = 0; i < QUAD_ELEMENTS; i++) {
        K[i] = relPos.value[i] / 6400.0;
        TESTCOUT2("K[", i, "]:", K[i]);
        Ksqrt[i] = sqrt(abs(K[i]));
        if (pulses < abs(relPos.value[i])) {
            pulses = abs(relPos.value[i]);
            iMax = i;
        }
    }

    // Generate scaled PH5Curve coefficients
    TESTCOUT1("buildLine:", relPos.toString());
#define Z6400 56.568542495
    PHVECTOR<Complex<PH5TYPE> > z[QUAD_ELEMENTS];
    PHVECTOR<Complex<PH5TYPE> > q[QUAD_ELEMENTS];
    for (QuadIndex i = 0; i < QUAD_ELEMENTS; i++) {
        z[i].push_back(Complex<PH5TYPE>());
        if (K[i] < 0) {
            z[i].push_back(Complex<PH5TYPE>(0, Z6400 * Ksqrt[i]));
            z[i].push_back(Complex<PH5TYPE>(0, Z6400 * Ksqrt[i]));
        } else {
            z[i].push_back(Complex<PH5TYPE>(Z6400 * Ksqrt[i]));
            z[i].push_back(Complex<PH5TYPE>(Z6400 * Ksqrt[i]));
        }
        q[i].push_back(Complex<PH5TYPE>());
        q[i].push_back(Complex<PH5TYPE>(3200 * K[i]));
        q[i].push_back(Complex<PH5TYPE>(6400 * K[i]));
    }

    // Use the longest PH5Curve to determine the parametric value for all
    PH5Curve<PH5TYPE> phMax(z[iMax], q[iMax]);
    PHFeed<PH5TYPE> phfMax(phMax, vMax, vMaxSeconds);
    PH5TYPE tS = phfMax.get_tS();

    // Calculate the optimal number of segments using weird heuristics
#define MS_PER_SEGMENT 30
    int16_t N = 1000 * tS / MS_PER_SEGMENT;
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
        minSegs = max((int16_t)16, min((int16_t)(STROKE_SEGMENTS - 1), minSegs));
    }
    N = max(minSegs, min(maxSegments, (int16_t)N));
    if (N >= STROKE_SEGMENTS) {
        return STATUS_STROKE_MAXLEN;
    }

    // Build the stroke dimension by dimension
    stroke.clear();
    stroke.setTimePlanned(tS);
    stroke.length = N;
    PH5TYPE E[STROKE_SEGMENTS + 1];
    E[0] = phfMax.Ekt(0, 0);
    for (int16_t iSeg = 1; iSeg <= N; iSeg++) {
        PH5TYPE fSeg = iSeg / (PH5TYPE)N;
        E[iSeg] = phfMax.Ekt(E[iSeg - 1], fSeg);
    }
    for (QuadIndex i = 0; i < QUAD_ELEMENTS; i++) {
        PH5Curve<PH5TYPE> ph(z[i], q[i]);
        StepCoord s = 0;
        StepCoord v = 0;
        for (int16_t iSeg = 1; iSeg <= N; iSeg++) {
            PH5TYPE pos = ph.r(E[iSeg]).Re();
            StepCoord sNew = pos < 0 ? pos - 0.5 : pos + 0.5;
            StepCoord vNew = sNew - s;
            stroke.vPeak = max(stroke.vPeak, (int32_t)abs(vNew));
            StepCoord dv = vNew - v;
            TESTCOUT4("iSeg:", iSeg, " sNew:", sNew, " vNew:", vNew, " dv:", dv);
            if (dv < (StepCoord) - 127 || (StepCoord) 127 < dv) {
                TESTCOUT3(" STATUS_STROKE_SEGPULSES pulses:", dv, " i:", (int)i, " N:", (int)N);
                return STATUS_STROKE_SEGPULSES;
            }
            stroke.seg[iSeg - 1].value[i] = dv;
            v = vNew;
            s = sNew;
        }
    }

    leastFreeRam = min(leastFreeRam, freeRam());

    TESTCOUT3(" N:", N, " tS:", tS, " dEndPos:", stroke.dEndPos.toString());

    return STATUS_OK;
}


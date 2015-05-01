#include <cstring>

#include "Stroke.h"

using namespace firestep;

template class Quad<int16_t>;
template class Quad<int32_t>;

Stroke::Stroke() 
	: length(0), maxV(16), scale(1), curSeg(0), planMicros(0), tStart(0), dtTotal(0)
{}

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
	cout << "dtEnd:" << (long) dtEnd
		<< " s:" << (long) s 
		<< " dtl:" << dtl 
		<< " dt:" << dt
		<< " dtTotal:" << dtTotal
		<< " length:" << (long) length
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
	cout << "dtEnd:" << (long) dtEnd
		<< " s:" << (long) s 
		<< " dtl:" << dtl 
		<< " dt:" << dt
		<< " dtTotal:" << dtTotal
		<< " length:" << (long) length
		<< endl;
#endif
	return dtEnd;
}

Quad<StepCoord> Stroke::goalPos(Ticks t) {
	Quad<StepCoord> v;
	SegIndex sGoal = goalSegment(t);
	Quad<StepCoord> dGoal;
	Ticks dtSegStart = goalStartTicks(t);
	Ticks dtSegEnd = goalEndTicks(t);
	Ticks dtSeg = dtSegEnd - dtSegStart;
#ifdef TEST
	string testMsg = "goalPos";
#endif
	if (t <= tStart || dtTotal <= 0 || length <= 0 || dtSeg <= 0) {
#ifdef TEST
		testMsg += " skip";
#endif
		// do nothing
	} else if (tStart+dtTotal <= t) {
		dGoal = dEndPos;
#ifdef TEST
		testMsg += " endPos";
#endif
	} else {
		Quad<StepCoord> posSegStart;
		for (SegIndex s=0; s<sGoal; s++) {
			v += seg[s]*scale;
			posSegStart += v;
		}
		Ticks dt = t - tStart;
		Ticks tNum = (dt>dtSegEnd ? dtSegEnd:dt) - dtSegStart;
		v += seg[sGoal]*scale;
		v *= tNum;
		v /= dtSeg;
		dGoal = posSegStart+v;
#ifdef TEST
		testMsg += " goalPos";
#endif
	}
#ifdef TEST_TRACE
	cout << testMsg
			<< " t:" << t
			<< " tStart:" << tStart
			<< " dtSeg:" << dtSeg 
			<< " dtSegStart:" << dtSegStart
			<< " dtSegEnd:" << dtSegEnd
			<< " dGoal:" << dGoal.toString() 
			<< endl;
#endif
	return dGoal;
}

template<class T> T abs(T a) { return a < 0 ? -a : a; };

Status Stroke::start(Ticks tStart) {
	this->tStart = tStart;

	if (dtTotal <= 0) {
		if (planMicros <= TICK_MICROSECONDS) {
			return STATUS_STROKE_PLANMICROS;
		}
		dtTotal = (planMicros + TICK_MICROSECONDS-1) / TICK_MICROSECONDS;
	}

	dPos = 0;
	Quad<StepCoord> end = goalPos(tStart+dtTotal-1);
	for (int i=0; i<4; i++) {
		if (maxV < abs(dEndPos.value[i] - end.value[i])) {
			return STATUS_STROKE_END_ERROR;
		}
	}
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
	if (tCurrent > tStart+dtTotal) {
		return STATUS_OK;
	}
	//cout << "I dPos:" << dPos.toString() << " dGoal:" << dGoal.toString() << endl;
	while (dPos != dGoal) {
		StepCoord d[4];
		StepCoord dMax = 0;
		for (uint8_t i=0; i<4; i++) {
			d[i] = dGoal.value[i] - dPos.value[i];
			dMax = max(dMax, abs(d[i]));
		}
		if (dMax == 0) {
			break;
		}
		Quad<StepCoord> pulse;
		for (uint8_t i=0; i<4; i++) {
			if (abs(d[i]) != dMax) {
				continue;
			}
			pulse.value[i] = d[i] < 0 ? -1 : 1;
		}
		dPos += pulse;
		Status status = stepper.step(pulse);
		switch (status) {
			case STATUS_OK:			// operation complete
			case STATUS_PROCESSING:	// work in progress
				break;
			default:
				return status;	// abnormal return
		}
	//cout << "L dPos:" << dPos.toString() << " dGoal:" << dGoal.toString() << endl;
	}
	return STATUS_PROCESSING;
}



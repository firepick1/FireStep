#include <cstring>
//#include "Arduino.h"
//#include "Machine.h"
//#include "AnalogRead.h"
//#include "version.h"
//#include "build.h"

#include "Stroke.h"

using namespace firestep;

template class Quad<int16_t>;
template class Quad<int32_t>;

Stroke::Stroke() 
	: length(0), scale(1), curSeg(0), planMicros(1000000), tStart(0), tTotal(0)
{}

void Stroke::start(TICKS tStart) {
	this->tStart = tStart;
	this->dPos = 0;
	this->velocity = 0;
}

SegIndex Stroke::goalSegment(TICKS t) {
	if (t < tStart || length == 0 || tTotal==0) {
		return 0;
	}
	TICKS dt = t-tStart;
	return dt >= tTotal ? length-1 : ((dt*length) / tTotal);
}

TICKS Stroke::goalStartTicks(TICKS t) {
	if (t < tStart || length == 0 || tTotal==0) {
		return 0;
	}
	TICKS dt = t - tStart;
	TICKS dtl = (dt >= tTotal ? tTotal-1:dt) * length;
	return ((dtl/tTotal)*tTotal)/length;
}

TICKS Stroke::goalEndTicks(TICKS t) {
	if (t < tStart || length == 0 || tTotal==0) {
		return 0;
	}
	TICKS dt = t - tStart;
	TICKS dtl = (dt >= tTotal ? tTotal-1:dt) * length;
	return (((dtl+tTotal-1)/tTotal)*tTotal)/length;
}

Quad<StepCoord> Stroke::goalPos(TICKS t) {
	Quad<StepCoord> v;
	SegIndex sGoal = goalSegment(t);
	Quad<StepCoord> pos;
	if (t > tStart && tTotal > 0 && length > 0) {
		Quad<StepCoord> posSegStart;
		for (SegIndex s=0; s<sGoal; s++) {
			v += seg[s];
			posSegStart += v;
		}
		TICKS tSegStart = goalStartTicks(t);
		TICKS tSegEnd = goalEndTicks(t);
		TICKS dt = t - tStart;
		TICKS tNum = (dt>tSegEnd ? tSegEnd:dt) - tSegStart;
		TICKS tDenom = tSegEnd - tSegStart;
		v += seg[sGoal];
		v *= tNum;
		v /= tDenom;
		pos = posSegStart+v;
		cout << "goalPos tNum:" << tNum << " tDenom:" << tDenom << " v:" << v.toString() << endl;
	}
	return pos;
}

bool Stroke::traverse(TICKS tCurrent) {
	TICKS tElapsed = tCurrent - tStart;
	SegIndex sGoal = goalSegment(tCurrent);
}



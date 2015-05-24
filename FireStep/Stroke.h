#ifndef STROKE_H
#define STROKE_H

#ifdef TEST
#include "FireLog.h"
#include "FireUtils.hpp"
#endif

#include "Status.h"
#include "Thread.h"
#include "Quad.h"
#include "ph5.h" 
namespace firestep {

#define SEGMENT_COUNT 64

typedef int8_t  StepDV;			// change in StepCoord velocity
typedef int16_t StepCoord;		// stepper coordinate (i.e., pulses)
typedef uint8_t SegIndex;		// Stroke segment index [0..length)

typedef class QuadStepper {
	public:
		virtual Status step(const Quad<StepCoord> &pulse) = 0;
} QuadStepper;

typedef class Stroke {
	private:
		Quad<StepCoord> dPos;				// current offset from start position
		Ticks			dtTotal;			// ticks for planned traversal 
    public:
		Ticks			tStart;				// ticks at start of traversal
		StepCoord		maxEndPulses;		// max pulse offset for end position (default 16)
        StepCoord		scale;				// segment velocity unit
        SegIndex		curSeg;				// current segment index
        SegIndex	 	length;				// number of segments
		Quad<StepDV> 	seg[SEGMENT_COUNT];	// delta velocity 
        Quad<StepCoord>	dEndPos;			// ending offset
    public:
        Stroke();
		void clear();
		Status start(Ticks tStart);
		Status traverse(Ticks tCurrent, QuadStepper &quadStep);
		bool isDone();
		Quad<StepCoord> goalPos(Ticks t);
		Ticks goalStartTicks(Ticks t);
		Ticks goalEndTicks(Ticks t);
		SegIndex goalSegment(Ticks t);
		int16_t append(Quad<StepDV> dv);
		inline Quad<StepCoord>& position() {
			return dPos;
		}
		inline Ticks get_dtTotal() { return dtTotal; }
		float getTotalTime();
		Ticks getTotalTicks();
		void setTotalTime(float seconds);
} Stroke;

typedef class StrokeBuilder {
	public:
		StepCoord	vMax; // max pulses per second 
		float 		vMaxSeconds; // seconds to achieve vMax 
		int16_t		minSegments; // minimum number of stroke segments (default 20)
		int16_t		maxSegments; // maximum number of stroke segments (defuault 50);

	public:
		StrokeBuilder(StepCoord vMax=12800, float vMaxSeconds=0.5, 
			int16_t minSegments=20, int16_t maxSegments=50);
		Status buildLine(Stroke & stroke, Quad<StepCoord> dPos);
} StrokeBuilder;

} // namespace firestep

#endif

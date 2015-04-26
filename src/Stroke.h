#ifndef STROKE_H
#define STROKE_H

#include "Status.h"
#include "Thread.h"
#include "Quad.h"

namespace firestep {

#if defined(TEST)
#define SEGMENT_COUNT 5
#else
#define SEGMENT_COUNT 100
#endif

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
    public:
		Ticks			tStart;				// ticks at start of traversal
		StepCoord		maxV;				// max steps/tick (default 16)
        StepDV 			scale;				// segment velocity unit
		Ticks			dtTotal;			// ticks for planned traversal 
        int32_t 		planMicros;			// planned traversal time in microseconds
        SegIndex		curSeg;				// current segment index
        SegIndex	 	length;				// number of segments
		Quad<StepDV> 	seg[SEGMENT_COUNT];	// delta velocity 
        Quad<StepCoord>	dEndPos;			// ending offset
    public:
        Stroke();
		Status start(Ticks tStart);
		Status traverse(Ticks tCurrent, QuadStepper &quadStep);
		bool isDone();
		Quad<StepCoord> goalPos(Ticks t);
		Ticks goalStartTicks(Ticks t);
		Ticks goalEndTicks(Ticks t);
		SegIndex goalSegment(Ticks t);
		inline Quad<StepCoord>& position() {
			return dPos;
		}
} Stroke;

} // namespace firestep

#endif

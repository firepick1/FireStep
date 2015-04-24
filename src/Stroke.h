#ifndef STROKE_H
#define STROKE_H

#include "Status.h"
#include "Thread.h"
#include "Quad.h"

namespace firestep {

#define SEGMENT_COUNT 100

typedef int8_t  StepDV;			// change in StepCoord velocity
typedef int16_t StepCoord;		// stepper coordinate (i.e., pulses)
typedef uint8_t SegIndex;		// Stroke segment index [0..length)

typedef class QuadStepper {
	public:
		virtual Status step(const Quad<StepCoord> &pulse) = 0;
} QuadStepper;

typedef class Stroke {
    public:
        SegIndex	 	length;				// number of segments
        float 			scale;				// steps per segment
        SegIndex		curSeg;				// current segment index
        int32_t 		planMicros;			// planned traversal time in microseconds
		TICKS			tStart;				// ticks at start of traversal
		TICKS			tTotal;				// ticks for planned traversal 
		Quad<StepDV> 	seg[SEGMENT_COUNT];	// delta velocity 
        Quad<StepCoord>	velocity;			// current velocity
		Quad<StepCoord> dPos;				// current offset from start position
        Quad<StepCoord>	dPosEnd;			// ending offset
    public:
        Stroke();
		void start(TICKS tStart);
		bool traverse(TICKS tCurrent, QuadStepper &quadStep);
		Quad<StepCoord> goalPos(TICKS t);
		TICKS goalStartTicks(TICKS t);
		TICKS goalEndTicks(TICKS t);
		SegIndex goalSegment(TICKS t);
} Stroke;

} // namespace firestep

#endif

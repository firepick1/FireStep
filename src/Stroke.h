#ifndef STROKE_H
#define STROKE_H

#include "Thread.h"
#include "Quad.h"

namespace firestep {

#define SEGMENT_COUNT 100

typedef int8_t  StepDV;			// change in StepCoord velocity
typedef int16_t StepCoord;		// stepper coordinate (i.e., pulses)
typedef uint8_t SegIndex;		// Stroke segment index [0..length)

typedef class Stroke {
	class Machine;
    public:
        SegIndex	 	length;				// number of segments
        float 			scale;				// steps per segment
        SegIndex		curSeg;				// current segment index
        int32_t 		planMicros;			// planned traversal time in microseconds
		TICKS			tStart;				// ticks at start of traversal
		TICKS			tTotal;				// ticks for planned traversal 
		Quad<StepDV> 	seg[SEGMENT_COUNT];	// delta velocity 
		Quad<StepCoord> dPos;				// offset from start position
        Quad<StepCoord>	velocity;			// current velocity
        Quad<StepCoord>	endPos;				// end position
    public:
        Stroke();
		void start(TICKS tStart);
		bool traverse(TICKS tCurrent);
		Quad<StepCoord> goalPos(TICKS t);
		TICKS goalStartTicks(TICKS t);
		TICKS goalEndTicks(TICKS t);
		SegIndex goalSegment(TICKS t);
} Stroke;

} // namespace firestep

#endif

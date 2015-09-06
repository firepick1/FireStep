#ifndef STROKE_H
#define STROKE_H

//#include "FireUtils.h"
#include "Status.h"
#include "Thread.h"
#include "IDuino.h"
#include "Quad.h"
#include "ph5.h"
namespace firestep {

#define STROKE_SEGMENTS 100

// Scaled strokes usually won't end exactly at desired endpoint,
// so we allow for some extra pulses to account for that
#define STROKE_MAX_END_PULSES 110

typedef int8_t  StepDV;			// change in StepCoord velocity
typedef int16_t StepCoord;		// stepper coordinate (i.e., pulses)
typedef uint8_t SegIndex;		// Stroke segment index [0..length)

typedef class QuadStepper {
public:
    // ProtocolA: step()
    virtual Status step(const Quad<StepDV> &pulse) = 0;

    // ProtocolB: stepDirection(); stepFast();
    virtual Status stepDirection(const Quad<StepDV> &pulse) = 0;
    virtual Status stepFast(Quad<StepDV> &pulse) = 0;
} QuadStepper;

typedef class Stroke {
    friend class StrokeBuilder;
private:
    Quad<StepCoord> dPos;				// current offset from start position
    Ticks			dtTotal;			// ticks for planned traversal
public:
    Ticks			tStart;				// ticks at start of traversal
    int32_t			vPeak;				// peak velocity on any axis
    StepCoord		scale;				// segment velocity unit
    SegIndex		curSeg;				// current segment index
    SegIndex	 	length;				// number of segments
    Quad<StepDV> 	seg[STROKE_SEGMENTS];	// delta velocity
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
    inline Ticks get_dtTotal() {
        return dtTotal;
    }
    float getTimePlanned();
    Ticks getTotalTicks();
    void setTimePlanned(float seconds);
} Stroke;

typedef class StrokeBuilder {
public:
    int32_t		vMax; // max pulses per second
    float 		vMaxSeconds; // seconds to achieve vMax
    int16_t		minSegments; // minimum number of stroke segments (default 20)
    int16_t		maxSegments; // maximum number of stroke segments (defuault 50);

public:
    StrokeBuilder(int32_t vMax = 12800, float vMaxSeconds = 0.5,
                  int16_t minSegments = 0, int16_t maxSegments = 0);
    Status buildLine(Stroke & stroke, Quad<StepCoord> dPos);
} StrokeBuilder;

} // namespace firestep

#endif

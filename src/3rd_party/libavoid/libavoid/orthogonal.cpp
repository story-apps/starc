/*
 * vim: ts=4 sw=4 et tw=0 wm=0
 *
 * libavoid - Fast, Incremental, Object-avoiding Line Router
 *
 * Copyright (C) 2009-2014  Monash University
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * See the file LICENSE.LGPL distributed with the library.
 *
 * Licensees holding a valid commercial license may use this file in
 * accordance with the commercial license agreement provided with the
 * library.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author(s):  Michael Wybrow
*/


#include <cstdlib>
#include <cfloat>
#include <cmath>
#include <set>
#include <list>
#include <algorithm>

#include "libavoid/router.h"
#include "libavoid/geomtypes.h"
#include "libavoid/shape.h"
#include "libavoid/orthogonal.h"
#include "libavoid/connend.h"
#include "libavoid/connector.h"
#include "libavoid/junction.h"
#include "libavoid/vpsc.h"
#include "libavoid/assertions.h"
#include "libavoid/scanline.h"
#include "libavoid/debughandler.h"

// For debugging:
//#define NUDGE_DEBUG
//#define DEBUG_JUST_UNIFY


namespace Avoid {


// IDs:
static const int freeSegmentID  = 0;
static const int fixedSegmentID = 1;
static const int channelLeftID  = 2;
static const int channelRightID = 3;
// Weights:
static const double freeWeight   = 0.00001;
static const double strongWeight = 0.001;
static const double strongerWeight = 1.0;
static const double fixedWeight  = 100000;


// A pair of unsigned values that can be compared and used as the keys
// for sets and maps.
class UnsignedPair
{
    public:
        UnsignedPair(unsigned ind1, unsigned ind2)
        {
            COLA_ASSERT(ind1 != ind2);
            // Assign the lesser value to m_index1.
            m_index1 = (ind1 < ind2) ? ind1 : ind2;
            // Assign the greater value to m_index2.
            m_index2 = (ind1 > ind2) ? ind1 : ind2;
        }

        bool operator<(const UnsignedPair& rhs) const
        {
            if (m_index1 != rhs.m_index1)
            {
                return m_index1 < rhs.m_index1;
            }
            return m_index2 < rhs.m_index2;
        }
    private:
        unsigned short m_index1;
        unsigned short m_index2;
};
typedef std::set<UnsignedPair> UnsignedPairSet;


// Used to sort points when merging NudgingShiftSegments.
// Sorts the indexes, by point position in one dimension.
class CmpIndexes
{
    public:
        CmpIndexes(ConnRef *conn, size_t dim)
            : connRef(conn),
              dimension(dim)
        {
        }
        bool operator()(size_t lhs, size_t rhs)
        {
            return connRef->displayRoute().ps[lhs][dimension] <
                    connRef->displayRoute().ps[rhs][dimension];
        }
    private:
        ConnRef *connRef;
        size_t dimension;
};


class NudgingShiftSegment : public ShiftSegment
{
    public:
        // For shiftable segments.
        NudgingShiftSegment(ConnRef *conn, const size_t low, const size_t high,
                bool isSBend, bool isZBend, const size_t dim, double minLim,
                double maxLim)
            : ShiftSegment(dim),
              connRef(conn),
              variable(nullptr),
              fixed(false),
              finalSegment(false),
              endsInShape(false),
              singleConnectedSegment(false),
              sBend(isSBend),
              zBend(isZBend)
        {
            indexes.push_back(low);
            indexes.push_back(high);
            minSpaceLimit = minLim;
            maxSpaceLimit = maxLim;
        }
        // For fixed segments.
        NudgingShiftSegment(ConnRef *conn, const size_t low, const size_t high,
                const size_t dim)
            : ShiftSegment(dim),
              connRef(conn),
              variable(nullptr),
              fixed(true),
              finalSegment(false),
              endsInShape(false),
              singleConnectedSegment(false),
              sBend(false),
              zBend(false)
        {
            indexes.push_back(low);
            indexes.push_back(high);
            // This has no space to shift.
            minSpaceLimit = lowPoint()[dim];
            maxSpaceLimit = lowPoint()[dim];
        }
        virtual ~NudgingShiftSegment()
        {
        }
        Point& lowPoint(void)
        {
            return connRef->displayRoute().ps[indexes.front()];
        }
        Point& highPoint(void)
        {
            return connRef->displayRoute().ps[indexes.back()];
        }
        const Point& lowPoint(void) const
        {
            return connRef->displayRoute().ps[indexes.front()];
        }
        const Point& highPoint(void) const
        {
            return connRef->displayRoute().ps[indexes.back()];
        }
        double nudgeDistance(void) const
        {
            return connRef->router()->routingParameter(idealNudgingDistance);
        }
        bool immovable(void) const
        {
            return ! zigzag();
        }
        void createSolverVariable(const bool justUnifying)
        {
            bool nudgeFinalSegments = connRef->router()->routingOption(
                    nudgeOrthogonalSegmentsConnectedToShapes);
            int varID = freeSegmentID;
            double varPos = lowPoint()[dimension];
            double weight = freeWeight;
            if (nudgeFinalSegments && finalSegment)
            {
                weight = strongWeight;

                if (singleConnectedSegment && !justUnifying)
                {
                    // This is a single segment connector bridging
                    // two shapes.  So, we want to try to keep it
                    // centred rather than shift it.
                    // Don't do this during Unifying stage, or else
                    // these connectors could end up at slightly
                    // different positions and get the wrong ordering
                    // for nudging.
                    weight = strongerWeight;
                }
            }
            else if (checkpoints.size() > 0)
            {
                weight = strongWeight;
            }
            else if (zigzag())
            {
                COLA_ASSERT(minSpaceLimit > -CHANNEL_MAX);
                COLA_ASSERT(maxSpaceLimit < CHANNEL_MAX);

                // For zigzag bends, take the middle as ideal.
                varPos = minSpaceLimit + ((maxSpaceLimit - minSpaceLimit) / 2);
            }
            else if (fixed)
            {
                // Fixed segments shouldn't get moved.
                weight = fixedWeight;
                varID = fixedSegmentID;
            }
            else if ( ! finalSegment )
            {
                // Set a higher weight for c-bends to stop them sometimes
                // getting pushed out into channels by more-free connectors
                // to the "inner" side of them.
                weight = strongWeight;
            }

            variable = new Variable(varID, varPos, weight);
        }

        void updatePositionsFromSolver(const bool justUnifying)
        {
            if (fixed)
            {
                return;
            }
            double newPos = variable->finalPosition;

            // The solver can sometimes cause variables to be outside their
            // limits by a tiny amount, since all variables are held by
            // weights.  Thus, just make sure they stay in their limits.
            newPos = std::max(newPos, minSpaceLimit);
            newPos = std::min(newPos, maxSpaceLimit);

#ifdef NUDGE_DEBUG
            printf("Pos: %lX, %.16f\n", (long) connRef, newPos);
#endif
            for (size_t it = 0; it < indexes.size(); ++it)
            {
                size_t index = indexes[it];
                connRef->displayRoute().ps[index][dimension] = newPos;
            }

#ifdef DEBUGHANDLER
            if (!justUnifying && connRef->router()->debugHandler())
            {
                connRef->router()->debugHandler()->updateConnectorRoute(
                        connRef, indexes[0], indexes[indexes.size() - 1]);
            }
#endif
        }
        int fixedOrder(bool& isFixed) const
        {
            double nudgeDist = nudgeDistance();
            double pos = lowPoint()[dimension];
            bool minLimited = ((pos - minSpaceLimit) < nudgeDist);
            bool maxLimited = ((maxSpaceLimit - pos) < nudgeDist);

            if (fixed || (minLimited && maxLimited))
            {
                isFixed = true;
                return 0;
            }
            else if (minLimited)
            {
                return 1;
            }
            else if (maxLimited)
            {
                return -1;
            }
            return 0;
        }
        int order(void) const
        {
            if (lowC())
            {
                return -1;
            }
            else if (highC())
            {
                return 1;
            }
            return 0;
        }
        bool zigzag(void) const
        {
            return sBend || zBend;
        }
        // This counts segments that are collinear and share an endpoint as
        // overlapping.  This allows them to be nudged apart where possible.
        bool overlapsWith(const ShiftSegment *rhsSuper, const size_t dim) const
        {
            const NudgingShiftSegment *rhs =
                    static_cast<const NudgingShiftSegment *> (rhsSuper);
            size_t altDim = (dim + 1) % 2;
            const Point& lowPt = lowPoint();
            const Point& highPt = highPoint();
            const Point& rhsLowPt = rhs->lowPoint();
            const Point& rhsHighPt = rhs->highPoint();
            if ( (lowPt[altDim] < rhsHighPt[altDim]) &&
                    (rhsLowPt[altDim] < highPt[altDim]))
            {
                // The segments overlap.
                if ( (minSpaceLimit <= rhs->maxSpaceLimit) &&
                        (rhs->minSpaceLimit <= maxSpaceLimit) )
                {
                    return true;
                }
            }
            else if ( (lowPt[altDim] == rhsHighPt[altDim]) ||
                      (rhsLowPt[altDim] == highPt[altDim]) )
            {
                bool nudgeColinearSegments = connRef->router()->routingOption(
                        nudgeOrthogonalTouchingColinearSegments);

                if ( (minSpaceLimit <= rhs->maxSpaceLimit) &&
                        (rhs->minSpaceLimit <= maxSpaceLimit) )
                {
                    // The segments could touch at one end.
                    if (connRef->router()->routingParameter(
                            fixedSharedPathPenalty) > 0)
                    {
                        // If we are routing with a fixedSharedPathPenalty
                        // then we don't want these segments to ever touch
                        // or slide past each other, so they are always
                        // considered to be overlapping.
                        return true;
                    }
                    else if ((rhs->sBend && sBend) || (rhs->zBend && zBend))
                    {
                        // Count them as overlapping for nudging if they
                        // are both s-bends or both z-bends, i.e., when
                        // the ordering would matter.
                        return nudgeColinearSegments;
                    }
                    else if ((rhs->finalSegment && finalSegment) &&
                            (rhs->connRef == connRef))
                    {
                        return nudgeColinearSegments;
                    }
                }
            }
            return false;
        }
        // These segments are allowed to drift into alignment but don't have to.
        bool canAlignWith(const NudgingShiftSegment *rhs,
                const size_t dim) const
        {
            COLA_UNUSED(dim);

            if (connRef != rhs->connRef)
            {
                return false;
            }

            // Don't allow segments of the same connector to drift together
            // where one of them goes via a checkpoint.  We want the path
            // through the checkpoint to be maintained.
            bool hasCheckpoints = checkpoints.size() > 0;
            bool rhsHasCheckpoints = rhs->checkpoints.size() > 0;
            if (hasCheckpoints || rhsHasCheckpoints)
            {
                return false;
            }
            return true;
        }
        // These segments should align with each other.
        bool shouldAlignWith(const ShiftSegment *rhsSuper,
                const size_t dim) const
        {
            const NudgingShiftSegment *rhs =
                    static_cast<const NudgingShiftSegment *> (rhsSuper);
            if ((connRef == rhs->connRef) && finalSegment &&
                    rhs->finalSegment && overlapsWith(rhs, dim))
            {
                // If both the segments are in shapes then we know limits
                // and can align.  Otherwise we do this just for segments
                // that are very close together, since these will often
                // prevent nudging, or force it to have a tiny separation
                // value.
                if ((endsInShape && rhs->endsInShape) ||
                        (fabs(lowPoint()[dim] - rhs->lowPoint()[dim]) < 10))
                {
                    return true;
                }
            }
            else if ((connRef == rhs->connRef) &&
                     // Not both final
                     ((finalSegment & rhs->finalSegment) != true))
            {
                bool hasCheckpoints = checkpoints.size() > 0;
                bool rhsHasCheckpoints = rhs->checkpoints.size() > 0;

                if (hasCheckpoints != rhsHasCheckpoints)
                {
                    // At least one segment has checkpoints, but not both.

                    size_t altDim = (dim + 1) % 2;
                    double space = fabs(lowPoint()[dim] - rhs->lowPoint()[dim]);
                    double touchPos;
                    bool couldTouch = false;
                    if (lowPoint()[altDim] == rhs->highPoint()[altDim])
                    {
                        couldTouch = true;
                        touchPos = lowPoint()[altDim];
                    }
                    else if (highPoint()[altDim] == rhs->lowPoint()[altDim])
                    {
                        couldTouch = true;
                        touchPos = highPoint()[altDim];
                    }

                    // We should align these so long as they are close
                    // together (<= 10) and there isn't a checkpoint at the
                    // touch point, i.e., we'd be altering the edges leading
                    // into the checkpoint.  We want to keep these in place
                    // and opportunistically move other edges to align with
                    // them.
                    return couldTouch && (space <= 10) &&
                            !hasCheckpointAtPosition(touchPos, altDim) &&
                            !rhs->hasCheckpointAtPosition(touchPos, altDim);
                }
            }
            return false;
        }
        // Used for merging segments with end segments that should appear as
        // a single segment.
        void mergeWith(const ShiftSegment *rhsSuper, const size_t dim)
        {
            // Adjust limits.
            minSpaceLimit = std::max(minSpaceLimit, rhsSuper->minSpaceLimit);
            maxSpaceLimit = std::min(maxSpaceLimit, rhsSuper->maxSpaceLimit);

            // Find a new position for the segment, taking into account
            // the two original positions and the combined limits.
            double segmentPos = lowPoint()[dimension];
            double segment2Pos = rhsSuper->lowPoint()[dimension];
            if (segment2Pos < segmentPos)
            {
                segmentPos -= ((segmentPos - segment2Pos) / 2.0);
            }
            else if (segment2Pos > segmentPos)
            {
                segmentPos += ((segment2Pos - segmentPos) / 2.0);
            }
            segmentPos = std::max(minSpaceLimit, segmentPos);
            segmentPos = std::min(maxSpaceLimit, segmentPos);

            // Merge the index lists and sort the new list.
            const NudgingShiftSegment *rhs =
                    static_cast<const NudgingShiftSegment *> (rhsSuper);
            indexes.insert(indexes.end(), rhs->indexes.begin(), rhs->indexes.end());
            size_t altDim = (dim + 1) % 2;
            CmpIndexes compare(connRef, altDim);
            sort(indexes.begin(), indexes.end(), compare);

            // Apply the new positon to all points to keep them constant.
            for (size_t it = 0; it < indexes.size(); ++it)
            {
                size_t index = indexes[it];
                connRef->displayRoute().ps[index][dimension] = segmentPos;
            }
        }
        bool hasCheckpointAtPosition(const double position,
                const size_t dim) const
        {
            for (size_t cp = 0; cp < checkpoints.size(); ++cp)
            {
                if (checkpoints[cp][dim] == position)
                {
                    return true;
                }
            }
            return false;
        }

        ConnRef *connRef;
        Variable *variable;
        std::vector<size_t> indexes;
        bool fixed;
        bool finalSegment;
        bool endsInShape;
        bool singleConnectedSegment;
        std::vector<Point> checkpoints;
    private:
        bool sBend;
        bool zBend;
        bool lowC(void) const
        {
            // This is true if this is a cBend and its adjoining points
            // are at lower positions.
            if (!finalSegment && !zigzag() && !fixed &&
                    (minSpaceLimit == lowPoint()[dimension]))
            {
                return true;
            }
            return false;
        }
        bool highC(void) const
        {
            // This is true if this is a cBend and its adjoining points
            // are at higher positions.
            if (!finalSegment && !zigzag() && !fixed &&
                    (maxSpaceLimit == lowPoint()[dimension]))
            {
                return true;
            }
            return false;
        }
};



enum ScanVisDirFlag {
    VisDirNone  = 0,
    VisDirUp    = 1,
    VisDirDown  = 2
};
typedef unsigned int ScanVisDirFlags;


// Returns a bitfield of the directions of visibility in terms of the scanline
// in a particular dimension dimension.  It will return either ConnDirDown
// (meaning visibility to lower position values) or ConnDirUp (for visibility
// towards higher position values).
//
static ScanVisDirFlags getPosVertInfDirections(VertInf *v, size_t dim)
{
    if (dim == XDIM) // X-dimension
    {
        unsigned int dirs = v->visDirections & (ConnDirLeft | ConnDirRight);
        if (dirs == (ConnDirLeft | ConnDirRight))
        {
            return (VisDirDown | VisDirUp);
        }
        else if (dirs == ConnDirLeft)
        {
            return VisDirDown;
        }
        else if (dirs == ConnDirRight)
        {
            return VisDirUp;
        }
    }
    else if (dim == YDIM) // Y-dimension
    {
        unsigned int dirs = v->visDirections & (ConnDirDown | ConnDirUp);
        if (dirs == (ConnDirDown | ConnDirUp))
        {
            return (VisDirDown | VisDirUp);
        }
        else if (dirs == ConnDirDown)
        {
            // libavoid's Y-axis points downwards, so where the user has
            // specified visibility downwards, this results in visibility to
            // higher scanline positition values.
            return VisDirUp;
        }
        else if (dirs == ConnDirUp)
        {
            // libavoid's Y-axis points downwards, so where the user has
            // specified visibility upwards, this results in visibility to
            // lower scanline positition values.
            return VisDirDown;
        }
    }

    // Can occur for ConnDirNone visibility.
    return VisDirNone;
}


struct PosVertInf
{
    PosVertInf(double p, VertInf *vI, ScanVisDirFlags d = VisDirNone)
        : pos(p),
          vert(vI),
          dirs(d)
    {
    }

    bool operator<(const PosVertInf& rhs) const
    {
        if (pos != rhs.pos)
        {
            return pos < rhs.pos;
        }
        if ((vert->id == rhs.vert->id) && (vert->id == dummyOrthogID))
        {
            // Multiple dummy nodes can get placed at the same point for
            // multiple ShapeConnectionPins on junctions (outside of shapes).
            // We only need one at each position, so multiples can be seen
            // as equal here.
            return false;
        }
        if (vert->id != rhs.vert->id)
        {
            return vert->id < rhs.vert->id;
        }
        return dirs < rhs.dirs;
    }

    double pos;
    VertInf *vert;

    // A bitfield marking the direction of visibility (in this dimension)
    // made up of VisDirDown (for visibility towards lower position values)
    // and VisDirUp (for visibility towards higher position values).
    //
    ScanVisDirFlags dirs;
};


struct CmpVertInf
{
    bool operator()(const VertInf* u, const VertInf* v) const
    {
        // Comparator for VertSet, an ordered set of VertInf pointers.
        // It is assumed vertical sets of points will all have the same
        // x position and horizontal sets all share a y position, so this
        // method can be used to sort both these sets.
        COLA_ASSERT((u->point.x == v->point.x) || (u->point.y == v->point.y));
        if (u->point.x != v->point.x)
        {
            return u->point.x < v->point.x;
        }
        else if (u->point.y != v->point.y)
        {
            return u->point.y < v->point.y;
        }
        return u < v;
    }
};


typedef std::set<VertInf *, CmpVertInf> VertSet;

// A set of points to break the line segment,
// along with vertices for these points.
typedef std::set<PosVertInf> BreakpointSet;

// Temporary structure used to store the possible horizontal visibility
// lines arising from the vertical sweep.
class LineSegment
{
public:
    LineSegment(const double& b, const double& f, const double& p,
            bool ss = false, VertInf *bvi = nullptr, VertInf *fvi = nullptr)
        : begin(b),
          finish(f),
          pos(p),
          shapeSide(ss)
    {
        COLA_ASSERT(begin < finish);

        if (bvi)
        {
            vertInfs.insert(bvi);
        }
        if (fvi)
        {
            vertInfs.insert(fvi);
        }
    }
    LineSegment(const double& bf, const double& p, VertInf *bfvi = nullptr)
        : begin(bf),
          finish(bf),
          pos(p),
          shapeSide(false)
    {
        if (bfvi)
        {
            vertInfs.insert(bfvi);
        }
    }

    // Order by begin, pos, finish.
    bool operator<(const LineSegment& rhs) const
    {
        if (begin != rhs.begin)
        {
            return begin < rhs.begin;
        }
        if (pos != rhs.pos)
        {
            return pos < rhs.pos;
        }
        if (finish != rhs.finish)
        {
            return finish < rhs.finish;
        }
        COLA_ASSERT(shapeSide == rhs.shapeSide);
        return false;
    }

    bool overlaps(const LineSegment& rhs) const
    {
        if ((begin == rhs.begin) && (pos == rhs.pos) &&
                (finish == rhs.finish))
        {
            // Lines are exactly equal.
            return true;
        }

        if (pos == rhs.pos)
        {
            if (((begin >= rhs.begin) && (begin <= rhs.finish)) ||
                ((rhs.begin >= begin) && (rhs.begin <= finish)) )
            {
                // They are colinear and overlap by some amount.
                return true;
            }
        }
        return false;
    }

    void mergeVertInfs(const LineSegment& segment)
    {
        begin = std::min(begin, segment.begin);
        finish = std::max(finish, segment.finish);
        vertInfs.insert(segment.vertInfs.begin(), segment.vertInfs.end());
    }

    VertInf *beginVertInf(void) const
    {
        if (vertInfs.empty())
        {
            return nullptr;
        }
        VertInf *inf = *vertInfs.begin();
        if ( ((inf->point.y == begin) && (inf->point.x == pos)) ||
             ((inf->point.x == begin) && (inf->point.y == pos)) )
        {
            // Only return the point if it is actually at the begin pos.
            return inf;
        }
        return nullptr;
    }
    VertInf *finishVertInf(void) const
    {
        if (vertInfs.empty())
        {
            return nullptr;
        }
        VertInf *inf = *vertInfs.rbegin();
        if ( ((inf->point.y == finish) && (inf->point.x == pos)) ||
             ((inf->point.x == finish) && (inf->point.y == pos)) )
        {
            // Only return the point if it is actually at the finish pos.
            return inf;
        }
        return nullptr;
    }

    VertInf *commitPositionX(Router *router, double posX)
    {
        VertInf *found = nullptr;
        for (VertSet::iterator v = vertInfs.begin();
                v != vertInfs.end(); ++v)
        {
            if ((*v)->point.x == posX)
            {
                found = *v;
                break;
            }
        }
        if (!found)
        {
            found = new VertInf(router, dummyOrthogID, Point(posX, pos));
            vertInfs.insert(found);
        }
        return found;
    }
    // Set begin endpoint vertex if none has been assigned.
    void horiCommitBegin(Router *router, VertInf *vert = nullptr)
    {
        if (vert)
        {
            vertInfs.insert(vert);
        }

        if (vertInfs.empty() ||
                ((*vertInfs.begin())->point.x != begin))
        {
            if (begin != -DBL_MAX)
            {
                vertInfs.insert(new
                        VertInf(router, dummyOrthogID, Point(begin, pos)));
            }
        }
    }

    // Set begin endpoint vertex if none has been assigned.
    void horiCommitFinish(Router *router, VertInf *vert = nullptr)
    {
        if (vert)
        {
            vertInfs.insert(vert);
        }

        if (vertInfs.empty() ||
                ((*vertInfs.rbegin())->point.x != finish))
        {
            if (finish != DBL_MAX)
            {
                vertInfs.insert(new
                        VertInf(router, dummyOrthogID, Point(finish, pos)));
            }
        }
    }

    // Converts a section of the points list to a set of breakPoints.
    // Returns the first of the intersection points occurring at finishPos.
    VertSet::iterator addSegmentsUpTo(double finishPos)
    {
        VertSet::iterator firstIntersectionPt = vertInfs.end();
        for (VertSet::iterator vert = vertInfs.begin();
                vert != vertInfs.end(); ++vert)
        {
            if ((*vert)->point.x > finishPos)
            {
                // We're done.
                break;
            }

            breakPoints.insert(PosVertInf((*vert)->point.x, (*vert),
                        getPosVertInfDirections(*vert, XDIM)));

            if ((firstIntersectionPt == vertInfs.end()) &&
                    ((*vert)->point.x == finishPos))
            {
                firstIntersectionPt = vert;
            }
        }
        // Returns the first of the intersection points at finishPos.
        return firstIntersectionPt;
    }

    // Add visibility edge(s) for this segment.  There may be multiple if
    // one of the endpoints is shared by multiple connector endpoints.
    void addEdgeHorizontal(Router *router)
    {
        horiCommitBegin(router);
        horiCommitFinish(router);

        addSegmentsUpTo(finish);
    }

    // Set flags to show what can be passed on this visibility line.
    // This can be used later to disregard some edges in the visibility
    // graph when routing particular connectors.
    void setLongRangeVisibilityFlags(size_t dim)
    {
        // First, travel in one direction
        bool seenConnPt = false;
        bool seenShapeEdge = false;
        for (BreakpointSet::iterator nvert = breakPoints.begin();
                nvert != breakPoints.end(); ++nvert)
        {
            VertIDProps mask = 0;
            if (dim == XDIM)
            {
                if (seenConnPt)
                {
                    mask |= XL_CONN;
                }
                if (seenShapeEdge)
                {
                    mask |= XL_EDGE;
                }
            }
            else // if (dim == YDIM)
            {
                if (seenConnPt)
                {
                    mask |= YL_CONN;
                }
                if (seenShapeEdge)
                {
                    mask |= YL_EDGE;
                }
            }
            nvert->vert->orthogVisPropFlags |= mask;

            if (nvert->vert->id.isConnPt())
            {
                seenConnPt = true;
            }
            if (nvert->vert->id.isOrthShapeEdge())
            {
                seenShapeEdge = true;
            }
        }
        // Then in the other direction
        seenConnPt = false;
        seenShapeEdge = false;
        for (BreakpointSet::reverse_iterator rvert = breakPoints.rbegin();
                rvert != breakPoints.rend(); ++rvert)
        {
            VertIDProps mask = 0;
            if (dim == XDIM)
            {
                if (seenConnPt)
                {
                    mask |= XH_CONN;
                }
                if (seenShapeEdge)
                {
                    mask |= XH_EDGE;
                }
            }
            else // if (dim == YDIM)
            {
                if (seenConnPt)
                {
                    mask |= YH_CONN;
                }
                if (seenShapeEdge)
                {
                    mask |= YH_EDGE;
                }
            }
            rvert->vert->orthogVisPropFlags |= mask;

            if (rvert->vert->id.isConnPt())
            {
                seenConnPt = true;
            }
            if (rvert->vert->id.isOrthShapeEdge())
            {
                seenShapeEdge = true;
            }
        }
    }

    // Add visibility edge(s) for this segment up until an intersection.
    // Then, move the segment beginning to the intersection point, so we
    // later only consider the remainder of the segment.
    // There may be multiple segments added to the graph if the beginning
    // endpoint of the segment is shared by multiple connector endpoints.
    VertSet addEdgeHorizontalTillIntersection(Router *router,
            LineSegment& vertLine)
    {
        VertSet intersectionSet;

        horiCommitBegin(router);

        // Does a vertex already exist for this point.
        commitPositionX(router, vertLine.pos);

        // Generate segments and set end iterator to the first point
        // at the intersection position.
        VertSet::iterator restBegin = addSegmentsUpTo(vertLine.pos);

        // Add the intersections points to intersectionSet.
        VertSet::iterator restEnd = restBegin;
        while ((restEnd != vertInfs.end()) &&
                (*restEnd)->point.x == vertLine.pos)
        {
            ++restEnd;
        }
        intersectionSet.insert(restBegin, restEnd);

        // Adjust segment to remove processed portion.
        begin = vertLine.pos;
        vertInfs.erase(vertInfs.begin(), restBegin);

        return intersectionSet;
    }

    // Insert vertical breakpoints.
    void insertBreakpointsBegin(Router *router, LineSegment& vertLine)
    {
        VertInf *vert = nullptr;
        if (pos == vertLine.begin && vertLine.beginVertInf())
        {
            vert = vertLine.beginVertInf();
        }
        else if (pos == vertLine.finish && vertLine.finishVertInf())
        {
            vert = vertLine.finishVertInf();
        }
        horiCommitBegin(router, vert);

        for (VertSet::iterator v = vertInfs.begin();
                v != vertInfs.end(); ++v)
        {
            if ((*v)->point.x == begin)
            {
                vertLine.breakPoints.insert(PosVertInf(pos, *v,
                        getPosVertInfDirections(*v, YDIM)));
            }
        }
    }

    // Insert vertical breakpoints.
    void insertBreakpointsFinish(Router *router, LineSegment& vertLine)
    {
        VertInf *vert = nullptr;
        if (pos == vertLine.begin && vertLine.beginVertInf())
        {
            vert = vertLine.beginVertInf();
        }
        else if (pos == vertLine.finish && vertLine.finishVertInf())
        {
            vert = vertLine.finishVertInf();
        }
        horiCommitFinish(router, vert);

        for (VertSet::iterator v = vertInfs.begin();
                v != vertInfs.end(); ++v)
        {
            if ((*v)->point.x == finish)
            {
                vertLine.breakPoints.insert(PosVertInf(pos, *v,
                        getPosVertInfDirections(*v, YDIM)));
            }
        }
    }
    void generateVisibilityEdgesFromBreakpointSet(Router *router, size_t dim)
    {
        if (breakPoints.empty() || ((breakPoints.begin())->pos > begin))
        {
            // Add a begin point if there was not already an intersection
            // found at that point. Though, don't do this if the line
            // segment goes off to infinity -- we can't reach anything
            // by going in that direction!
            if (begin == -DBL_MAX)
            {
                // Shorten line to first intersection point.
                COLA_ASSERT(!breakPoints.empty());
                begin = breakPoints.begin()->pos;
            }
            else
            {
                // Add begin point.
                Point point(pos, pos);
                point[dim] = begin;
                VertInf *vert = new VertInf(router, dummyOrthogID, point);
                breakPoints.insert(PosVertInf(begin, vert));
            }
        }
        if (breakPoints.empty() || ((breakPoints.rbegin())->pos < finish))
        {
            // Add a finish point if there was not already an intersection
            // found at that point. Though, don't do this if the line
            // segment goes off to infinity -- we can't reach anything
            // by going in that direction!
            if (finish == DBL_MAX)
            {
                // Shorten line to last intersection point.
                finish = breakPoints.rbegin()->pos;
            }
            else
            {
                // Add finish point.
                Point point(pos, pos);
                point[dim] = finish;
                VertInf *vert = new VertInf(router, dummyOrthogID, point);
                breakPoints.insert(PosVertInf(finish, vert));
            }
        }

        // Set flags for orthogonal routing optimisation.
        setLongRangeVisibilityFlags(dim);

        const bool orthogonal = true;
        BreakpointSet::iterator vert, last;
#if 0
        last = breakPoints.end();
        for (vert = breakPoints.begin(); vert != breakPoints.end();)
        {
            if (vert->vert->id == dummyOrthogID)
            {
                if (last == breakPoints.end() ||
                        (last->vert->point != vert->vert->point))
                {
                    last = vert;
                }
                else
                {
                    // Already seen a dummy orthogonal point at this
                    // position, delete it.

            }
            else
            {
                ++vert;
            }
        }
#endif
        for (vert = last = breakPoints.begin(); vert != breakPoints.end();)
        {
            BreakpointSet::iterator firstPrev = last;
            while (last->vert->point[dim] != vert->vert->point[dim])
            {
                COLA_ASSERT(vert != last);
                // Assert points are not at the same position.
                COLA_ASSERT(vert->vert->point != last->vert->point);

                if (vert->vert->id.isConnPt() && last->vert->id.isConnPt())
                {
                    // Here we have a pair of two endpoints that are both
                    // connector endpoints and both are inside a shape.

                    // Give vert visibility back to the first non-connector
                    // endpoint vertex (i.e., the side of the shape).
                    BreakpointSet::iterator side = last;
                    while (side->vert->id.isConnPt())
                    {
                        if (side == breakPoints.begin())
                        {
                            break;
                        }
                        --side;
                    }
                    bool canSeeDown = (vert->dirs & VisDirDown);
                    if (canSeeDown && !(side->vert->id.isConnPt()))
                    {
                        EdgeInf *edge = new
                                EdgeInf(side->vert, vert->vert, orthogonal);
                        edge->setDist(vert->vert->point[dim] -
                                side->vert->point[dim]);
                    }

                    // Give last visibility back to the first non-connector
                    // endpoint vertex (i.e., the side of the shape).
                    side = vert;
                    while ((side != breakPoints.end()) &&
                            side->vert->id.isConnPt())
                    {
                        ++side;
                    }
                    bool canSeeUp = (last->dirs & VisDirUp);
                    if (canSeeUp && (side != breakPoints.end()))
                    {
                        EdgeInf *edge = new
                                EdgeInf(last->vert, side->vert, orthogonal);
                        edge->setDist(side->vert->point[dim] -
                                last->vert->point[dim]);
                    }
                }

                // The normal case.
                //
                // Note: It's okay to give two connector endpoints visibility
                // here since we only consider the partner endpoint as a
                // candidate while searching if it is the other endpoint of
                // the connector in question.
                //
                bool generateEdge = true;
                if (last->vert->id.isConnPt() && !(last->dirs & VisDirUp))
                {
                    // Don't generate the visibility segment if the ConnEnd
                    // doesn't have visibility in that direction.
                    generateEdge = false;
                }
                else if (vert->vert->id.isConnPt() && !(vert->dirs & VisDirDown))
                {
                    // Don't generate the visibility segment if the ConnEnd
                    // doesn't have visibility in that direction.
                    generateEdge = false;
                }
                if (generateEdge)
                {
                    EdgeInf *edge =
                            new EdgeInf(last->vert, vert->vert, orthogonal);
                    edge->setDist(vert->vert->point[dim] -
                            last->vert->point[dim]);
                }

                ++last;
            }

            ++vert;

            if ((vert != breakPoints.end()) &&
                    (last->vert->point[dim] == vert->vert->point[dim]))
            {
                // Still looking at same pair, just reset prev number pointer.
                last = firstPrev;
            }
            else
            {
                // vert has moved to the beginning of a group at a new
                // position.  Last is now in the right place, so do nothing.
            }
        }
    }

    double begin;
    double finish;
    double pos;

    // XXX shapeSide is unused and could possibly be removed?
    bool shapeSide;

    VertSet vertInfs;
    BreakpointSet breakPoints;
private:
    // MSVC wants to generate the assignment operator and the default
    // constructor, but fails.  Therefore we declare them private and
    // don't implement them.
    LineSegment & operator=(LineSegment const &);
    LineSegment();
};

typedef std::list<LineSegment> SegmentList;

class SegmentListWrapper
{
    public:
        LineSegment *insert(LineSegment segment)
        {
            SegmentList::iterator found = _list.end();
            for (SegmentList::iterator curr = _list.begin();
                    curr != _list.end(); ++curr)
            {
                if (curr->overlaps(segment))
                {
                    if (found != _list.end())
                    {
                        // This is not the first segment that overlaps,
                        // so we need to merge and then delete an existing
                        // segment.
                        curr->mergeVertInfs(*found);
                        _list.erase(found);
                        found = curr;
                    }
                    else
                    {
                        // This is the first overlapping segment, so just
                        // merge the new segment with this one.
                        curr->mergeVertInfs(segment);
                        found = curr;
                    }
                }
            }

            if (found == _list.end())
            {
                // Add this line.
                _list.push_back(segment);
                return &(_list.back());
            }

            return &(*found);
        }
        SegmentList& list(void)
        {
            return _list;
        }
    private:
        SegmentList _list;
};


// Given a router instance and a set of possible horizontal segments, and a
// possible vertical visibility segment, compute and add edges to the
// orthogonal visibility graph for all the visibility edges.
static void intersectSegments(Router *router, SegmentList& segments,
        LineSegment& vertLine)
{
    // XXX: It seems that this case can sometimes occur... maybe when
    // there are many overlapping rectangles.
    //COLA_ASSERT(vertLine.beginVertInf() == nullptr);
    //COLA_ASSERT(vertLine.finishVertInf() == nullptr);

    COLA_ASSERT(!segments.empty());
    for (SegmentList::iterator it = segments.begin(); it != segments.end(); )
    {
        LineSegment& horiLine = *it;

        bool inVertSegRegion = ((vertLine.begin <= horiLine.pos) &&
                                (vertLine.finish >= horiLine.pos));

        if (vertLine.pos < horiLine.begin)
        {
            // We've yet to reach this segment in the sweep, so ignore.
            ++it;
            continue;
        }
        else if (vertLine.pos == horiLine.begin)
        {
            if (inVertSegRegion)
            {
                horiLine.insertBreakpointsBegin(router, vertLine);
            }
        }
        else if (vertLine.pos == horiLine.finish)
        {
            if (inVertSegRegion)
            {
                // Add horizontal visibility segment.
                horiLine.addEdgeHorizontal(router);

                horiLine.insertBreakpointsFinish(router, vertLine);

                size_t dim = XDIM; // x-dimension
                horiLine.generateVisibilityEdgesFromBreakpointSet(router, dim);

                // And we've now finished with the segment, so delete.
                it = segments.erase(it);
                continue;
            }
        }
        else if (vertLine.pos > horiLine.finish)
        {
            // Add horizontal visibility segment.
            horiLine.addEdgeHorizontal(router);

            size_t dim = XDIM; // x-dimension
            horiLine.generateVisibilityEdgesFromBreakpointSet(router, dim);

            // We've now swept past this horizontal segment, so delete.
            it = segments.erase(it);
            continue;
        }
        else
        {
            COLA_ASSERT(vertLine.pos > horiLine.begin);
            COLA_ASSERT(vertLine.pos < horiLine.finish);

            if (inVertSegRegion)
            {
                // Add horizontal visibility segment.
                VertSet intersectionVerts =
                        horiLine.addEdgeHorizontalTillIntersection(
                            router, vertLine);

                for (VertSet::iterator v = intersectionVerts.begin();
                        v != intersectionVerts.end(); ++v)
                {
                    vertLine.breakPoints.insert(PosVertInf(horiLine.pos, *v,
                            getPosVertInfDirections(*v, YDIM)));
                }
            }
        }
        ++it;
    }

    // Split breakPoints set into visibility segments.
    size_t dimension = YDIM; // y-dimension
    vertLine.generateVisibilityEdgesFromBreakpointSet(router, dimension);
}


// Processes an event for the vertical sweep used for computing the static
// orthogonal visibility graph.  This adds possible horizontal visibility
// segments to the segments list.
// The first pass is adding the event to the scanline, the second is for
// processing the event and the third for removing it from the scanline.
static void processEventVert(Router *router, NodeSet& scanline,
        SegmentListWrapper& segments, Event *e, unsigned int pass)
{
    Node *v = e->v;

    if ( ((pass == 1) && (e->type == Open)) ||
         ((pass == 2) && (e->type == ConnPoint)) )
    {
        std::pair<NodeSet::iterator, bool> result = scanline.insert(v);
        v->iter = result.first;
        COLA_ASSERT(result.second);

        NodeSet::iterator it = v->iter;
        // Work out neighbours
        if (it != scanline.begin())
        {
            Node *u = *(--it);
            v->firstAbove = u;
            u->firstBelow = v;
        }
        it = v->iter;
        if (++it != scanline.end())
        {
            Node *u = *it;
            v->firstBelow = u;
            u->firstAbove = v;
        }
    }

    if (pass == 2)
    {
        if ((e->type == Open) || (e->type == Close))
        {
            // Only difference between Open and Close is whether the line
            // segments are at the top or bottom of the shape.  Decide here.
            double lineY = (e->type == Open) ? v->min[YDIM] : v->max[YDIM];

            // Shape edge positions.
            double minShape = v->min[XDIM];
            double maxShape = v->max[XDIM];
            // As far as we can see.
            double minLimit, maxLimit;
            double minLimitMax, maxLimitMin;
            v->findFirstPointAboveAndBelow(XDIM, lineY, minLimit, maxLimit,
                    minLimitMax, maxLimitMin);

            // Insert possible visibility segments.
            if (minLimitMax >= maxLimitMin)
            {
                // These vertices represent the shape corners.
                VertInf *vI1 = new VertInf(router, dummyOrthogShapeID,
                            Point(minShape, lineY));
                VertInf *vI2 = new VertInf(router, dummyOrthogShapeID,
                            Point(maxShape, lineY));

                // There are no overlapping shapes, so give full visibility.
                if (minLimit < minShape)
                {
                    segments.insert(LineSegment(minLimit, minShape, lineY,
                                true, nullptr, vI1));
                }
                segments.insert(LineSegment(minShape, maxShape, lineY,
                            true, vI1, vI2));
                if (maxShape < maxLimit)
                {
                    segments.insert(LineSegment(maxShape, maxLimit, lineY,
                                true, vI2, nullptr));
                }
            }
            else
            {
                // There are overlapping shapes along this shape edge.

                if ((minLimitMax > minLimit) && (minLimitMax >= minShape))
                {
                    LineSegment *line = segments.insert(
                            LineSegment(minLimit, minLimitMax, lineY, true));
                    // Shape corner:
                    VertInf *vI1 = new VertInf(router, dummyOrthogShapeID,
                                Point(minShape, lineY));
                    line->vertInfs.insert(vI1);
                }
                if ((maxLimitMin < maxLimit) && (maxLimitMin <= maxShape))
                {
                    LineSegment *line = segments.insert(
                            LineSegment(maxLimitMin, maxLimit, lineY, true));
                    // Shape corner:
                    VertInf *vI2 = new VertInf(router, dummyOrthogShapeID,
                                Point(maxShape, lineY));
                    line->vertInfs.insert(vI2);
                }
            }
        }
        else if (e->type == ConnPoint)
        {
            // Connection point.
            VertInf *centreVert = e->v->c;
            Point& cp = centreVert->point;

            // As far as we can see.
            double minLimit = v->firstPointAbove(XDIM);
            double maxLimit = v->firstPointBelow(XDIM);
            bool inShape = v->isInsideShape(XDIM);

            // Insert if we have visibility in that direction and the segment
            // length is greater than zero.
            LineSegment *line1 = nullptr, *line2 = nullptr;
            if ((centreVert->visDirections & ConnDirLeft) && (minLimit < cp.x))
            {
                line1 = segments.insert(LineSegment(minLimit, cp.x, e->pos,
                        true, nullptr, centreVert));
            }
            if ((centreVert->visDirections & ConnDirRight) && (cp.x < maxLimit))
            {
                line2 = segments.insert(LineSegment(cp.x, maxLimit, e->pos,
                        true, centreVert, nullptr));
                // If there was a line1, then we just merged with it, so
                // that pointer will be invalid (and now unnecessary).
                line1 = nullptr;
            }
            if (!line1 && !line2)
            {
                // Add a point segment for the centre point.
                segments.insert(LineSegment(cp.x, e->pos, centreVert));
            }

            if (!inShape)
            {
                // This is not contained within a shape so add a normal
                // visibility graph point here too (since paths won't route
                // *through* connector endpoint vertices).
                if (line1 || line2)
                {
                    VertInf *cent = new VertInf(router, dummyOrthogID, cp);
                    if (line1)
                    {
                        line1->vertInfs.insert(cent);
                    }
                    if (line2)
                    {
                        line2->vertInfs.insert(cent);
                    }
                }
            }
        }
    }

    if ( ((pass == 3) && (e->type == Close)) ||
         ((pass == 2) && (e->type == ConnPoint)) )
    {
        // Clean up neighbour pointers.
        Node *l = v->firstAbove, *r = v->firstBelow;
        if (l != nullptr)
        {
            l->firstBelow = v->firstBelow;
        }
        if (r != nullptr)
        {
            r->firstAbove = v->firstAbove;
        }

        if (e->type == ConnPoint)
        {
            scanline.erase(v->iter);
            delete v;
        }
        else  // if (e->type == Close)
        {
            size_t result;
            result = scanline.erase(v);
            COLA_ASSERT(result == 1);
            COLA_UNUSED(result);  // Avoid warning.
            delete v;
        }
    }
}


// Processes an event for the vertical sweep used for computing the static
// orthogonal visibility graph.  This adds possible vertical visibility
// segments to the segments list.
// The first pass is adding the event to the scanline, the second is for
// processing the event and the third for removing it from the scanline.
static void processEventHori(Router *router, NodeSet& scanline,
        SegmentListWrapper& segments, Event *e, unsigned int pass)
{
    Node *v = e->v;

    if ( ((pass == 1) && (e->type == Open)) ||
         ((pass == 2) && (e->type == ConnPoint)) )
    {
        std::pair<NodeSet::iterator, bool> result = scanline.insert(v);
        v->iter = result.first;
        COLA_ASSERT(result.second);

        NodeSet::iterator it = v->iter;
        // Work out neighbours
        if (it != scanline.begin())
        {
            Node *u = *(--it);
            v->firstAbove = u;
            u->firstBelow = v;
        }
        it = v->iter;
        if (++it != scanline.end())
        {
            Node *u = *it;
            v->firstBelow = u;
            u->firstAbove = v;
        }
    }

    if (pass == 2)
    {
        if ((e->type == Open) || (e->type == Close))
        {
            // Only difference between Open and Close is whether the line
            // segments are at the left or right of the shape.  Decide here.
            double lineX = (e->type == Open) ? v->min[XDIM] : v->max[XDIM];

            // Shape edge positions.
            double minShape = v->min[YDIM];
            double maxShape = v->max[YDIM];
            // As far as we can see.
            double minLimit, maxLimit;
            double minLimitMax, maxLimitMin;
            v->findFirstPointAboveAndBelow(YDIM, lineX, minLimit, maxLimit,
                    minLimitMax, maxLimitMin);

            if (minLimitMax >= maxLimitMin)
            {
                LineSegment *line = segments.insert(
                        LineSegment(minLimit, maxLimit, lineX));

                // Shape corners:
                VertInf *vI1 = new VertInf(router, dummyOrthogShapeID,
                        Point(lineX, minShape));
                VertInf *vI2 = new VertInf(router, dummyOrthogShapeID,
                        Point(lineX, maxShape));
                line->vertInfs.insert(vI1);
                line->vertInfs.insert(vI2);
            }
            else
            {
                if ((minLimitMax > minLimit) && (minLimitMax >= minShape))
                {
                    LineSegment *line = segments.insert(
                            LineSegment(minLimit, minLimitMax, lineX));

                    // Shape corner:
                    VertInf *vI1 = new VertInf(router, dummyOrthogShapeID,
                                Point(lineX, minShape));
                    line->vertInfs.insert(vI1);
                }
                if ((maxLimitMin < maxLimit) && (maxLimitMin <= maxShape))
                {
                    LineSegment *line = segments.insert(
                            LineSegment(maxLimitMin, maxLimit, lineX));

                    // Shape corner:
                    VertInf *vI2 = new VertInf(router, dummyOrthogShapeID,
                                Point(lineX, maxShape));
                    line->vertInfs.insert(vI2);
                }
            }
        }
        else if (e->type == ConnPoint)
        {
            // Connection point.
            VertInf *centreVert = e->v->c;
            Point& cp = centreVert->point;

            // As far as we can see.
            double minLimit = v->firstPointAbove(YDIM);
            double maxLimit = v->firstPointBelow(YDIM);

            // Insert if we have visibility in that direction and the segment
            // length is greater than zero.
            if ((centreVert->visDirections & ConnDirUp) && (minLimit < cp.y))
            {
                segments.insert(LineSegment(minLimit, cp.y, e->pos));
            }

            if ((centreVert->visDirections & ConnDirDown) && (cp.y < maxLimit))
            {
                segments.insert(LineSegment(cp.y, maxLimit, e->pos));
            }
        }
    }

    if ( ((pass == 3) && (e->type == Close)) ||
         ((pass == 2) && (e->type == ConnPoint)) )
    {
        // Clean up neighbour pointers.
        Node *l = v->firstAbove, *r = v->firstBelow;
        if (l != nullptr)
        {
            l->firstBelow = v->firstBelow;
        }
        if (r != nullptr)
        {
            r->firstAbove = v->firstAbove;
        }

        if (e->type == ConnPoint)
        {
            scanline.erase(v->iter);
            delete v;
        }
        else  // if (e->type == Close)
        {
            size_t result;
            result = scanline.erase(v);
            COLA_ASSERT(result == 1);
            COLA_UNUSED(result);  // Avoid warning.
            delete v;
        }
    }
}

// Correct visibility for pins or connector endpoints on the leading or
// trailing edge of the visibility graph which may only have visibility in
// the outward direction where there will not be a possible path.
void fixConnectionPointVisibilityOnOutsideOfVisibilityGraph(Event **events,
        size_t totalEvents, ConnDirFlags addedVisibility)
{
    if (totalEvents > 0)
    {
        double firstPos = events[0]->pos;
        size_t index = 0;
        while (index < totalEvents)
        {
            if (events[index]->pos > firstPos)
            {
                break;
            }

            if (events[index]->v->c)
            {
                events[index]->v->c->visDirections |= addedVisibility;
            }
            ++index;
        }
        index = 0;
        double lastPos = events[totalEvents - 1]->pos;
        while (index < totalEvents)
        {
            size_t revIndex = totalEvents - 1 - index;
            if (events[revIndex]->pos < lastPos)
            {
                break;
            }

            if (events[revIndex]->v->c)
            {
                events[revIndex]->v->c->visDirections |= addedVisibility;
            }
            ++index;
        }
    }
}

extern void generateStaticOrthogonalVisGraph(Router *router)
{
    const size_t n = router->m_obstacles.size();
    const unsigned cpn = router->vertices.connsSize();
    // Set up the events for the vertical sweep.
    size_t totalEvents = (2 * n) + cpn;
    Event **events = new Event*[totalEvents];
    unsigned ctr = 0;
    ObstacleList::iterator obstacleIt = router->m_obstacles.begin();
    for (unsigned i = 0; i < n; i++)
    {
        Obstacle *obstacle = *obstacleIt;
#ifndef PAPER
        JunctionRef *junction = dynamic_cast<JunctionRef *> (obstacle);
        if (junction && ! junction->positionFixed())
        {
            // Junctions that are free to move are not treated as obstacles.
            ++obstacleIt;
            totalEvents -= 2;
            continue;
        }
#endif

        Box bbox = obstacle->routingBox();
        double midX = bbox.min.x + ((bbox.max.x - bbox.min.x) / 2);
        Node *v = new Node(obstacle, midX);
        events[ctr++] = new Event(Open, v, bbox.min.y);
        events[ctr++] = new Event(Close, v, bbox.max.y);

        ++obstacleIt;
    }

#ifdef DEBUGHANDLER
    if (router->debugHandler())
    {
        std::vector<Box> obstacleBoxes;
        ObstacleList::iterator obstacleIt = router->m_obstacles.begin();
        for (unsigned i = 0; i < n; i++)
        {
            Obstacle *obstacle = *obstacleIt;
            JunctionRef *junction = dynamic_cast<JunctionRef *> (obstacle);
            if (junction && ! junction->positionFixed())
            {
                // Junctions that are free to move are not treated as obstacles.
                ++obstacleIt;
                continue;
            }
            Box bbox = obstacle->routingBox();
            obstacleBoxes.push_back(bbox);
            ++obstacleIt;
        }
        router->debugHandler()->updateObstacleBoxes(obstacleBoxes);
    }
#endif

    for (VertInf *curr = router->vertices.connsBegin();
            curr && (curr != router->vertices.shapesBegin());
            curr = curr->lstNext)
    {
        if (curr->visDirections == ConnDirNone)
        {
            // This is a connector endpoint that is attached to a connection
            // pin on a shape, so it doesn't need to be given visibility.
            // Thus, skip it and record that there is one less total event.
            --totalEvents;
            continue;
        }
        Point& point = curr->point;

        Node *v = new Node(curr, point.x);
        events[ctr++] = new Event(ConnPoint, v, point.y);
    }
    qsort((Event*)events, (size_t) totalEvents, sizeof(Event*), compare_events);

    // Correct visibility for pins or connector endpoints on the leading or
    // trailing edge of the visibility graph which may only have visibility in
    // the outward direction where there will not be a possible path.  We
    // fix this by giving them visibility left and right.
    fixConnectionPointVisibilityOnOutsideOfVisibilityGraph(events, totalEvents,
            (ConnDirLeft | ConnDirRight));

    // Process the vertical sweep -- creating cadidate horizontal edges.
    // We do multiple passes over sections of the list so we can add relevant
    // entries to the scanline that might follow, before processing them.
    SegmentListWrapper segments;
    NodeSet scanline;
    double thisPos = (totalEvents > 0) ? events[0]->pos : 0;
    unsigned int posStartIndex = 0;
    unsigned int posFinishIndex = 0;
    for (unsigned i = 0; i <= totalEvents; ++i)
    {
        // Progress reporting and continuation check.
        router->performContinuationCheck(
                TransactionPhaseOrthogonalVisibilityGraphScanX,
                i, totalEvents);

        // If we have finished the current scanline or all events, then we
        // process the events on the current scanline in a couple of passes.
        if ((i == totalEvents) || (events[i]->pos != thisPos))
        {
            posFinishIndex = i;
            for (int pass = 2; pass <= 3; ++pass)
            {
                for (unsigned j = posStartIndex; j < posFinishIndex; ++j)
                {
                    processEventVert(router, scanline, segments,
                            events[j], pass);
                }
            }

            if (i == totalEvents)
            {
                // We have cleaned up, so we can now break out of loop.
                break;
            }

            thisPos = events[i]->pos;
            posStartIndex = i;
        }

        // Do the first sweep event handling -- building the correct
        // structure of the scanline.
        const int pass = 1;
        processEventVert(router, scanline, segments, events[i], pass);
    }
    COLA_ASSERT(scanline.size() == 0);
    for (unsigned i = 0; i < totalEvents; ++i)
    {
        delete events[i];
    }

    segments.list().sort();

    // Set up the events for the horizontal sweep.
    SegmentListWrapper vertSegments;
    ctr = 0;
    obstacleIt = router->m_obstacles.begin();
    for (unsigned i = 0; i < n; i++)
    {
        Obstacle *obstacle = *obstacleIt;
#ifndef PAPER
        JunctionRef *junction = dynamic_cast<JunctionRef *> (obstacle);
        if (junction && ! junction->positionFixed())
        {
            // Junctions that are free to move are not treated as obstacles.
            ++obstacleIt;
            continue;
        }
#endif
        Box bbox = obstacle->routingBox();
        double midY = bbox.min.y + ((bbox.max.y - bbox.min.y) / 2);
        Node *v = new Node(obstacle, midY);
        events[ctr++] = new Event(Open, v, bbox.min.x);
        events[ctr++] = new Event(Close, v, bbox.max.x);

        ++obstacleIt;
    }
    for (VertInf *curr = router->vertices.connsBegin();
            curr && (curr != router->vertices.shapesBegin());
            curr = curr->lstNext)
    {
        if (curr->visDirections == ConnDirNone)
        {
            // This is a connector endpoint that is attached to a connection
            // pin on a shape, so it doesn't need to be given visibility.
            // Thus, skip it.
            continue;
        }
        Point& point = curr->point;

        Node *v = new Node(curr, point.y);
        events[ctr++] = new Event(ConnPoint, v, point.x);
    }
    qsort((Event*)events, (size_t) totalEvents, sizeof(Event*), compare_events);

    // Correct visibility for pins or connector endpoints on the leading or
    // trailing edge of the visibility graph which may only have visibility in
    // the outward direction where there will not be a possible path.  We
    // fix this by giving them visibility up and down.
    fixConnectionPointVisibilityOnOutsideOfVisibilityGraph(events, totalEvents,
            (ConnDirUp | ConnDirDown));

    // Process the horizontal sweep -- creating vertical visibility edges.
    thisPos = (totalEvents > 0) ? events[0]->pos : 0;
    posStartIndex = 0;
    for (unsigned i = 0; i <= totalEvents; ++i)
    {
        // Progress reporting and continuation check.
        router->performContinuationCheck(
                TransactionPhaseOrthogonalVisibilityGraphScanY,
                i, totalEvents);

        // If we have finished the current scanline or all events, then we
        // process the events on the current scanline in a couple of passes.
        if ((i == totalEvents) || (events[i]->pos != thisPos))
        {
            posFinishIndex = i;
            for (int pass = 2; pass <= 3; ++pass)
            {
                for (unsigned j = posStartIndex; j < posFinishIndex; ++j)
                {
                    processEventHori(router, scanline, vertSegments,
                            events[j], pass);
                }
            }

            // Process the merged line segments.
            vertSegments.list().sort();
            for (SegmentList::iterator curr = vertSegments.list().begin();
                    curr != vertSegments.list().end(); ++curr)
            {
                intersectSegments(router, segments.list(), *curr);
            }
            vertSegments.list().clear();

            if (i == totalEvents)
            {
                // We have cleaned up, so we can now break out of loop.
                break;
            }

            thisPos = events[i]->pos;
            posStartIndex = i;
        }

        // Do the first sweep event handling -- building the correct
        // structure of the scanline.
        const int pass = 1;
        processEventHori(router, scanline, vertSegments, events[i], pass);
    }
    COLA_ASSERT(scanline.size() == 0);
    for (unsigned i = 0; i < totalEvents; ++i)
    {
        delete events[i];
    }
    delete [] events;

    // Add portions of horizontal lines that are after the final vertical
    // position we considered.
    for (SegmentList::iterator it = segments.list().begin();
            it != segments.list().end(); )
    {
        LineSegment& horiLine = *it;

        horiLine.addEdgeHorizontal(router);

        size_t dim = XDIM; // x-dimension
        horiLine.generateVisibilityEdgesFromBreakpointSet(router, dim);

        it = segments.list().erase(it);
    }
}


//============================================================================
//                           Path Adjustment code
//============================================================================


typedef std::pair<Point, Point> RectBounds;

static bool insideRectBounds(const Point& point, const RectBounds& rectBounds)
{
    Point zero(0, 0);
    if ((rectBounds.first == zero) && (rectBounds.second == zero))
    {
        // We can't be inside the invalid rectangle.
        return false;
    }

    for (size_t i = 0; i < 2; ++i)
    {
        if (point[i] < rectBounds.first[i])
        {
            return false;
        }
        if (point[i] > rectBounds.second[i])
        {
            return false;
        }
    }
    return true;
}


static void buildOrthogonalNudgingSegments(Router *router,
        const size_t dim, ShiftSegmentList& segmentList)
{
    if (router->routingParameter(segmentPenalty) == 0)
    {
        // The nudging code assumes the routes are pretty optimal.  This will
        // only be true if a segment penalty is set, so just return if this
        // is not the case.
        return;
    }
    bool nudgeFinalSegments =
            router->routingOption(nudgeOrthogonalSegmentsConnectedToShapes);
    std::vector<RectBounds> shapeLimits;
    if (nudgeFinalSegments)
    {
        // If we're going to nudge final segments, then cache the shape
        // rectangles to save us rebuilding them multiple times.
        const size_t n = router->m_obstacles.size();
        shapeLimits = std::vector<RectBounds>(n);

        double zeroBufferDist = 0.0;

        ObstacleList::iterator obstacleIt = router->m_obstacles.begin();
        for (unsigned i = 0; i < n; i++)
        {
            ShapeRef *shape = dynamic_cast<ShapeRef *> (*obstacleIt);
            JunctionRef *junction = dynamic_cast<JunctionRef *> (*obstacleIt);
            if (shape)
            {
                // Take the real bounds of the shape
                Box bBox = shape->polygon().offsetBoundingBox(zeroBufferDist);
                shapeLimits[i] = std::make_pair(bBox.min, bBox.max);
            }
            else if (junction)
            {
                // Don't nudge segments attached to junctions,
                // so just use the junction position here.
                Point pos = junction->position();
                shapeLimits[i] = std::make_pair(pos, pos);
            }
            ++obstacleIt;
        }
    }

    size_t altDim = (dim + 1) % 2;
    // For each connector.
    for (ConnRefList::const_iterator curr = router->connRefs.begin();
            curr != router->connRefs.end(); ++curr)
    {
        if ((*curr)->routingType() != ConnType_Orthogonal)
        {
            continue;
        }
        Polygon& displayRoute = (*curr)->displayRoute();
        // Determine all line segments that we are interested in shifting.
        // We don't consider the first or last segment of a path.
        for (size_t i = 1; i < displayRoute.size(); ++i)
        {
            if (displayRoute.ps[i - 1][dim] == displayRoute.ps[i][dim])
            {
                // It's a segment in the dimension we are processing,
                size_t indexLow = i - 1;
                size_t indexHigh = i;
                if (displayRoute.ps[i - 1][altDim] ==
                        displayRoute.ps[i][altDim])
                {
                    // This is a zero length segment, so ignore it.
                    continue;
                }
                else if (displayRoute.ps[i - 1][altDim] >
                        displayRoute.ps[i][altDim])
                {
                    indexLow = i;
                    indexHigh = i - 1;
                }

                // Find the checkpoints on the current segment and the
                // checkpoints on the adjoining segments that aren't on
                // the corner (hence the +1 and -1 modifiers).
                std::vector<Point> checkpoints =
                        displayRoute.checkpointsOnSegment(i - 1);
                std::vector<Point> prevCheckpoints =
                        displayRoute.checkpointsOnSegment(i - 2, -1);
                std::vector<Point> nextCheckpoints =
                        displayRoute.checkpointsOnSegment(i, +1);
                bool hasCheckpoints = (checkpoints.size() > 0);
                if (hasCheckpoints && !nudgeFinalSegments)
                {
                    // This segment includes one of the routing
                    // checkpoints so we shouldn't shift it.
                    segmentList.push_back(new NudgingShiftSegment(
                            *curr, indexLow, indexHigh, dim));
                    continue;
                }

                double thisPos = displayRoute.ps[i][dim];

                if ((i == 1) || ((i + 1) == displayRoute.size()))
                {
                    // Is first or last segment of route.

                    if (nudgeFinalSegments)
                    {
                        // Determine available space for nudging these
                        // final segments.
                        double minLim = -CHANNEL_MAX;
                        double maxLim = CHANNEL_MAX;

                        // If the position of the opposite end of the
                        // attached segment is within the shape boundaries
                        // then we want to use this as an ideal position
                        // for the segment.

                        // Bitflags indicating whether this segment starts
                        // and/or ends in a shape.
                        unsigned int endsInShapes = 0;
                        // Also limit their movement to the edges of the
                        // shapes they begin or end within.
                        for (size_t k = 0; k < shapeLimits.size(); ++k)
                        {
                            double shapeMin = shapeLimits[k].first[dim];
                            double shapeMax = shapeLimits[k].second[dim];
                            if (insideRectBounds(displayRoute.ps[i - 1],
                                        shapeLimits[k]))
                            {
                                minLim = std::max(minLim, shapeMin);
                                maxLim = std::min(maxLim, shapeMax);
                                endsInShapes |= 0x01;
                            }
                            if (insideRectBounds(displayRoute.ps[i],
                                        shapeLimits[k]))
                            {
                                minLim = std::max(minLim, shapeMin);
                                maxLim = std::min(maxLim, shapeMax);
                                endsInShapes |= 0x10;
                            }
                        }

                        if ( endsInShapes == 0 )
                        {
                            // If the segment is not within a shape, then we
                            // should limit it's nudging buffer so we don't
                            // combine many unnecessary regions.
                            double pos = displayRoute.ps[i - 1][dim];
                            double freeConnBuffer = 15;
                            minLim = std::max(minLim, pos - freeConnBuffer);
                            maxLim = std::min(maxLim, pos + freeConnBuffer);
                        }

                        if ((minLim == maxLim) || (*curr)->hasFixedRoute())
                        {
                            // Fixed.
                            segmentList.push_back(new NudgingShiftSegment(*curr,
                                    indexLow, indexHigh, dim));
                        }
                        else
                        {
                            // Shiftable.
                            NudgingShiftSegment *segment = new NudgingShiftSegment(
                                    *curr, indexLow, indexHigh, false, false, dim,
                                    minLim, maxLim);
                            segment->finalSegment = true;
                            segment->endsInShape = (endsInShapes > 0);
                            if ((displayRoute.size() == 2) &&
                                    (endsInShapes == 0x11))
                            {
                                // This is a single segment connector bridging
                                // two shapes.  So, we want to try to keep the
                                // segment centred rather than shift it.
                                segment->singleConnectedSegment = true;
                            }
                            segmentList.push_back(segment);
                        }
                    }
                    else
                    {
                        // The first and last segment of a connector can't be
                        // shifted.  We call them fixed segments.
                        segmentList.push_back(new NudgingShiftSegment(*curr,
                               indexLow, indexHigh, dim));
                    }
                    continue;
                }


                // The segment probably has space to be shifted.
                double minLim = -CHANNEL_MAX;
                double maxLim = CHANNEL_MAX;

                // Constrain these segments by checkpoints along the
                // adjoining segments.  Ignore checkpoints at ends of
                // those segments.  XXX Perhaps this should not
                // affect the ideal centre position in the channel.
                for (size_t cp = 0; cp < nextCheckpoints.size(); ++cp)
                {
                    if (nextCheckpoints[cp][dim] < thisPos)
                    {
                        // Not at thisPoint, so constrain.
                        minLim = std::max(minLim, nextCheckpoints[cp][dim]);
                    }
                    else if (nextCheckpoints[cp][dim] > thisPos)
                    {
                        // Not at thisPoint, so constrain.
                        maxLim = std::min(maxLim, nextCheckpoints[cp][dim]);
                    }
                }
                for (size_t cp = 0; cp < prevCheckpoints.size(); ++cp)
                {
                    if (prevCheckpoints[cp][dim] < thisPos)
                    {
                        // Not at thisPoint, so constrain.
                        minLim = std::max(minLim, prevCheckpoints[cp][dim]);
                    }
                    else if (prevCheckpoints[cp][dim] > thisPos)
                    {
                        // Not at thisPoint, so constrain.
                        maxLim = std::min(maxLim, prevCheckpoints[cp][dim]);
                    }
                }

                bool isSBend = false;
                bool isZBend = false;

                if (checkpoints.empty())
                {
                    // Segments with checkpoints are held in place, but for
                    // other segments, we should limit their movement based
                    // on the limits of the segments at either end.

                    double prevPos = displayRoute.ps[i - 2][dim];
                    double nextPos = displayRoute.ps[i + 1][dim];
                    if ( ((prevPos < thisPos) && (nextPos > thisPos)) ||
                         ((prevPos > thisPos) && (nextPos < thisPos)) )
                    {
                        // Determine limits if the s-bend is not due to an
                        // obstacle.  In this case we need to limit the channel
                        // to the span of the adjoining segments to this one.
                        if ((prevPos < thisPos) && (nextPos > thisPos))
                        {
                            minLim = std::max(minLim, prevPos);
                            maxLim = std::min(maxLim, nextPos);
                            isZBend = true;
                        }
                        else // if ((prevPos > thisPos) && (nextPos < thisPos))
                        {
                            minLim = std::max(minLim, nextPos);
                            maxLim = std::min(maxLim, prevPos);
                            isSBend = true;
                        }
                    }
                }

                NudgingShiftSegment *nss = new NudgingShiftSegment(*curr,
                        indexLow, indexHigh, isSBend, isZBend, dim,
                        minLim, maxLim);
                nss->checkpoints = checkpoints;
                segmentList.push_back(nss);
            }
        }
    }
}


typedef std::vector<ConnRef *> ConnRefVector;
typedef std::vector<Polygon> RouteVector;


class CmpLineOrder
{
    public:
        CmpLineOrder(PtOrderMap& ord, const size_t dim)
            : orders(ord),
              dimension(dim)
        {
        }
        bool operator()(const ShiftSegment *lhsSuper,
                const ShiftSegment *rhsSuper,
                bool *comparable = nullptr) const
        {
            const NudgingShiftSegment *lhs =
                    static_cast<const NudgingShiftSegment *> (lhsSuper);
            const NudgingShiftSegment *rhs =
                    static_cast<const NudgingShiftSegment *> (rhsSuper);
            if (comparable)
            {
                *comparable = true;
            }
            Point lhsLow  = lhs->lowPoint();
            Point rhsLow  = rhs->lowPoint();
            size_t altDim = (dimension + 1) % 2;
#ifndef NDEBUG
            const Point& lhsHigh = lhs->highPoint();
            const Point& rhsHigh = rhs->highPoint();
            COLA_ASSERT(lhsLow[dimension] == lhsHigh[dimension]);
            COLA_ASSERT(rhsLow[dimension] == rhsHigh[dimension]);
#endif

            // We consider things at effectively the same position to
            // be ordered based on their order and fixedOrder, so only
            // compare segments further apart than the nudgeDistance.
            if (lhsLow[dimension] != rhsLow[dimension])
            {
                return lhsLow[dimension] < rhsLow[dimension];
            }

            // If one of these is fixed, then determine order based on
            // fixed segment, that is, order so the fixed segment doesn't
            // block movement.
            bool oneIsFixed = false;
            const int lhsFixedOrder = lhs->fixedOrder(oneIsFixed);
            const int rhsFixedOrder = rhs->fixedOrder(oneIsFixed);
            if (oneIsFixed && (lhsFixedOrder != rhsFixedOrder))
            {
                return lhsFixedOrder < rhsFixedOrder;
            }

            // C-bends that did not have a clear order with s-bends might
            // not have a good ordering here, so compare their order in
            // terms of C-bend direction and S-bends and use that if it
            // differs for the two segments.
            const int lhsOrder = lhs->order();
            const int rhsOrder = rhs->order();
            if (lhsOrder != rhsOrder)
            {
                return lhsOrder < rhsOrder;
            }

            // Need to index using the original point into the map, so find it.
            Point& unchanged = (lhsLow[altDim] > rhsLow[altDim]) ?
                    lhsLow : rhsLow;

            PtOrder& lowOrder = orders[unchanged];
            int lhsPos = lowOrder.positionFor(dimension, lhs->connRef);
            int rhsPos = lowOrder.positionFor(dimension, rhs->connRef);
            if ((lhsPos == -1) || (rhsPos == -1))
            {
                // A value for rhsPos or lhsPos mean the points are not directly
                // comparable, meaning they are at the same position but cannot
                // overlap (they are just collinear.  The relative order for
                // these segments is not important since we do not constrain
                // them against each other.
                //COLA_ASSERT(lhs->overlapsWith(rhs, dimension) == false);
                // We do need to be consistent though.
                if (comparable)
                {
                    *comparable = false;
                }
                return lhsLow[altDim] < rhsLow[altDim];
            }
            return lhsPos < rhsPos;
        }

        PtOrderMap& orders;
        const size_t dimension;
};


// We can't use the normal sort algorithm for lists since it is not possible
// to compare all elements, but there will be an ordering defined between
// most of the elements.  Hence we order these, using insertion sort, and
// the case of them not being able to be compared is handled by not setting
// up any constraints between such segments when doing the nudging.
//
static ShiftSegmentList linesort(bool nudgeFinalSegments,
        ShiftSegmentList origList, CmpLineOrder& comparison)
{
    // Cope with end segments that are getting moved and will line up with
    // other segments of the same connector.  We do this by merging them into
    // a single NudgingShiftSegment.
    if (nudgeFinalSegments)
    {
        for (ShiftSegmentList::iterator currSegIt = origList.begin();
                currSegIt != origList.end(); ++currSegIt)
        {
            for (ShiftSegmentList::iterator otherSegIt = currSegIt;
                    otherSegIt != origList.end(); )
            {
                NudgingShiftSegment *currSeg =
                        static_cast<NudgingShiftSegment *> (*currSegIt);
                NudgingShiftSegment *otherSeg =
                        static_cast<NudgingShiftSegment *> (*otherSegIt);
                if ((currSegIt != otherSegIt) && currSeg && otherSeg &&
                        currSeg->shouldAlignWith(otherSeg, comparison.dimension))
                {
                    currSeg->mergeWith(otherSeg, comparison.dimension);
                    delete otherSeg;
                    otherSegIt = origList.erase(otherSegIt);
                }
                else
                {
                    ++otherSegIt;
                }
            }
        }
    }

    ShiftSegmentList resultList;

    size_t origListSize = origList.size();
    size_t deferredN = 0;
    while (!origList.empty())
    {
        // Get and remove the first element from the origList.
        ShiftSegment *segment = origList.front();
        origList.pop_front();

        // Find the insertion point in the resultList.
        bool allComparable = true;
        ShiftSegmentList::iterator curr;
        for (curr = resultList.begin(); curr != resultList.end(); ++curr)
        {
            bool comparable = false;
            bool lessThan = comparison(segment, *curr, &comparable);
            allComparable &= comparable;

            if (comparable && lessThan)
            {
                // If it is comparable and lessThan, then we have found the
                // insertion point.
                break;
            }
        }

        if (resultList.empty() || allComparable || (deferredN >= origListSize))
        {
            // Insert the element into the resultList at the required point.
            resultList.insert(curr, segment);
            // Reset the origListSize and deferred counter.
            deferredN = 0;
            origListSize = origList.size();
        }
        else
        {
            // This wasn't comparable to anything in the sorted list,
            // so defer addition of the segment till later.
            origList.push_back(segment);
            deferredN++;
        }
    }

    return resultList;
}


typedef std::list<ShiftSegment *> ShiftSegmentPtrList;

class PotentialSegmentConstraint
{
    public:
        PotentialSegmentConstraint(size_t index1, size_t index2,
                const Variables& vs)
            : index1(index1),
              index2(index2),
              vs(vs)
        {
        }

        bool operator<(const PotentialSegmentConstraint rhs) const
        {
            return sepDistance() < rhs.sepDistance();
        }
        double sepDistance(void) const
        {
            if (!stillValid())
            {
                return 0;
            }
            return fabs(vs[index1]->finalPosition - vs[index2]->finalPosition);
        }
        bool stillValid(void) const
        {
            return (index1 != index2);
        }
        void rewriteIndex(size_t oldIndex, size_t newIndex)
        {
            if (index1 == oldIndex)
            {
                index1 = newIndex;
            }

            if (index2 == oldIndex)
            {
                index2 = newIndex;
            }
        }

        size_t index1;
        size_t index2;

    private:
        const Variables& vs;
};


class ImproveOrthogonalRoutes
{
public:
    ImproveOrthogonalRoutes(Router *router);
    void execute(void);

private:
    void simplifyOrthogonalRoutes(void);
    void buildOrthogonalNudgingOrderInfo(void);
    void nudgeOrthogonalRoutes(size_t dimension,
           bool justUnifying = false);

    Router *m_router;
    PtOrderMap m_point_orders;
    UnsignedPairSet m_shared_path_connectors_with_common_endpoints;
    ShiftSegmentList m_segment_list;
};


ImproveOrthogonalRoutes::ImproveOrthogonalRoutes(Router *router)
    : m_router(router)
{
}

void ImproveOrthogonalRoutes::execute(void)
{
    TIMER_START(m_router, tmOrthogNudge);

    m_shared_path_connectors_with_common_endpoints.clear();

    // Simplify routes.
    simplifyOrthogonalRoutes();

    // Build a cache that denotes whether a certain segment of a connector
    // contains a checkpoint.  We can't just compare positions, since routes
    // can be moved away from their original positions during nudging.
    buildConnectorRouteCheckpointCache(m_router);

    // Do Unifying first, by itself.  This greedily tries to position free
    // segments in overlapping channels at the same position.  This way they
    // have correct nudging orders determined for them since they will form
    // shared paths, rather than segments just positioned as an results of
    // the routing process.  Of course, don't do this when rerouting with
    // a fixedSharedPathPenalty since these routes include extra segments
    // we want to keep apart which prevent some shared paths.
    if (m_router->routingOption(performUnifyingNudgingPreprocessingStep) &&
            (m_router->routingParameter(fixedSharedPathPenalty) == 0))
    {
        for (size_t dimension = 0; dimension < 2; ++dimension)
        {
            // Just perform Unifying operation.
            bool justUnifying = true;
            m_segment_list.clear();
            buildOrthogonalNudgingSegments(m_router, dimension, m_segment_list);
            buildOrthogonalChannelInfo(m_router, dimension, m_segment_list);
            nudgeOrthogonalRoutes(dimension, justUnifying);
        }
    }

#ifndef DEBUG_JUST_UNIFY
    // Do the Nudging and centring.
    for (size_t dimension = 0; dimension < 2; ++dimension)
    {
        m_point_orders.clear();
        // Build nudging info.
        // XXX Needs to be rebuilt for each dimension, cause of shifting
        //     points.  Maybe we could modify the point orders.
        buildOrthogonalNudgingOrderInfo();

        // Do the centring and nudging.
        m_segment_list.clear();
        buildOrthogonalNudgingSegments(m_router, dimension, m_segment_list);
        buildOrthogonalChannelInfo(m_router, dimension, m_segment_list);
        nudgeOrthogonalRoutes(dimension);
    }
#endif // DEBUG_JUST_UNIFY

    // Resimplify all the display routes that may have been split.
    simplifyOrthogonalRoutes();

    m_router->improveOrthogonalTopology();

    // Clear the segment-checkpoint cache for connectors.
    clearConnectorRouteCheckpointCache(m_router);

    TIMER_STOP(m_router);
}

void ImproveOrthogonalRoutes::nudgeOrthogonalRoutes(size_t dimension,
       bool justUnifying)
{
    bool nudgeFinalSegments = m_router->routingOption(
            nudgeOrthogonalSegmentsConnectedToShapes);
    bool nudgeSharedPathsWithCommonEnd = m_router->routingOption(
            nudgeSharedPathsWithCommonEndPoint);
    double baseSepDist = m_router->routingParameter(idealNudgingDistance);
    COLA_ASSERT(baseSepDist >= 0);
    // If we can fit things with the desired separation distance, then
    // we try 10 times, reducing each time by a 10th of the original amount.
    double reductionSteps = 10.0;

    size_t totalSegmentsToShift = m_segment_list.size();
    size_t numOfSegmentsShifted = 0;
    // Do the actual nudging.
    ShiftSegmentList currentRegion;
    while (!m_segment_list.empty())
    {
        // Progress reporting and continuation check.
        numOfSegmentsShifted = totalSegmentsToShift - m_segment_list.size();
        m_router->performContinuationCheck(
                (dimension == XDIM) ? TransactionPhaseOrthogonalNudgingX :
                TransactionPhaseOrthogonalNudgingY, numOfSegmentsShifted,
                totalSegmentsToShift);

        // Take a reference segment
        ShiftSegment *currentSegment = m_segment_list.front();
        // Then, find the segments that overlap this one.
        currentRegion.clear();
        currentRegion.push_back(currentSegment);
        m_segment_list.erase(m_segment_list.begin());
        for (ShiftSegmentList::iterator curr = m_segment_list.begin();
                curr != m_segment_list.end(); )
        {
            bool overlaps = false;
            for (ShiftSegmentList::iterator curr2 = currentRegion.begin();
                    curr2 != currentRegion.end(); ++curr2)
            {
                if ((*curr)->overlapsWith(*curr2, dimension))
                {
                    overlaps = true;
                    break;
                }
            }
            if (overlaps)
            {
                currentRegion.push_back(*curr);
                m_segment_list.erase(curr);
                // Consider segments from the beginning, since we may have
                // since passed segments that overlap with the new set.
                curr = m_segment_list.begin();
            }
            else
            {
                ++curr;
            }
        }

        if (! justUnifying)
        {
            CmpLineOrder lineSortComp(m_point_orders, dimension);
            currentRegion = linesort(nudgeFinalSegments, currentRegion,
                    lineSortComp);
        }

        if (currentRegion.size() == 1)
        {
            // Save creating the solver instance if there is just one
            // immovable segment, or if we are in the unifying stage.
            if (currentRegion.front()->immovable() || justUnifying)
            {
                delete currentRegion.front();
                continue;
            }
        }

        // Process these segments.
        std::list<size_t> freeIndexes;
        Variables vs;
        Constraints cs;
        Constraints gapcs;
        ShiftSegmentPtrList prevVars;
        double sepDist = baseSepDist;
#ifdef NUDGE_DEBUG
        fprintf(stderr, "-------------------------------------------------------\n");
        fprintf(stderr, "%s -- size: %d\n", (justUnifying) ? "Unifying" : "Nudging",
                (int) currentRegion.size());
#endif
#ifdef NUDGE_DEBUG_SVG
        printf("\n\n");
#endif
        for (ShiftSegmentList::iterator currSegmentIt = currentRegion.begin();
                currSegmentIt != currentRegion.end(); ++currSegmentIt )
        {
            NudgingShiftSegment *currSegment = static_cast<NudgingShiftSegment *> (*currSegmentIt);

            // Create a solver variable for the position of this segment.
            currSegment->createSolverVariable(justUnifying);

            vs.push_back(currSegment->variable);
            size_t index = vs.size() - 1;
#ifdef NUDGE_DEBUG
            fprintf(stderr,"line(%d)  %.15f  dim: %d pos: %.16f\n"
                   "min: %.16f  max: %.16f\n"
                   "minEndPt: %.16f  maxEndPt: %.16f weight: %g cc: %d\n",
                    currSegment->connRef->id(),
                    currSegment->lowPoint()[dimension], (int) dimension,
                    currSegment->variable->desiredPosition,
                    currSegment->minSpaceLimit, currSegment->maxSpaceLimit,
                    currSegment->lowPoint()[!dimension], currSegment->highPoint()[!dimension],
                    currSegment->variable->weight,
                    (int) currSegment->checkpoints.size());
#endif
#ifdef NUDGE_DEBUG_SVG
            // Debugging info:
            double minP = std::max(currSegment->minSpaceLimit, -5000.0);
            double maxP = std::min(currSegment->maxSpaceLimit, 5000.0);
            fprintf(stdout, "<rect style=\"fill: #f00; opacity: 0.2;\" "
                    "x=\"%g\" y=\"%g\" width=\"%g\" height=\"%g\" />\n",
                    currSegment->lowPoint()[XDIM], minP,
                    currSegment->highPoint()[XDIM] - currSegment->lowPoint()[XDIM],
                    maxP - minP);
            fprintf(stdout, "<line style=\"stroke: #000;\" x1=\"%g\" "
                    "y1=\"%g\" x2=\"%g\" y2=\"%g\" />\n",
                    currSegment->lowPoint()[XDIM], currSegment->lowPoint()[YDIM],
                    currSegment->highPoint()[XDIM], currSegment->highPoint()[YDIM]);
#endif

            if (justUnifying)
            {
                // Just doing centring, not nudging.
                // Record the index of the variable so we can use it as
                // a segment to potentially constrain to other segments.
                if (currSegment->variable->weight == freeWeight)
                {
                    freeIndexes.push_back(index);
                }
                // Thus, we don't need to constrain position against other
                // segments.
                prevVars.push_back(&(*currSegment));
                continue;
            }

            // The constraints generated here must be in order of
            // leftBoundary-segment ... segment-segment ... segment-rightBoundary
            // since this order is leveraged later for rewriting the
            // separations of unsatisfable channel groups.

            // Constrain to channel boundary.
            if (!currSegment->fixed)
            {
                // If this segment sees a channel boundary to its left,
                // then constrain its placement as such.
                if (currSegment->minSpaceLimit > -CHANNEL_MAX)
                {
                    vs.push_back(new Variable(channelLeftID,
                                currSegment->minSpaceLimit, fixedWeight));
                    cs.push_back(new Constraint(vs[vs.size() - 1], vs[index],
                                0.0));
                }
            }

            // Constrain position in relation to previously seen segments,
            // if necessary (i.e. when they could overlap).
            for (ShiftSegmentPtrList::iterator prevVarIt = prevVars.begin();
                    prevVarIt != prevVars.end(); ++prevVarIt)
            {
                NudgingShiftSegment *prevSeg =
                        static_cast<NudgingShiftSegment *> (*prevVarIt);
                Variable *prevVar = prevSeg->variable;

                if (currSegment->overlapsWith(prevSeg, dimension) &&
                        (!(currSegment->fixed) || !(prevSeg->fixed)))
                {
                    // If there is a previous segment to the left that
                    // could overlap this in the shift direction, then
                    // constrain the two segments to be separated.
                    // Though don't add the constraint if both the
                    // segments are fixed in place.
                    double thisSepDist = sepDist;
                    bool equality = false;
                    if (currSegment->shouldAlignWith(prevSeg, dimension))
                    {
                        // Handles the case where the two end segments can
                        // be brought together to make a single segment. This
                        // can help in situations where having the small kink
                        // can restrict other kinds of nudging.
                        thisSepDist = 0;
                        equality = true;
                    }
                    else if (currSegment->canAlignWith(prevSeg, dimension))
                    {
                        // We need to address the problem of two neighbouring
                        // segments of the same connector being kept separated
                        // due only to a kink created in the other dimension.
                        // Here, we let such segments drift back together.
                        thisSepDist = 0;
                    }
                    else if (!nudgeSharedPathsWithCommonEnd &&
                            (m_shared_path_connectors_with_common_endpoints.count(
                                 UnsignedPair(currSegment->connRef->id(), prevSeg->connRef->id())) > 0))
                    {
                        // We don't want to nudge apart these two segments
                        // since they are from a shared path with a common
                        // endpoint.  There might be multiple chains of
                        // segments that don't all have the same endpoints
                        // so we need to make this an equality to prevent
                        // some of them possibly getting nudged apart.
                        thisSepDist = 0;
                        equality = true;
                    }

                    Constraint *constraint = new Constraint(prevVar,
                            vs[index], thisSepDist, equality);
                    cs.push_back(constraint);
                    if (thisSepDist)
                    {
                        // Add to the list of gap constraints so we can
                        // rewrite the separation distance later.
                        gapcs.push_back(constraint);
                    }
                }
            }

            if (!currSegment->fixed)
            {
                // If this segment sees a channel boundary to its right,
                // then constrain its placement as such.
                if (currSegment->maxSpaceLimit < CHANNEL_MAX)
                {
                    vs.push_back(new Variable(channelRightID,
                                currSegment->maxSpaceLimit, fixedWeight));
                    cs.push_back(new Constraint(vs[index], vs[vs.size() - 1],
                                0.0));
                }
            }

            prevVars.push_back(&(*currSegment));
        }

        std::list<PotentialSegmentConstraint> potentialConstraints;
        if (justUnifying)
        {
            for (std::list<size_t>::iterator curr = freeIndexes.begin();
                    curr != freeIndexes.end(); ++curr)
            {
                for (std::list<size_t>::iterator curr2 = curr;
                        curr2 != freeIndexes.end(); ++curr2)
                {
                    if (curr == curr2)
                    {
                        continue;
                    }
                    potentialConstraints.push_back(
                            PotentialSegmentConstraint(*curr, *curr2, vs));
                }
            }
        }
#ifdef NUDGE_DEBUG
        for (unsigned i = 0;i < vs.size(); ++i)
        {
            fprintf(stderr, "-vs[%d]=%f\n", i, vs[i]->desiredPosition);
        }
#endif
        // Repeatedly try solving this.  There are two cases:
        //  -  When Unifying, we greedily place as many free segments as
        //     possible at the same positions, that way they have more
        //     accurate nudging orders determined for them in the Nudging
        //     stage.
        //  -  When Nudging, if we can't fit all the segments with the
        //     default nudging distance we try smaller separation
        //     distances till we find a solution that is satisfied.
        bool justAddedConstraint = false;
        bool satisfied;

        typedef std::pair<size_t, size_t> UnsatisfiedRange;
        std::list<UnsatisfiedRange> unsatisfiedRanges;
        do
        {
            IncSolver f(vs, cs);
            f.solve();

            // Determine if the problem was satisfied.
            satisfied = true;
            for (size_t i = 0; i < vs.size(); ++i)
            {
                // For each variable...
                if (vs[i]->id != freeSegmentID)
                {
                    // If it is a fixed segment (should stay still)...
                    if (fabs(vs[i]->finalPosition -
                            vs[i]->desiredPosition) > 0.0001)
                    {
                        // and it is not at it's desired position, then
                        // we consider the problem to be unsatisfied.
                        satisfied = false;

                        // We record ranges of unsatisfied variables based on
                        // the channel edges.
                        if (vs[i]->id == channelLeftID)
                        {
                            // This is the left-hand-side of a channel.
                            if (unsatisfiedRanges.empty() ||
                                    (unsatisfiedRanges.back().first !=
                                    unsatisfiedRanges.back().second))
                            {
                                // There are no existing unsatisfied ranges,
                                // or there are but they are a valid range
                                // (we've encountered the right-hand channel
                                // edges already).
                                // So, start a new unsatisfied range.
                                unsatisfiedRanges.push_back(
                                        std::make_pair(i, i + 1));
                            }
                        }
                        else if (vs[i]->id == channelRightID)
                        {
                            // This is the right-hand-side of a channel.
                            if (unsatisfiedRanges.empty())
                            {
                                // There are no existing unsatisfied ranges,
                                // so start a new unsatisfied range.
                                // We are looking at a unsatisfied right side
                                // where the left side was satisfied, so the
                                // range begins at the previous variable
                                // which should be a left channel side.
                                COLA_ASSERT(i > 0);
                                COLA_ASSERT(vs[i - 1]->id == channelLeftID);
                                unsatisfiedRanges.push_back(
                                        std::make_pair(i - 1, i));
                            }
                            else
                            {
                                // Expand the existing range to include index.
                                unsatisfiedRanges.back().second = i;
                            }
                        }
                        else if (vs[i]->id == fixedSegmentID)
                        {
                            // Fixed connector segments can also start and
                            // extend unsatisfied variable ranges.
                            if (unsatisfiedRanges.empty())
                            {
                                // There are no existing unsatisfied ranges,
                                // so start a new unsatisfied range.
                                unsatisfiedRanges.push_back(
                                        std::make_pair(i, i));
                            }
                            else
                            {
                                // Expand the existing range to include index.
                                unsatisfiedRanges.back().second = i;
                            }
                        }
                    }
                }
            }

#ifdef NUDGE_DEBUG
            if (!satisfied)
            {
                fprintf(stderr,"unsatisfied\n");
            }
#endif

            if (justUnifying)
            {
                // When we're centring, we'd like to greedily place as many
                // segments as possible at the same positions, that way they
                // have more accurate nudging orders determined for them.
                //
                // We do this by taking pairs of adjoining free segments and
                // attempting to constrain them to have the same position,
                // starting from the closest up to the furthest.

                if (justAddedConstraint)
                {
                    COLA_ASSERT(potentialConstraints.size() > 0);
                    if (!satisfied)
                    {
                        // We couldn't satisfy the problem with the added
                        // potential constraint, so we can't position these
                        // segments together.  Roll back.
                        potentialConstraints.pop_front();
                        delete cs.back();
                        cs.pop_back();
                    }
                    else
                    {
                        // We could position these two segments together.
                        PotentialSegmentConstraint& pc =
                                potentialConstraints.front();

                        // Rewrite the indexes of these two variables to
                        // one, so we need not worry about redundant
                        // equality constraints.
                        for (std::list<PotentialSegmentConstraint>::iterator
                                it = potentialConstraints.begin();
                                it != potentialConstraints.end(); ++it)
                        {
                            it->rewriteIndex(pc.index1, pc.index2);
                        }
                        potentialConstraints.pop_front();
                    }
                }
                potentialConstraints.sort();
                justAddedConstraint = false;

                // Remove now invalid potential segment constraints.
                // This could have been caused by the variable rewriting.
                while (!potentialConstraints.empty() &&
                       !potentialConstraints.front().stillValid())
                {
                    potentialConstraints.pop_front();
                }

                if (!potentialConstraints.empty())
                {
                    // We still have more possibilities to consider.
                    // Create a constraint for this, add it, and mark as
                    // unsatisfied, so the problem gets re-solved.
                    PotentialSegmentConstraint& pc =
                            potentialConstraints.front();
                    COLA_ASSERT(pc.index1 != pc.index2);
                    cs.push_back(new Constraint(vs[pc.index1], vs[pc.index2],
                            0, true));
                    satisfied = false;
                    justAddedConstraint = true;
                }
            }
            else
            {
                if (!satisfied)
                {
                    COLA_ASSERT(unsatisfiedRanges.size() > 0);
                    // Reduce the separation distance.
                    sepDist -= (baseSepDist / reductionSteps);
#ifndef NDEBUG
                    for (std::list<UnsatisfiedRange>::iterator it =
                            unsatisfiedRanges.begin();
                            it != unsatisfiedRanges.end(); ++it)
                    {
                        COLA_ASSERT(vs[it->first]->id != freeSegmentID);
                        COLA_ASSERT(vs[it->second]->id != freeSegmentID);
                    }
#endif
#ifdef NUDGE_DEBUG
                    for (std::list<UnsatisfiedRange>::iterator it =
                            unsatisfiedRanges.begin();
                            it != unsatisfiedRanges.end(); ++it)
                    {
                        fprintf(stderr, "unsatisfiedVarRange(%ld, %ld)\n",
                                it->first, it->second);
                    }
                    fprintf(stderr, "unsatisfied, trying %g\n", sepDist);
#endif
                    // And rewrite all the gap constraints to have the new
                    // reduced separation distance.
                    bool withinUnsatisfiedGroup = false;
                    for (Constraints::iterator cIt = cs.begin();
                            cIt != cs.end(); ++cIt)
                    {
                        UnsatisfiedRange& range = unsatisfiedRanges.front();
                        Constraint *constraint = *cIt;

                        if (constraint->left == vs[range.first])
                        {
                            // Entered an unsatisfied range of variables.
                            withinUnsatisfiedGroup = true;
                        }

                        if (withinUnsatisfiedGroup && (constraint->gap > 0))
                        {
                            // Rewrite constraints in unsatisfied ranges
                            // that have a non-zero gap.
                            constraint->gap = sepDist;
                        }

                        if (constraint->right == vs[range.second])
                        {
                            // Left an unsatisfied range of variables.
                            withinUnsatisfiedGroup = false;
                            unsatisfiedRanges.pop_front();
                            if (unsatisfiedRanges.empty())
                            {
                                // And there are no more unsatisfied variables.
                                break;
                            }
                        }
                    }
                }
            }
        }
        while (!satisfied && (sepDist > 0.0001));

        if (satisfied)
        {
#ifdef NUDGE_DEBUG
            fprintf(stderr,"satisfied at nudgeDist = %g\n", sepDist);
#endif
            for (ShiftSegmentList::iterator currSegment = currentRegion.begin();
                    currSegment != currentRegion.end(); ++currSegment)
            {
                NudgingShiftSegment *segment =
                        static_cast<NudgingShiftSegment *> (*currSegment);

                segment->updatePositionsFromSolver(justUnifying);
            }
        }
#ifdef NUDGE_DEBUG
        for(unsigned i=0;i<vs.size();i++) {
            fprintf(stderr, "+vs[%d]=%f\n",i,vs[i]->finalPosition);
        }
#endif
#ifdef NUDGE_DEBUG_SVG
        for (ShiftSegmentList::iterator currSegment = currentRegion.begin();
                currSegment != currentRegion.end(); ++currSegment)
        {
            NudgingShiftSegment *segment =
                    static_cast<NudgingShiftSegment *> (*currSegment);

            fprintf(stdout, "<line style=\"stroke: #00F;\" x1=\"%g\" "
                    "y1=\"%g\" x2=\"%g\" y2=\"%g\" />\n",
                    segment->lowPoint()[XDIM], segment->variable->finalPosition,
                    segment->highPoint()[XDIM], segment->variable->finalPosition);
        }
#endif
        for_each(currentRegion.begin(), currentRegion.end(), delete_object());
        for_each(vs.begin(), vs.end(), delete_object());
        for_each(cs.begin(), cs.end(), delete_object());
    }
}


void ImproveOrthogonalRoutes::simplifyOrthogonalRoutes(void)
{
    // Simplify routes.
    for (ConnRefList::const_iterator curr = m_router->connRefs.begin();
            curr != m_router->connRefs.end(); ++curr)
    {
        if ((*curr)->routingType() != ConnType_Orthogonal)
        {
            continue;
        }
        (*curr)->set_route((*curr)->displayRoute().simplify());
    }
}


// Populates m_point_orders and m_shared_path_connectors_with_common_endpoints.
void ImproveOrthogonalRoutes::buildOrthogonalNudgingOrderInfo(void)
{
    // Simplify routes.
    simplifyOrthogonalRoutes();

    int crossingsN = 0;

    bool buildSharedPathInfo = false;
    if (!m_router->routingOption(Avoid::nudgeSharedPathsWithCommonEndPoint) &&
            m_shared_path_connectors_with_common_endpoints.empty())
    {
        // We're not going to nudge apart shared paths with common ends so we
        // will need to store information about this during the crossing
        // detection.
        buildSharedPathInfo = true;
    }


    // Make a vector of the ConnRefList, for convenience.
    ConnRefVector connRefs(m_router->connRefs.begin(), m_router->connRefs.end());

    // Make a temporary copy of all the connector displayRoutes.
    RouteVector connRoutes(connRefs.size());
    for (size_t ind = 0; ind < connRefs.size(); ++ind)
    {
        connRoutes[ind] = connRefs[ind]->displayRoute();
    }

    // Do segment splitting.
    for (size_t ind1 = 0; ind1 < connRefs.size(); ++ind1)
    {
        ConnRef *conn = connRefs[ind1];
        if (conn->routingType() != ConnType_Orthogonal)
        {
            continue;
        }

        for (size_t ind2 = 0; ind2 < connRefs.size(); ++ind2)
        {
            if (ind1 == ind2)
            {
                continue;
            }

            ConnRef *conn2 = connRefs[ind2];
            if (conn2->routingType() != ConnType_Orthogonal)
            {
                continue;
            }

            Avoid::Polygon& route = connRoutes[ind1];
            Avoid::Polygon& route2 = connRoutes[ind2];
            splitBranchingSegments(route2, true, route);
        }
    }

    for (size_t ind1 = 0; ind1 < connRefs.size(); ++ind1)
    {
        ConnRef *conn = connRefs[ind1];
        if (conn->routingType() != ConnType_Orthogonal)
        {
            continue;
        }

        for (size_t ind2 = ind1 + 1; ind2 < connRefs.size(); ++ind2)
        {
            ConnRef *conn2 = connRefs[ind2];
            if (conn2->routingType() != ConnType_Orthogonal)
            {
                continue;
            }

            Avoid::Polygon& route = connRoutes[ind1];
            Avoid::Polygon& route2 = connRoutes[ind2];
            int crossings = 0;
            unsigned int crossingFlags = 0;
            ConnectorCrossings cross(route2, true, route, conn2, conn);
            cross.pointOrders = &m_point_orders;
            for (size_t i = 1; i < route.size(); ++i)
            {
                const bool finalSegment = ((i + 1) == route.size());
                cross.countForSegment(i, finalSegment);

                crossings += cross.crossingCount;
                crossingFlags |= cross.crossingFlags;
            }
            if (crossings > 0)
            {
                crossingsN += crossings;
            }

            if (buildSharedPathInfo &&
                    (crossingFlags & CROSSING_SHARES_PATH_AT_END))
            {
                // Record if these two connectors have a shared path with a
                // common end point.
                m_shared_path_connectors_with_common_endpoints.insert(
                        UnsignedPair(conn->id(), conn2->id()));
            }
        }
    }
}


extern void improveOrthogonalRoutes(Router *router)
{
    ImproveOrthogonalRoutes improver(router);
    improver.execute();
}


}

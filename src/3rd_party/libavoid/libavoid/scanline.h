/*
 * vim: ts=4 sw=4 et tw=0 wm=0
 *
 * libavoid - Fast, Incremental, Object-avoiding Line Router
 *
 * Copyright (C) 2009-2013  Monash University
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
 * Author(s):   Michael Wybrow
*/


#ifndef AVOID_SCANLINE_H
#define AVOID_SCANLINE_H

#include <set>
#include <list>

#include "libavoid/geomtypes.h"


namespace Avoid {

static const double CHANNEL_MAX = 100000000;


class Obstacle;
class VertInf;

// ShiftSegment interface.
class ShiftSegment
{ 
    public:
        ShiftSegment(const size_t dim)
            : dimension(dim)
        {
        }
        virtual ~ShiftSegment()
        {
        }
        virtual Point& lowPoint(void) = 0;
        virtual Point& highPoint(void) = 0;
        virtual const Point& lowPoint(void) const = 0;
        virtual const Point& highPoint(void) const = 0;
        virtual bool overlapsWith(const ShiftSegment *rhs,
                const size_t dim) const = 0;
        virtual bool immovable(void) const = 0;
        
        size_t dimension;
        double minSpaceLimit;
        double maxSpaceLimit;
};
typedef std::list<ShiftSegment *> ShiftSegmentList;


class Node;
struct CmpNodePos
{
    bool operator()(const Node* u, const Node* v) const; 
};


typedef std::set<Node*,CmpNodePos> NodeSet;

class Node 
{
    public:

    Obstacle *v;
    VertInf *c;
    ShiftSegment *ss;
    double pos;
    double min[2], max[2];
    Node *firstAbove, *firstBelow;
    NodeSet::iterator iter;

    Node(Obstacle *v, const double p);
    Node(VertInf *c, const double p);
    Node(ShiftSegment *ss, const double p);
    virtual ~Node();
    double firstObstacleAbove(size_t dim);
    double firstObstacleBelow(size_t dim);
    void markShiftSegmentsAbove(size_t dim);
    void markShiftSegmentsBelow(size_t dim);
    void findFirstPointAboveAndBelow(const size_t dim, const double linePos,
            double& firstAbovePos, double& firstBelowPos, 
            double& lastAbovePos, double& lastBelowPos);
    double firstPointAbove(size_t dim);
    double firstPointBelow(size_t dim);
    bool isInsideShape(size_t dimension);
};


// Note: Open must come first.
typedef enum {
    Open = 1,
    SegOpen = 2,
    ConnPoint = 3, 
    SegClose = 4,
    Close = 5
} EventType;


struct Event
{
    Event(EventType t, Node *v, double p);
    
    EventType type;
    Node *v;
    double pos;
};


extern int compare_events(const void *a, const void *b);
extern void buildConnectorRouteCheckpointCache(Router *router);
extern void clearConnectorRouteCheckpointCache(Router *router);
extern void buildOrthogonalChannelInfo(Router *router, 
        const size_t dim, ShiftSegmentList& segmentList);


}

#endif

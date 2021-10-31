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

#include <cfloat>
#include <algorithm>

#include "libavoid/scanline.h"
#include "libavoid/obstacle.h"
#include "libavoid/vertices.h"
#include "libavoid/connector.h"
#include "libavoid/junction.h"
#include "libavoid/router.h"

namespace Avoid {

bool CmpNodePos::operator() (const Node* u, const Node* v) const 
{
    if (u->pos != v->pos) 
    {
        return u->pos < v->pos;
    }
    
    // Use the pointers to the base objects to differentiate them.
    void *up = (u->v) ? (void *) u->v : 
            ((u->c) ? (void *) u->c : (void *) u->ss);
    void *vp = (v->v) ? (void *) v->v : 
            ((v->c) ? (void *) v->c : (void *) v->ss);
    return up < vp;
}


Node::Node(Obstacle *v, const double p)
    : v(v),
      c(nullptr),
      ss(nullptr),
      pos(p),
      firstAbove(nullptr),
      firstBelow(nullptr)
{
    Box bBox = v->routingBox();
    min[XDIM] = bBox.min.x;
    min[YDIM] = bBox.min.y;
    max[XDIM] = bBox.max.x;
    max[YDIM] = bBox.max.y;
    //COLA_ASSERT(r->width()<1e40);
}

Node::Node(VertInf *c, const double p)
    : v(nullptr),
      c(c),
      ss(nullptr),
      pos(p),
      firstAbove(nullptr),
      firstBelow(nullptr)
{
    min[XDIM] = max[XDIM] = c->point.x;
    min[YDIM] = max[YDIM] = c->point.y;
}

Node::Node(ShiftSegment *ss, const double p)
    : v(nullptr),
      c(nullptr),
      ss(ss),
      pos(p),
      firstAbove(nullptr),
      firstBelow(nullptr)
{
    // These values shouldn't ever be used, so they don't matter.
    min[XDIM] = max[XDIM] = min[YDIM] = max[YDIM] = 0;
}

Node::~Node() 
{
}

// Find the first Node above in the scanline that is a shape edge,
// and does not have an open or close event at this position (meaning
// it is just about to be removed).
double Node::firstObstacleAbove(size_t dim)
{
    Node *curr = firstAbove;
    while (curr && (curr->ss || (curr->max[dim] > pos)))
    {
        curr = curr->firstAbove;
    }
   
    if (curr)
    {
        return curr->max[dim];
    }
    return -DBL_MAX;
}

// Find the first Node below in the scanline that is a shape edge,
// and does not have an open or close event at this position (meaning
// it is just about to be removed).
double Node::firstObstacleBelow(size_t dim)
{
    Node *curr = firstBelow;
    while (curr && (curr->ss || (curr->min[dim] < pos)))
    {
        curr = curr->firstBelow;
    }
    
    if (curr)
    {
        return curr->min[dim];
    }
    return DBL_MAX;
}

// Mark all connector segments above in the scanline as being able 
// to see to this shape edge.
void Node::markShiftSegmentsAbove(size_t dim)
{
    Node *curr = firstAbove;
    while (curr && (curr->ss || (curr->pos > min[dim])))
    {
        if (curr->ss && (curr->pos <= min[dim]))
        {
            curr->ss->maxSpaceLimit = 
                    std::min(min[dim], curr->ss->maxSpaceLimit);
        }
        curr = curr->firstAbove;
    }
}

// Mark all connector segments below in the scanline as being able 
// to see to this shape edge.
void Node::markShiftSegmentsBelow(size_t dim)
{
    Node *curr = firstBelow;
    while (curr && (curr->ss || (curr->pos < max[dim])))
    {
        if (curr->ss && (curr->pos >= max[dim]))
        {
            curr->ss->minSpaceLimit = 
                    std::max(max[dim], curr->ss->minSpaceLimit);
        }
        curr = curr->firstBelow;
    }
}

void Node::findFirstPointAboveAndBelow(const size_t dim, const double linePos,
        double& firstAbovePos, double& firstBelowPos, 
        double& lastAbovePos, double& lastBelowPos)
{
    firstAbovePos = -DBL_MAX;
    firstBelowPos = DBL_MAX;
    // We start looking left from the right side of the shape, 
    // and vice versa. 
    lastAbovePos = max[dim];
    lastBelowPos = min[dim];

    Node *curr = nullptr;
    bool eventsAtSamePos = false;
    for (int direction = 0; direction < 2; ++direction)
    {
        // Look for obstacles in one direction, then the other.
        curr = (direction == 0) ? firstAbove: firstBelow;

        while (curr)
        {
            // The events are at a shared beginning or end of a shape or 
            // connection point.  Note, connection points have the same 
            // min and max value in the !dim dimension.
            eventsAtSamePos = 
                    (((linePos == max[!dim]) && 
                      (linePos == curr->max[!dim])) || 
                     ((linePos == min[!dim]) && 
                      (linePos == curr->min[!dim])));
            
            if (curr->max[dim] <= min[dim])
            {
                // Curr shape is completely to the left, 
                // so add it's right side as a limit
                firstAbovePos = std::max(curr->max[dim], firstAbovePos);
            }
            else if (curr->min[dim] >= max[dim])
            {
                // Curr shape is completely to the right, 
                // so add it's left side as a limit
                firstBelowPos = std::min(curr->min[dim], firstBelowPos);
            }
            else if (!eventsAtSamePos)
            {
                // Shapes that open or close at the same position do not
                // block visibility, so if they are not at same position
                // determine where they overlap.
                lastAbovePos = std::min(curr->min[dim], lastAbovePos);
                lastBelowPos = std::max(curr->max[dim], lastBelowPos);
            }
            curr = (direction == 0) ? curr->firstAbove : curr->firstBelow;
        }
    }    
}

double Node::firstPointAbove(size_t dim) 
{
    // We are looking for the first obstacle above this position,
    // though we ignore shape edges if this point is inline with
    // the edge of the obstacle.  That is, points have visibility
    // along the edge of shapes.
    size_t altDim = (dim + 1) % 2;
    double result = -DBL_MAX;
    Node *curr = firstAbove; 
    while (curr) 
    {
        bool inLineWithEdge = (min[altDim] == curr->min[altDim]) ||
                (min[altDim] == curr->max[altDim]);
        if ( ! inLineWithEdge && (curr->max[dim] <= pos) )
        {
            result = std::max(curr->max[dim], result);
        }
        curr = curr->firstAbove; 
    } 
    return result; 
}

double Node::firstPointBelow(size_t dim) 
{ 
    // We are looking for the first obstacle below this position,
    // though we ignore shape edges if this point is inline with
    // the edge of the obstacle.  That is, points have visibility
    // along the edge of shapes.
    size_t altDim = (dim + 1) % 2;
    double result = DBL_MAX;
    Node *curr = firstBelow; 
    while (curr) 
    { 
        bool inLineWithEdge = (min[altDim] == curr->min[altDim]) ||
                (min[altDim] == curr->max[altDim]);
        if ( ! inLineWithEdge && (curr->min[dim] >= pos) )
        {
            result = std::min(curr->min[dim], result);
        }
        curr = curr->firstBelow; 
    }
    return result;
}

// This is a bit inefficient, but we won't need to do it once we have 
// connection points.
bool Node::isInsideShape(size_t dimension)
{
    for (Node *curr = firstBelow; curr; curr = curr->firstBelow)
    {
        if ((curr->min[dimension] < pos) && (pos < curr->max[dimension]))
        {
            return true;
        }
    }
    for (Node *curr = firstAbove; curr; curr = curr->firstAbove)
    {
        if ((curr->min[dimension] < pos) && (pos < curr->max[dimension]))
        {
            return true;
        }
    }
    return false;
}


Event::Event(EventType t, Node *v, double p) 
    : type(t),
      v(v),
      pos(p)
{
}


// Used for quicksort.  Must return <0, 0, or >0.
int compare_events(const void *a, const void *b)
{
	Event *ea = *(Event**) a;
	Event *eb = *(Event**) b;
    if (ea->pos != eb->pos)
    {
        return (ea->pos < eb->pos) ? -1 : 1;
    }
    if (ea->type != eb->type)
    {
        return ea->type - eb->type;
    }
    COLA_ASSERT(ea->v != eb->v);
    return (int)(ea->v - eb->v);
}


void buildConnectorRouteCheckpointCache(Router *router)
{
    for (ConnRefList::const_iterator curr = router->connRefs.begin(); 
            curr != router->connRefs.end(); ++curr) 
    {
        ConnRef *conn = *curr;
        if (conn->routingType() != ConnType_Orthogonal)
        {
            continue;
        }

        PolyLine& displayRoute = conn->displayRoute();
        std::vector<Checkpoint> checkpoints = conn->routingCheckpoints();
       
        // Initialise checkpoint vector and set to false.  There will be
        // one entry for each *segment* in the path, and the value indicates
        // whether the segment is affected by a checkpoint.
        displayRoute.checkpointsOnRoute = 
                std::vector<std::pair<size_t, Point> >();

        for (size_t ind = 0; ind < displayRoute.size(); ++ind)
        {
            if (ind > 0)
            {
                for (size_t cpi = 0; cpi < checkpoints.size(); ++cpi)
                {
                    if (pointOnLine(displayRoute.ps[ind - 1], 
                             displayRoute.ps[ind], checkpoints[cpi].point) )
                    {
                        // The checkpoint is on a segment.
                        displayRoute.checkpointsOnRoute.push_back(
                                std::make_pair((ind * 2) - 1, 
                                    checkpoints[cpi].point));
                    }
                }
            }

            for (size_t cpi = 0; cpi < checkpoints.size(); ++cpi)
            {
                if (displayRoute.ps[ind].equals(checkpoints[cpi].point))
                {
                    // The checkpoint is at a bendpoint.
                    displayRoute.checkpointsOnRoute.push_back(
                            std::make_pair(ind * 2, checkpoints[cpi].point));
                }
            }
        }
    }
}


void clearConnectorRouteCheckpointCache(Router *router)
{
    for (ConnRefList::const_iterator curr = router->connRefs.begin(); 
            curr != router->connRefs.end(); ++curr) 
    {
        ConnRef *conn = *curr;
        if (conn->routingType() != ConnType_Orthogonal)
        {
            continue;
        }

        // Clear the cache.
        PolyLine& displayRoute = conn->displayRoute();
        displayRoute.checkpointsOnRoute.clear();
    }
}


// Processes sweep events used to determine each horizontal and vertical 
// line segment in a connector's channel of visibility.  
// Four calls to this function are made at each position by the scanline:
//   1) Handle all Close event processing.
//   2) Remove Close event objects from the scanline.
//   3) Add Open event objects to the scanline.
//   4) Handle all Open event processing.
//
static void processShiftEvent(NodeSet& scanline, Event *e, size_t dim,
        unsigned int pass)
{
    Node *v = e->v;
    
    if ( ((pass == 3) && (e->type == Open)) ||
         ((pass == 3) && (e->type == SegOpen)) )
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
    
    if ( ((pass == 4) && (e->type == Open)) ||
         ((pass == 4) && (e->type == SegOpen)) ||
         ((pass == 1) && (e->type == SegClose)) ||
         ((pass == 1) && (e->type == Close)) )
    {
        if (v->ss)
        {
            // As far as we can see.
            double minLimit = v->firstObstacleAbove(dim);
            double maxLimit = v->firstObstacleBelow(dim);

            v->ss->minSpaceLimit = 
                    std::max(minLimit, v->ss->minSpaceLimit);
            v->ss->maxSpaceLimit = 
                    std::min(maxLimit, v->ss->maxSpaceLimit);
        }
        else
        {
            v->markShiftSegmentsAbove(dim);
            v->markShiftSegmentsBelow(dim);
        }
    }
    
    if ( ((pass == 2) && (e->type == SegClose)) ||
         ((pass == 2) && (e->type == Close)) )
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

        size_t result;
        result = scanline.erase(v);
        COLA_ASSERT(result == 1);
        COLA_UNUSED(result);  // Avoid warning.
        delete v;
    }
}

void buildOrthogonalChannelInfo(Router *router, 
        const size_t dim, ShiftSegmentList& segmentList)
{
    if (segmentList.empty())
    {
        // There are no segments, so we can just return now.
        return;
    }
    
    // Do a sweep to determine space for shifting segments.
    size_t altDim = (dim + 1) % 2;
    const size_t n = router->m_obstacles.size();
    const size_t cpn = segmentList.size();
    // Set up the events for the sweep.
    size_t totalEvents = 2 * (n + cpn);
    Event **events = new Event*[totalEvents];
    unsigned ctr = 0;
    ObstacleList::iterator obstacleIt = router->m_obstacles.begin();
    for (unsigned i = 0; i < n; i++)
    {
        Obstacle *obstacle = *obstacleIt;
        JunctionRef *junction = dynamic_cast<JunctionRef *> (obstacle);
        if (junction && ! junction->positionFixed())
        {
            // Junctions that are free to move are not treated as obstacles.
            ++obstacleIt;
            totalEvents -= 2;
            continue;
        }
        Box bBox = obstacle->routingBox();
        Point min = bBox.min;
        Point max = bBox.max;
        double mid = min[dim] + ((max[dim] - min[dim]) / 2);
        Node *v = new Node(obstacle, mid);
        events[ctr++] = new Event(Open, v, min[altDim]);
        events[ctr++] = new Event(Close, v, max[altDim]);

        ++obstacleIt;
    }
    for (ShiftSegmentList::iterator curr = segmentList.begin(); 
            curr != segmentList.end(); ++curr)
    {
        const Point& lowPt = (*curr)->lowPoint();
        const Point& highPt = (*curr)->highPoint();

        COLA_ASSERT(lowPt[dim] == highPt[dim]);
        COLA_ASSERT(lowPt[altDim] < highPt[altDim]);
        Node *v = new Node(*curr, lowPt[dim]);
        events[ctr++] = new Event(SegOpen, v, lowPt[altDim]);
        events[ctr++] = new Event(SegClose, v, highPt[altDim]);
    }
    qsort((Event*)events, (size_t) totalEvents, sizeof(Event*), compare_events);

    // Process the sweep.
    // We do multiple passes over sections of the list so we can add relevant
    // entries to the scanline that might follow, before process them.
    NodeSet scanline;
    double thisPos = (totalEvents > 0) ? events[0]->pos : 0;
    unsigned int posStartIndex = 0;
    unsigned int posFinishIndex = 0;
    for (unsigned i = 0; i <= totalEvents; ++i)
    {
        // If we have finished the current scanline or all events, then we
        // process the events on the current scanline in a couple of passes.
        if ((i == totalEvents) || (events[i]->pos != thisPos))
        {
            posFinishIndex = i;
            for (int pass = 2; pass <= 4; ++pass)
            {
                for (unsigned j = posStartIndex; j < posFinishIndex; ++j)
                {
                    processShiftEvent(scanline, events[j], dim, pass);
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
        processShiftEvent(scanline, events[i], dim, pass);
    }
    COLA_ASSERT(scanline.size() == 0);
    for (unsigned i = 0; i < totalEvents; ++i)
    {
        delete events[i];
    }
    delete [] events;
}


}


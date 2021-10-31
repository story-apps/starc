/*
 * vim: ts=4 sw=4 et tw=0 wm=0
 *
 * libavoid - Fast, Incremental, Object-avoiding Line Router
 *
 * Copyright (C) 2004-2014  Monash University
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

// For M_PI.
// This should be first include for MSVC.
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <cmath>

#include <algorithm>
#include <vector>
#include <climits>
#include <cfloat>

#include "libavoid/makepath.h"
#include "libavoid/vertices.h"
#include "libavoid/geometry.h"
#include "libavoid/connector.h"
#include "libavoid/viscluster.h"
#include "libavoid/graph.h"
#include "libavoid/router.h"
#include "libavoid/debug.h"
#include "libavoid/assertions.h"
#include "libavoid/debughandler.h"

//#define ESTIMATED_COST_DEBUG

namespace Avoid {

class ANode
{
    public:
        VertInf* inf;
        double g;        // Gone
        double h;        // Heuristic
        double f;        // Formula f = g + h
        
        ANode *prevNode; // VertInf for the previous ANode.
        int timeStamp;   // Time-stamp used to determine exploration order of
                         // seemingly equal paths during orthogonal routing.

        ANode(VertInf *vinf, int time)
            : inf(vinf),
              g(0),
              h(0),
              f(0),
              prevNode(nullptr),
              timeStamp(time)
        {
        }
        ANode()
            : inf(nullptr),
              g(0),
              h(0),
              f(0),
              prevNode(nullptr),
              timeStamp(-1)
        {
        }
};

class AStarPathPrivate
{
    public:
        AStarPathPrivate()
            : m_available_nodes(),
              m_available_array_size(0),
              m_available_array_index(0),
              m_available_node_index(0)
        {
        }
        ~AStarPathPrivate()
        {
            // Free memory
            for (size_t i = 0; i < m_available_nodes.size(); ++i)
            {
                delete[] m_available_nodes[i];
            }
        }
        // Returns a pointer to an ANode for aStar search, but allocates
        // these in blocks 
        ANode *newANode(const ANode& node, const bool addToPending = true)
        {
            const size_t blockSize = 5000;
            if ((m_available_array_index + 1 > m_available_array_size) ||
                    (m_available_node_index >= blockSize))
            {
                m_available_nodes.push_back(new ANode[blockSize]);
                ++m_available_array_size;
                m_available_node_index = 0;
                m_available_array_index = m_available_array_size - 1;
            }
            
            ANode *nodes = m_available_nodes[m_available_array_index];
            ANode *newNode = &(nodes[m_available_node_index++]);
            *newNode = node;
            if (addToPending)
            {
                node.inf->aStarPendingNodes.push_back(newNode);
            }
            return newNode;
        }
        void search(ConnRef *lineRef, VertInf *src, VertInf *tar, 
                VertInf *start);

    private:
        void determineEndPointLocation(double dist, VertInf *start,
                VertInf *target, VertInf *other, int level);
        double estimatedCost(ConnRef *lineRef, const Point *last,
                const Point& curr) const;

        std::vector<ANode *> m_available_nodes;
        size_t m_available_array_size;
        size_t m_available_array_index;
        size_t m_available_node_index;
 
        // For determining estimated cost target.
        std::vector<VertInf *> m_cost_targets;
        std::vector<unsigned int> m_cost_targets_directions;
        std::vector<double> m_cost_targets_displacements;
};



// This returns the opposite result (>) so that when used with stl::make_heap, 
// the head node of the heap will be the smallest value, rather than the 
// largest.  This saves us from having to sort the heap (and then reorder
// it back into a heap) when getting the next node to examine.  This way we
// get better complexity -- logarithmic pushes and pops to the heap.
//
class ANodeCmp
{
    public:
    ANodeCmp()
    {
    }
bool operator()(const ANode *a, const ANode *b)
{
    // We need to use an epsilon here since otherwise the multiple addition
    // of floating point numbers that makes up the 'f' values cause a problem
    // with routings occasionally being non-deterministic.
    if (fabs(a->f - b->f) > 0.0000001)
    {
        return a->f > b->f;
    }
    if (a->timeStamp != b->timeStamp)
    {
        // Tiebreaker, if two paths have equal cost, then choose the one with
        // the highest timeStamp.  This corresponds to the furthest point
        // explored along the straight-line path.  When exploring we give the
        // directions the following timeStamps; left:1, right:2 and forward:3,
        // then we always try to explore forward first.
        return a->timeStamp < b->timeStamp;
    }
    return false;
}
};


static double Dot(const Point& l, const Point& r)
{
    return (l.x * r.x) + (l.y * r.y);
}

static double CrossLength(const Point& l, const Point& r)
{
    return (l.x * r.y) - (l.y * r.x);
}


// Return the angle between the two line segments made by the
// points p1--p2 and p2--p3.  Return value is in radians.
//
static double angleBetween(const Point& p1, const Point& p2, const Point& p3)
{
    if ((p1.x == p2.x && p1.y == p2.y) || (p2.x == p3.x && p2.y == p3.y))
    {
        // If two of the points are the same, then we can't say anything
        // about the angle between.  Treat them as being collinear.
        return M_PI;
    }

    Point v1(p1.x - p2.x, p1.y - p2.y);
    Point v2(p3.x - p2.x, p3.y - p2.y);

    return fabs(atan2(CrossLength(v1, v2), Dot(v1, v2)));
}


// Construct a temporary Polygon path given several VertInf's for a connector.
//
static void constructPolygonPath(Polygon& connRoute, VertInf *inf2, 
        VertInf *inf3, ANode *inf1Node)
{
    // Don't include colinear points.
    bool simplified = true;

    int routeSize = 2;
    for (ANode *curr = inf1Node; curr != nullptr; curr = curr->prevNode)
    {
        routeSize += 1;
    }
    connRoute.ps.resize(routeSize);
    int arraySize = routeSize;
    connRoute.ps[routeSize - 1] = inf3->point;
    connRoute.ps[routeSize - 2] = inf2->point;
    routeSize -= 3;
    for (ANode *curr = inf1Node; curr != nullptr; curr = curr->prevNode)
    {
        // For connection pins, we stop and don't include the fake shape 
        // center as part of this path.
        bool isConnectionPin = curr->inf->id.isConnectionPin();

        if (!simplified)
        {
            // If this is non-simplified, we don't need to do anything 
            // clever and can simply add the new point.
            connRoute.ps[routeSize] = curr->inf->point;
            routeSize -= 1;
            
            if (isConnectionPin)
            {
                // Stop at the connection pin.
                break;
            }
            continue;
        }
            
        if ((curr == inf1Node) || 
                vecDir(curr->inf->point, connRoute.ps[routeSize + 1], 
                    connRoute.ps[routeSize + 2]) != 0)
        {
            // Add new point if this is the earlier than the last segment
            // and it is not colinear with the other points.  
            // Note, you can't collapse the 'last' segment with previous 
            // segments, or if this just intersects another line you risk 
            // penalising it once for each collapsed line segment.
            connRoute.ps[routeSize] = curr->inf->point;
            routeSize -= 1;
        }
        else
        {
            // The last point is inline with this one, so update it.
            connRoute.ps[routeSize + 1] = curr->inf->point;
        }
            
        if (isConnectionPin)
        {
            // Stop at the connection pin.
            break;
        }
    }

    // If the vector is not filled, move entries to the beginning and 
    // remove the unused end of the vector.
    int diff = routeSize + 1;
    COLA_ASSERT(simplified || (diff == 0));
    if (diff > 0)
    {
        for (int i = diff; i < arraySize; ++i)
        {
            connRoute.ps[i - diff] = connRoute.ps[i];
        }
        connRoute.ps.resize(connRoute.size() - diff);
    }
}

// Used to get an indication of if a diffence is positive (1),
// negative (-1) or no different (0).
static inline int dimDirection(double difference)
{
    if (difference > 0)
    {
        return 1;
    }
    else if (difference < 0)
    {
        return -1;
    }
    return 0;
}

// Given the two points for a new segment of a path (inf2 & inf3)
// as well as the distance between these points (dist), as well as
// possibly the previous point (inf1) [from inf1--inf2], return a
// cost associated with this route.
//
static double cost(ConnRef *lineRef, const double dist, VertInf *inf2, 
        VertInf *inf3, ANode *inf1Node)
{
    bool isOrthogonal = (lineRef->routingType() == ConnType_Orthogonal);
    VertInf *inf1 = (inf1Node) ? inf1Node->inf : nullptr;
    double result = dist;
    Polygon connRoute;

    Router *router = inf2->_router;
    if (inf1 != nullptr)
    {
        const double angle_penalty = router->routingParameter(anglePenalty);
        const double segmt_penalty = router->routingParameter(segmentPenalty);

        // This is not the first segment, so there is a bend
        // between it and the last one in the existing path.
        if ((angle_penalty > 0) || (segmt_penalty > 0))
        {
            Point p1 = inf1->point;
            Point p2 = inf2->point;
            Point p3 = inf3->point;

            double rad = M_PI - angleBetween(p1, p2, p3);

            if ((rad > 0) && !isOrthogonal)
            {
                // Make `xval' between 0--10 then take its log so small
                // angles are not penalised as much as large ones.
                //
                double xval = rad * 10 / M_PI;
                double yval = xval * log10(xval + 1) / 10.5;
                result += (angle_penalty * yval);
                //db_printf("deg from straight: %g\tpenalty: %g\n",
                //        rad * 180 / M_PI, (angle_penalty * yval));
            }

            if (rad == M_PI)
            {
                // Needs to double back
                result += (2 * segmt_penalty);
            }
            else if (rad > 0)
            {
                // Only penalise as an extra segment if the two 
                // segments are not collinear.
                result += segmt_penalty;
            }
        }
    }

    const double cluster_crossing_penalty = 
            router->routingParameter(clusterCrossingPenalty);
    // XXX: Clustered routing doesn't yet work with orthogonal connectors.
    if (router->ClusteredRouting && !router->clusterRefs.empty() &&
            (cluster_crossing_penalty > 0))
    {
        if (connRoute.empty())
        {
            constructPolygonPath(connRoute, inf2, inf3, inf1Node);
        }
        // There are clusters so do cluster routing.
        for (ClusterRefList::const_iterator cl = router->clusterRefs.begin(); 
                cl != router->clusterRefs.end(); ++cl)
        {
            Polygon cBoundary = (isOrthogonal) ?
                    (*cl)->rectangularPolygon() : (*cl)->polygon();
            if (cBoundary.size() <= 2)
            {
                continue;
            }
            COLA_ASSERT(cBoundary.ps[0] != cBoundary.ps[cBoundary.size() - 1]);
            for (size_t j = 0; j < cBoundary.size(); ++j)
            {
                // Non-orthogonal cluster boundary points should correspond to 
                // shape vertices and hence already be in the list of vertices.
                COLA_ASSERT(isOrthogonal || 
                        router->vertices.getVertexByPos(cBoundary.at(j)));
            }
            
            bool isConn = false;
            Polygon dynamic_conn_route(connRoute);
            const bool finalSegment = (inf3 == lineRef->dst());
            ConnectorCrossings cross(cBoundary, isConn, dynamic_conn_route);
            cross.checkForBranchingSegments = true;
            cross.countForSegment(connRoute.size() - 1, finalSegment);
            
            result += (cross.crossingCount * cluster_crossing_penalty);
        }
    }
    
    // This penalty penalises route segments that head in a direction opposite
    // of the direction(s) toward the target point.  
    const double reversePenalty = router->routingParameter(
            reverseDirectionPenalty);
    if (reversePenalty)
    {
        // X and Y direction of destination from source point.
        const Point& srcPoint = lineRef->src()->point;
        const Point& dstPoint = lineRef->dst()->point;
        int xDir = dimDirection(dstPoint.x - srcPoint.x);
        int yDir = dimDirection(dstPoint.y - srcPoint.y);
        
        bool doesReverse = false;

        if ((xDir != 0) && 
                (-xDir == dimDirection(inf3->point.x - inf2->point.x)))
        {
            // Connector has an X component and the segment heads in the 
            // opposite direction.
            doesReverse |= true;
        }

        if ((yDir != 0) && 
                (-yDir == dimDirection(inf3->point.y - inf2->point.y)))
        {
            // Connector has an Y component and the segment heads in the 
            // opposite direction.
            doesReverse |= true;
        }

        if (doesReverse)
        {
            result += reversePenalty;
        }
    }

    if (!router->isInCrossingPenaltyReroutingStage())
    {
        // Return here if we are not in the post-processing stage 
        return result;
    }

    const double crossing_penalty = router->routingParameter(crossingPenalty);
    const double shared_path_penalty = 
            router->routingParameter(fixedSharedPathPenalty);
    if ((shared_path_penalty > 0) || (crossing_penalty > 0))
    {
        if (connRoute.empty())
        {
            constructPolygonPath(connRoute, inf2, inf3, inf1Node);
        }
        ConnRefList::const_iterator curr, finish = router->connRefs.end();
        for (curr = router->connRefs.begin(); curr != finish; ++curr)
        {
            ConnRef *connRef = *curr;

            if (connRef->id() == lineRef->id())
            {
                continue;
            }
            const Avoid::PolyLine& route2 = connRef->displayRoute();
            
            bool isConn = true;
            Polygon dynamic_route2(route2);
            Polygon dynamic_conn_route(connRoute);
            const bool finalSegment = (inf3->point == lineRef->dst()->point);
            ConnectorCrossings cross(dynamic_route2, isConn, 
                    dynamic_conn_route, connRef, lineRef);
            cross.checkForBranchingSegments = true;
            cross.countForSegment(connRoute.size() - 1, finalSegment);

            if ((cross.crossingFlags & CROSSING_SHARES_PATH) &&
                    (cross.crossingFlags & CROSSING_SHARES_FIXED_SEGMENT) &&
                    (router->routingOption(
                            penaliseOrthogonalSharedPathsAtConnEnds) || 
                     !(cross.crossingFlags & CROSSING_SHARES_PATH_AT_END))) 
            {
                // Penalise unnecessary shared paths in the middle of
                // connectors.
                result += shared_path_penalty;
            }
            result += (cross.crossingCount * crossing_penalty);
        }
    }

    return result;
}

// Directions for estimated orthgonal cost, as bitflags.
static const unsigned int CostDirectionN = 1;
static const unsigned int CostDirectionE = 2;
static const unsigned int CostDirectionS = 4;
static const unsigned int CostDirectionW = 8;

#ifdef ESTIMATED_COST_DEBUG
static void printDirections(FILE *fp, unsigned int directions)
{
    if (directions & CostDirectionN)
    {
        fprintf(fp, "N ");
    }
    if (directions & CostDirectionE)
    {
        fprintf(fp, "E ");
    }
    if (directions & CostDirectionS)
    {
        fprintf(fp, "S ");
    }
    if (directions & CostDirectionW)
    {
        fprintf(fp, "W ");
    }
}
#endif

// Returns the number of directions for the argument.
static unsigned int orthogonalDirectionsCount(const unsigned int directions)
{
    unsigned int count = 0;
    if (directions & CostDirectionN)
    {
        ++count;
    }
    if (directions & CostDirectionE)
    {
        ++count;
    }
    if (directions & CostDirectionS)
    {
        ++count;
    }
    if (directions & CostDirectionW)
    {
        ++count;
    }
    return count;
}

// Returns the directions of point b from point a.
static unsigned int orthogonalDirection(const Point &a, const Point &b)
{
    unsigned int result = 0;

    if (b.y > a.y)
    {
        result |= CostDirectionS;
    }
    else if (b.y < a.y)
    {
        result |= CostDirectionN;
    }

    if (b.x > a.x)
    {
        result |= CostDirectionE;
    }
    else if (b.x < a.x)
    {
        result |= CostDirectionW;
    }

    return result;
}

// Returns the direction to the right of the given direction.
static unsigned int dirRight(unsigned int direction)
{
    if (direction == CostDirectionN)
    {
        return CostDirectionE;
    }
    else if (direction == CostDirectionE)
    {
        return CostDirectionS;
    }
    else if (direction == CostDirectionS)
    {
        return CostDirectionW;
    }
    else if (direction == CostDirectionW)
    {
        return CostDirectionN;
    }
    
    // Should not be possible to reach here.
    COLA_ASSERT(false);
    return direction;
}

// Returns the direction to the left of the given direction.
static unsigned int dirLeft(unsigned int direction)
{
    if (direction == CostDirectionN)
    {
        return CostDirectionW;
    }
    else if (direction == CostDirectionE)
    {
        return CostDirectionN;
    }
    else if (direction == CostDirectionS)
    {
        return CostDirectionE;
    }
    else if (direction == CostDirectionW)
    {
        return CostDirectionS;
    }
    
    // Should not be possible to reach here.
    COLA_ASSERT(false);
    return direction;
}

// Returns the reverse direction to the given direction.
static unsigned int dirReverse(unsigned int direction)
{
    if (direction == CostDirectionN)
    {
        return CostDirectionS;
    }
    else if (direction == CostDirectionE)
    {
        return CostDirectionW;
    }
    else if (direction == CostDirectionS)
    {
        return CostDirectionN;
    }
    else if (direction == CostDirectionW)
    {
        return CostDirectionE;
    }
    
    // Should not be possible to reach here.
    COLA_ASSERT(false);
    return direction;
}

// Given Point curr with a direction of currDir, returns the nimimum number 
// of bends to reach Point dest with the entry direction of destDir
// 
// This is used for estimating the bend penalty cost to the target point
// from the current point of the search. The geometry was described in the 
// "Orthogonal Connector Routing" paper, although the version described 
// there is incorrect.
//
int bends(const Point& curr, unsigned int currDir, const Point& dest,
        unsigned int destDir)
{
        // Bend counts from 'o' to 'D' should be:
        //
        //                1            1            3
        //                v            v            v
        //            2 > o < 2    2 > o < 2    4 > o < 2
        //                ^            ^            ^
        //                3            3            3
        //                                      
        //   0 > o < 4                 D-->             4 > o < 4
        //       ^                                          ^
        //       1                                          3
        //
    COLA_ASSERT(currDir != 0);
    unsigned int currToDestDir = orthogonalDirection(curr, dest);
    unsigned int reverseDestDir = dirReverse(destDir);
    bool currDirPerpendicularToDestDir =
            (currDir == dirLeft(destDir)) || (currDir == dirRight(destDir));

    if ((currDir == destDir) &&
             (currToDestDir == currDir))
    {
        //                                      
        //   0 > o                     D--> 
        //
        return 0;
    }
    else if (currDirPerpendicularToDestDir &&
             (currToDestDir == (destDir | currDir)))
    {
        //
        //                1
        //                v
        //                o
        //                 
        //                                      
        //                             D-->
        //
        return 1;
    }
    else if (currDirPerpendicularToDestDir &&
             (currToDestDir == currDir))
    {
        //
        //                             1
        //                             v
        //                             o
        //                 
        //                                      
        //                             D-->
        //
        return 1;
    }
    else if (currDirPerpendicularToDestDir &&
             (currToDestDir == destDir))
    {
        //
        //       o                     D-->
        //       ^
        //       1
        //
        return 1;
    }
    else if ((currDir == destDir) && 
             (currToDestDir != currDir) &&
             !(currToDestDir & reverseDestDir))
    {
        //
        //            2 > o        2 > o
        //
        //                                      
        //                             D-->
        //
        return 2;
    }
    else if (currDir == reverseDestDir && 
             (currToDestDir != destDir) &&
             (currToDestDir != currDir))
    {
        //
        //                o < 2        o < 2       o < 2
        //
        //                                      
        //                             D-->
        //
        return 2;
    }
    else if (currDirPerpendicularToDestDir &&
             (currToDestDir != (destDir | currDir)) && 
             (currToDestDir != currDir))
    {
        //
        //                                          3
        //                                          v
        //                o            o            o
        //                ^            ^            ^
        //                3            3            3
        //                                      
        //                             D-->                 o
        //                                                  ^
        //                                                  3
        //
        return 3;
    }
    else if ((currDir == reverseDestDir) && 
             ((currToDestDir == destDir) || (currToDestDir == currDir)))
    {
        //
        //                                      
        //                                      
        //       o < 4                 D-->                 o < 4
        //
        return 4;
    }
    else if ((currDir == destDir) && 
             (currToDestDir & reverseDestDir))
    {
        //
        //                                      4 > o
        //                                      
        //                                      
        //                             D-->             4 > o
        //
        return 4;
    }

    // Should not be possible to reach here.
    COLA_ASSERT(false);
    return 0;
}


static double estimatedCostSpecific(ConnRef *lineRef, const Point *last,
        const Point& curr, const VertInf *costTar,
        const unsigned int costTarDirs)
{
    Point costTarPoint = costTar->point;

    if (lineRef->routingType() == ConnType_PolyLine)
    {
        return euclideanDist(curr, costTarPoint);
    }
    else // Orthogonal
    {
        // Really doesn't make sense to route orthogonal paths without 
        // a segment penalty.
        COLA_ASSERT(lineRef->router()->routingParameter(segmentPenalty) > 0);

        double dist = manhattanDist(curr, costTarPoint);

        int bendCount = 0;
        double xmove = costTarPoint.x - curr.x;
        double ymove = costTarPoint.y - curr.y;
        if (last == nullptr)
        {
            // This is just the initial point.  Penalise it simply if it is 
            // not inline with the target in either the x- or y-dimension.
            if ((xmove != 0) && (ymove != 0))
            {
                bendCount += 1;
            }
        }
        else if (dist > 0)
        {
            // We have two points and a non-zero distance, so we know 
            // the segment direction.

            unsigned int currDir = orthogonalDirection(*last, curr);
            if ((currDir > 0) && (orthogonalDirectionsCount(currDir) == 1))
            {
                // Suitably high value, then we find the minimum.
                bendCount = 10;

                // Find the minimum bent penalty given all the possible 
                // directions at the target point.
                if (costTarDirs & CostDirectionN)
                {
                    bendCount = std::min(bendCount,
                            bends(curr, currDir, costTarPoint, CostDirectionN));
                }
                if (costTarDirs & CostDirectionE)
                {
                    bendCount = std::min(bendCount,
                            bends(curr, currDir, costTarPoint, CostDirectionE));
                }
                if (costTarDirs & CostDirectionS)
                {
                    bendCount = std::min(bendCount,
                            bends(curr, currDir, costTarPoint, CostDirectionS));
                }
                if (costTarDirs & CostDirectionW)
                {
                    bendCount = std::min(bendCount,
                            bends(curr, currDir, costTarPoint, CostDirectionW));
                }
            }
        }
        double penalty = bendCount *
                lineRef->router()->routingParameter(segmentPenalty);

        return dist + penalty;
    }
}



double AStarPathPrivate::estimatedCost(ConnRef *lineRef, const Point *last,
        const Point& curr) const
{
    double estimate = DBL_MAX;
    COLA_ASSERT(m_cost_targets.size() > 0);

    // Find the minimum cost from the estimates to each of the possible
    // target points from this current point.
    for (size_t i = 0; i < m_cost_targets.size(); ++i)
    {
        double iEstimate = estimatedCostSpecific(lineRef, last,
                curr, m_cost_targets[i], m_cost_targets_directions[i]);
        
        // Add on the distance to the real target, otherwise this difference
        // might may make the comparisons unfair if they vary between targets.
        iEstimate += m_cost_targets_displacements[i];
        
        estimate = std::min(estimate, iEstimate);
    }
    return estimate;
}


class CmpVisEdgeRotation 
{
    public:
        CmpVisEdgeRotation(const VertInf* lastPt)
            : _lastPt(lastPt)
        {
        }
        bool operator() (const EdgeInf* u, const EdgeInf* v) const 
        {
            // Dummy ShapeConnectionPin edges are not orthogonal and 
            // therefore can't be compared in the same way.
            if (u->isOrthogonal() && v->isOrthogonal())
            {
                return u->rotationLessThan(_lastPt, v);
            }
            return u < v;
        }
    private:
        const VertInf *_lastPt;
};


static inline bool pointAlignedWithOneOf(const Point& point, 
        const std::vector<Point>& points, const size_t dim)
{
    for (size_t i = 0; i < points.size(); ++i)
    {
        if (point[dim] == points[i][dim])
        {
            return true;
        }
    }
    return false;
}

AStarPath::AStarPath(void)
    : m_private(new AStarPathPrivate())
{
}

AStarPath::~AStarPath(void)
{
    delete m_private;
}

void AStarPath::search(ConnRef *lineRef, VertInf *src, VertInf *tar, VertInf *start)
{
    m_private->search(lineRef, src, tar, start);
}

void AStarPathPrivate::determineEndPointLocation(double dist, VertInf *start, 
        VertInf *target, VertInf *other, int level)
{
    COLA_UNUSED(dist);
    COLA_UNUSED(start);
    COLA_UNUSED(level);

    Point otherPoint = other->point;
    unsigned int thisDirs = orthogonalDirection(otherPoint, target->point);
    COLA_ASSERT(orthogonalDirectionsCount(thisDirs) > 0);
    double displacement = manhattanDist(otherPoint, target->point);

    m_cost_targets.push_back(other);
    m_cost_targets_directions.push_back(thisDirs);
    m_cost_targets_displacements.push_back(displacement);

#ifdef ESTIMATED_COST_DEBUG
    fprintf(stderr," - %g %g ", otherPoint.x, otherPoint.y);
    if (manhattanDist(start->point, otherPoint) > dist)
    {
        fprintf(stderr,"far ");
    }
    fprintf(stderr, "%s", (level == 1) ? "--" : "- ");
    printDirections(stderr, thisDirs);
    fprintf(stderr,"\n");
#endif
}

// Returns the best path from src to tar using the cost function.
//
// The path is worked out using the aStar algorithm, and is encoded via
// prevNode values for each ANode which point back to the previous ANode.
// At completion, this order is written into the pathNext links in each 
// of the VerInfs along the path.
//
// The aStar STL code is originally based on public domain code available 
// on the internet.
//
void AStarPathPrivate::search(ConnRef *lineRef, VertInf *src, VertInf *tar, VertInf *start)
{
    ANodeCmp pendingCmp;

    bool isOrthogonal = (lineRef->routingType() == ConnType_Orthogonal);

    if (start == nullptr)
    {
        start = src;
    }

#ifdef DEBUGHANDLER
    if (lineRef->router()->debugHandler())
    {
        lineRef->router()->debugHandler()->beginningSearchWithEndpoints(start, tar);
    }
#endif

    // Find a target point to use for cost estimate for orthogonal routing.
    //
    // If the connectivity is only on the far side we need to estimate to the
    // point on the far side.  Otherwise for orthogonal routing we can explore
    // all the space in between before we pay the extra cost to explore this
    // area.  This is especially true given many orthogonal routes have 
    // equivalent costs.
#ifdef ESTIMATED_COST_DEBUG
    fprintf(stderr,"== aStar  %g %g ==\n", tar->point.x, tar->point.y);
#endif
    if (isOrthogonal && tar->id.isConnPt() && !tar->id.isConnCheckpoint())
    {
        // The target is a connector endpoint and the connector is orthogonal.
        double dist = manhattanDist(start->point, tar->point);
        for (EdgeInfList::const_iterator it = tar->orthogVisList.begin(); 
                it != tar->orthogVisList.end(); ++it)
        {
            // For each edge from the target endpoint, find the other vertex.
            EdgeInf *edge = *it;
            VertInf *other = edge->otherVert(tar);
            if (other->id.isConnectionPin())
            {
                // If this is a connection pin we need to do this process
                // another time since the current edge will be a dummy 
                // zero-length edge.
                VertInf *replacementTar = other;
                for (EdgeInfList::const_iterator it = 
                        replacementTar->orthogVisList.begin(); 
                        it != replacementTar->orthogVisList.end(); ++it)
                {
                    EdgeInf *edge = *it;
                    VertInf *other = edge->otherVert(replacementTar);
                    if ((other == tar) || 
                            (other->point == tar->point))
                    {
                        // Ignore edge we came from, or zero-length edges.
                        continue;
                    }

                    // Determine possible target endpoint directions and 
                    // position.
                    determineEndPointLocation(dist, start, replacementTar,
                            other, 2);
                }
                continue;
            }
                    
            // Determine possible target endpoint directions and position.
            determineEndPointLocation(dist, start, tar, other, 1);
        }
    }


    if (m_cost_targets.empty())
    {
        m_cost_targets.push_back(tar);
        // For polyline routing, assume target has visibility is all 
        // directions for the purpose of cost estimations.
        m_cost_targets_directions.push_back(CostDirectionN |
                CostDirectionE | CostDirectionS | CostDirectionW);
        m_cost_targets_displacements.push_back(0.0);
    }

#ifdef ESTIMATED_COST_DEBUG
    fprintf(stderr, "------------\n");
    for (size_t i = 0; i < m_cost_targets.size(); ++i)
    {
        fprintf(stderr,"== %g %g - ", m_cost_targets[i]->point.x,
                m_cost_targets[i]->point.y);
        printDirections(stderr, m_cost_targets_directions[i]);
        fprintf(stderr,"\n");
    }
#endif


    double (*dist)(const Point& a, const Point& b) = 
        (isOrthogonal) ? manhattanDist : euclideanDist;

    // We need to know the possible endpoints for doing an orthogonal 
    // routing optimisation where we only turn when we are heading beside
    // a shape or are in line with a possible endpoint.
    std::vector<Point> endPoints;
    if (isOrthogonal)
    {
        endPoints = lineRef->possibleDstPinPoints();
    }
    endPoints.push_back(tar->point);
    
    // Heap of PENDING nodes.
    std::vector<ANode *> PENDING;
    PENDING.reserve(1000);

    size_t exploredCount = 0;
    ANode node, ati;
    ANode *bestNode = nullptr;         // Temporary bestNode
    bool bNodeFound = false;        // Flag if node is found in container
    int timestamp = 1;

    Router *router = lineRef->router();
    if (router->RubberBandRouting && (start != src))
    {
        COLA_ASSERT(router->IgnoreRegions == true);
        
        const PolyLine& currRoute = lineRef->route();
        VertInf *last = nullptr;
        int rIndx = 0;
        while (last != start)
        {
            const Point& pnt = currRoute.at(rIndx);
            VertIDProps props = (rIndx > 0) ? 0 : VertID::PROP_ConnPoint;
            VertID vID(pnt.id, pnt.vn, props);

#ifdef PATHDEBUG
            db_printf("/// %d %d\n", pnt.id, pnt.vn);
#endif
            VertInf *curr = router->vertices.getVertexByID(vID);
            COLA_ASSERT(curr != nullptr);

            node = ANode(curr, timestamp++);
            if (!last)
            {
                node.inf = src;
                node.g = 0;
                node.h = estimatedCost(lineRef, nullptr, node.inf->point);

                node.f = node.g + node.h;
            }
            else
            {
                double edgeDist = dist(bestNode->inf->point, curr->point);

                node.g = bestNode->g + cost(lineRef, edgeDist, bestNode->inf, 
                        node.inf, bestNode->prevNode);

                // Calculate the Heuristic.
                node.h = estimatedCost(lineRef, &(bestNode->inf->point),
                        node.inf->point);

                // The A* formula
                node.f = node.g + node.h;
                
                // Point parent to last bestNode
                node.prevNode = bestNode;
            }

            if (curr != start)
            {
                bool addToPending = false;
                bestNode = newANode(node, addToPending);
                bestNode->inf->aStarDoneNodes.push_back(bestNode);
                ++exploredCount;
            }
            else
            {
                ANode * newNode = newANode(node);
                PENDING.push_back(newNode);
            }

            rIndx++;
            last = curr;
        }
    }
    else
    {
        if (start->pathNext)
        {
            // If we are doing checkpoint routing and have already done one
            // path, then we have an existing segment to consider for the 
            // cost of the  choice from the start node, so we add a dummy 
            // nodes as if they were already in the Done set.  This causes 
            // us to first search in a collinear direction from the previous 
            // segment.
            bool addToPending = false;
            bestNode = newANode(ANode(start->pathNext, timestamp++), 
                    addToPending);
            bestNode->inf->aStarDoneNodes.push_back(bestNode);
            ++exploredCount;
        }

        // Create the start node
        node = ANode(src, timestamp++);
        node.g = 0;
        node.h = estimatedCost(lineRef, nullptr, node.inf->point);
        node.f = node.g + node.h;
        // Set a nullptr parent, so cost function knows this is the first segment.
        node.prevNode = bestNode;

        // Populate the PENDING container with the first location
        ANode *newNode = newANode(node);
        PENDING.push_back(newNode);
    }

    tar->pathNext = nullptr;

    // Create a heap from PENDING for sorting
    using std::make_heap; using std::push_heap; using std::pop_heap;
    make_heap( PENDING.begin(), PENDING.end(), pendingCmp);

    // Continue until the queue is empty.
    while (!PENDING.empty())
    {
        TIMER_VAR_ADD(router, 0, 1);
        // Set the Node with lowest f value to BESTNODE.
        // Since the ANode operator< is reversed, the head of the
        // heap is the node with the lowest f value.
        bestNode = PENDING.front();
        VertInf *bestNodeInf = bestNode->inf;

#ifdef DEBUGHANDLER
        if (router->debugHandler())
        {
            PolyLine currentSearchPath;

            ANode *curr = bestNode;
            while (curr)
            {
                currentSearchPath.ps.push_back(curr->inf->point);
                curr = curr->prevNode;
            }
            router->debugHandler()->updateCurrentSearchPath(currentSearchPath);
        }
#endif

        // Remove this node from the aStarPendingList
        std::list<ANode *>::iterator finishIt = 
                bestNodeInf->aStarPendingNodes.end();
        for (std::list<ANode *>::iterator currInd = 
                bestNodeInf->aStarPendingNodes.begin(); currInd != finishIt; 
                ++currInd)
        {
            if (*currInd == bestNode)
            {
                bestNodeInf->aStarPendingNodes.erase(currInd);
                break;
            }
        }

        // Pop off the heap.  Actually this moves the
        // far left value to the far right.  The node
        // is not actually removed since the pop is to
        // the heap and not the container.
        pop_heap(PENDING.begin(), PENDING.end(), pendingCmp);
        // Remove node from right (the value we pop_heap'd)
        PENDING.pop_back();

        // Add the bestNode into the Done set.
        bestNodeInf->aStarDoneNodes.push_back(bestNode);
        ++exploredCount;

        VertInf *prevInf = (bestNode->prevNode) ? bestNode->prevNode->inf : nullptr;
#ifdef ASTAR_DEBUG
        db_printf("Considering... ");
        db_printf(" %g %g  ", bestNodeInf->point.x, bestNodeInf->point.y);
        bestNodeInf->id.db_print();
        db_printf(" - g: %3.1f h: %3.1f back: ", bestNode->g, bestNode->h);
        if (prevInf)
        {
            db_printf(" %g %g", prevInf->point.x, prevInf->point.y);
            //prevInf->id.db_print();
        }
        db_printf("\n");
#endif

        if (bestNodeInf == tar)
        {
            TIMER_VAR_ADD(router, 1, PENDING.size());
            // This node is our goal.
#ifdef ASTAR_DEBUG
            db_printf("LINE %10d  Steps: %4d  Cost: %g\n", lineRef->id(), 
                    (int) exploredCount, bestNode->f);
#endif
     
            // Correct all the pathNext pointers.
            for (ANode *curr = bestNode; curr->prevNode; curr = curr->prevNode)
            {
#ifdef ASTAR_DEBUG
                db_printf("[%.12f, %.12f]\n", curr->inf->point.x, curr->inf->point.y);
#endif
                curr->inf->pathNext = curr->prevNode->inf;
            }
#ifdef ASTAR_DEBUG
            db_printf("\n");
#endif

            // Exit from the search
            break;
        }

        // Check adjacent points in graph and add them to the queue.
        EdgeInfList& visList = (!isOrthogonal) ?
                bestNodeInf->visList : bestNodeInf->orthogVisList;
        if (isOrthogonal)
        {
            // We would like to explore in a structured way, 
            // so sort the points in the visList...
            CmpVisEdgeRotation compare(prevInf);
            visList.sort(compare);
        }
        EdgeInfList::const_iterator finish = visList.end();
        for (EdgeInfList::const_iterator edge = visList.begin(); 
                edge != finish; ++edge)
        {
            if ((*edge)->isDisabled())
            {
                // Skip disabled edges.
                continue;
            }

            node = ANode((*edge)->otherVert(bestNodeInf), timestamp++);
            
            // Set the index to the previous ANode that we reached
            // this ANode via.
            node.prevNode = bestNode;

            VertInf *prevInf = (bestNode->prevNode) ?
                    bestNode->prevNode->inf : nullptr;

            // Don't bother looking at the segment we just arrived along.
            if (prevInf && (prevInf == node.inf))
            {
                continue;
            }
            if (node.inf->id.isConnectionPin() && 
                    !node.inf->id.isConnCheckpoint())
            {
                if ( !( (bestNodeInf == lineRef->src()) &&
                        lineRef->src()->id.isDummyPinHelper()
                      ) &&
                     !( node.inf->hasNeighbour(lineRef->dst(), isOrthogonal) &&
                        lineRef->dst()->id.isDummyPinHelper())
                      )
                {
                    // Don't check connection pins if they don't have the 
                    // target vertex as a direct neighbour, or are directly
                    // leaving the source vertex.
                    continue;
                }
            }
            else if (node.inf->id.isConnPt())
            {
                if ((node.inf != tar))
                {
                    // Don't check connector endpoints vertices unless they
                    // are the target endpoint.
                    continue;
                }
            }

            if (isOrthogonal && !(*edge)->isDummyConnection())
            {
                // Orthogonal routing optimisation.
                // Skip the edges that don't lead to shape edges, or the 
                // connection point we are looking for.  Though allow them
                // if we haven't yet turned from the source point, since it
                // may be a free-floating endpoint with directional visibility.
                // Also, don't check if the previous point was a dummy for a
                // connection pin and this happens to be placed diagonally 
                // from here, i.e., when both of notInline{X,Y} are true.
                Point& bestPt = bestNodeInf->point;
                Point& nextPt = node.inf->point;

                bool notInlineX = prevInf && (prevInf->point.x != bestPt.x);
                bool notInlineY = prevInf && (prevInf->point.y != bestPt.y);
                if ((bestPt.x == nextPt.x) && notInlineX && !notInlineY &&
                        (bestPt[YDIM] != src->point[YDIM]))
                {
                    if (nextPt.y < bestPt.y)
                    {
                        if (!(bestNodeInf->orthogVisPropFlags & YL_EDGE) &&
                                !pointAlignedWithOneOf(bestPt, endPoints, XDIM))
                        {
                            continue;
                        }
                    }
                    else if (nextPt.y > bestPt.y)
                    {
                        if (!(bestNodeInf->orthogVisPropFlags & YH_EDGE) &&
                                !pointAlignedWithOneOf(bestPt, endPoints, XDIM))
                        {
                            continue;
                        }
                    }
                }
                if ((bestPt.y == nextPt.y) && notInlineY && !notInlineX &&
                        (bestPt[XDIM] != src->point[XDIM]))
                {
                    if (nextPt.x < bestPt.x)
                    {
                        if (!(bestNodeInf->orthogVisPropFlags & XL_EDGE) &&
                                !pointAlignedWithOneOf(bestPt, endPoints, YDIM))
                        {
                            continue;
                        }
                    }
                    else if (nextPt.x > bestPt.x)
                    {
                        if (!(bestNodeInf->orthogVisPropFlags & XH_EDGE) &&
                                !pointAlignedWithOneOf(bestPt, endPoints, YDIM))
                        {
                            continue;
                        }
                    }
                }
            }

            double edgeDist = (*edge)->getDist();

            if (edgeDist == 0)
            {
                continue;
            }

            if (!isOrthogonal &&
                  (!router->RubberBandRouting || (start == src)) && 
                  (validateBendPoint(prevInf, bestNodeInf, node.inf) == false))
            {
                // The bendpoint is not valid, i.e., is a zigzag corner, so...
                continue;
                // For RubberBand routing we want to allow these routes and
                // unwind them later, otherwise instead or unwinding, paths
                // can go the *really* long way round.
            }

            // Figure out if we are at one of the cost targets.
            bool atCostTarget = false;
            for (size_t i = 0; i < m_cost_targets.size(); ++i)
            {
                if (bestNode->inf == m_cost_targets[i])
                        
                {
                    atCostTarget = true;
                    break;
                }
            }

            if (atCostTarget &&
                    (node.inf->id.isConnectionPin() || (node.inf == tar)))
            {
                // This is a point on the side of an obstacle that connects
                // to the target or a connection pin.  It should have no 
                // further cost and the heuristic should be zero.
                node.g = bestNode->g;
                node.h = 0;
            }
            else 
            {
                if (node.inf == tar)
                {
                    // We've reached the target.  The heuristic should be zero.
                    node.h = 0;
                }
                else
                {
                    // Otherwise, calculate the heuristic value.
                    node.h = estimatedCost(lineRef, &(bestNodeInf->point),
                            node.inf->point);
                }
                
                if (node.inf->id.isDummyPinHelper())
                {
                    // This is connecting to a connection pin helper vertex.
                    // There should be no additional cost for this step.
                    node.g = bestNode->g;
                }
                else
                {
                    // Otherwise, calculate the cost of this step.
                    node.g = bestNode->g + cost(lineRef, edgeDist, bestNodeInf, 
                            node.inf, bestNode->prevNode);
                }
            }

            // The A* formula
            node.f = node.g + node.h;

#ifdef ASTAR_DEBUG
            db_printf("-- Adding: %g %g  ", node.inf->point.x, 
                    node.inf->point.y);
            node.inf->id.db_print();
            db_printf(" - g: %3.1f h: %3.1f \n", node.g, node.h);
#endif

            bNodeFound = false;

    
            // Check to see if already on PENDING
            std::list<ANode *>::const_iterator finish = node.inf->aStarPendingNodes.end();
            for (std::list<ANode *>::const_iterator currInd = 
                    node.inf->aStarPendingNodes.begin(); currInd != finish; ++currInd)
            {
                ati = **currInd;
                // The (node.prevNode == ati.prevNode) is redundant, but may
                // save checking the mosre costly prevNode->inf test if the
                // Nodes are the same.
                if ((node.inf == ati.inf) && 
                        ((node.prevNode == ati.prevNode) ||
                         (node.prevNode->inf == ati.prevNode->inf)))
                {
                    // If already on PENDING
                    if (node.g < ati.g)
                    {
                        // Replace the existing node in PENDING
                        **currInd = node;
                        make_heap( PENDING.begin(), PENDING.end(), pendingCmp);
                    }
                    bNodeFound = true;
                    break;
                }
            }
            if ( !bNodeFound ) // If Node NOT found on PENDING
            {
                // Check to see if it is already in the Done set for this
                // vertex.
                for (std::list<ANode *>::const_iterator currInd = 
                        node.inf->aStarDoneNodes.begin();
                        currInd != node.inf->aStarDoneNodes.end(); ++currInd)
                {
                    ati = **currInd;
                    // The (node.prevNode == ati.prevNode) is redundant, but may
                    // save checking the mosre costly prevNode->inf test if the
                    // Nodes are the same.
                    if ((node.inf == ati.inf) && ati.prevNode &&
                            ((node.prevNode == ati.prevNode) ||
                             (node.prevNode->inf == ati.prevNode->inf)))
                    {
                        //COLA_ASSERT(node.g >= (ati.g - 10e-10));
                        // This node is already in the Done set and the 
                        // current node also has a higher g-value, so we 
                        // don't need to consider this node.
                        bNodeFound = true;
                        break;
                    }
                }
            }

            if (!bNodeFound ) // If Node NOT in either Pending or Done.
            {
                // Push NewNode onto PENDING
                ANode *newNode = newANode(node);
                PENDING.push_back(newNode);
                // Push NewNode onto heap
                push_heap( PENDING.begin(), PENDING.end(), pendingCmp);

#if 0
                using std::cout; using std::endl;
                // Display PENDING container (For Debugging)
                cout << "PENDING:   ";
                for (unsigned int i = 0; i < PENDING.size(); i++)
                {
                    cout << PENDING[i]->g << "," << PENDING[i]->h << ",";
                    cout << PENDING[i]->inf << "," << PENDING[i]->pp << "  ";
                }
                cout << endl << endl;
#endif
            }
        }
    }

    // Cleanup lists used to store Done and Pending sets for each vertex.
    VertInf *endVert = router->vertices.end();
    for (VertInf *k = router->vertices.connsBegin(); k != endVert;
            k = k->lstNext)
    {
        k->aStarDoneNodes.clear();
        k->aStarPendingNodes.clear();
    }
}


}



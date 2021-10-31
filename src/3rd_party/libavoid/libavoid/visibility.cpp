/*
 * vim: ts=4 sw=4 et tw=0 wm=0
 *
 * libavoid - Fast, Incremental, Object-avoiding Line Router
 *
 * Copyright (C) 2004-2009  Monash University
 *
 * --------------------------------------------------------------------
 * The Visibility Sweep technique is based upon the method described
 * in Section 5.2 of:
 *     Lee, D.-T. (1978). Proximity and reachability in the plane.,
 *     PhD thesis, Department of Electrical Engineering, 
 *     University of Illinois, Urbana, IL.
 * --------------------------------------------------------------------
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


#include <algorithm>
#include <cfloat>

#include "libavoid/shape.h"
#include "libavoid/debug.h"
#include "libavoid/visibility.h"
#include "libavoid/vertices.h"
#include "libavoid/graph.h"
#include "libavoid/geometry.h"
#include "libavoid/router.h"
#include "libavoid/assertions.h"


namespace Avoid {


static void vertexSweep(VertInf *vert);

void Obstacle::computeVisibilityNaive(void)
{
    if ( !(router()->InvisibilityGrph) )
    {
        // Clear shape from graph.
        removeFromGraph();
    }

    VertInf *shapeBegin = firstVert();
    VertInf *shapeEnd = lastVert()->lstNext;

    VertInf *pointsBegin = router()->vertices.connsBegin();
    for (VertInf *curr = shapeBegin; curr != shapeEnd; curr = curr->lstNext)
    {
        bool knownNew = true;

        db_printf("-- CONSIDERING --\n");
        curr->id.db_print();

        db_printf("\tFirst Half:\n");
        for (VertInf *j = pointsBegin ; j != curr; j = j->lstNext)
        {
            if (j->id == dummyOrthogID)
            {
                // Don't include orthogonal dummy vertices.
                continue;
            }
            EdgeInf::checkEdgeVisibility(curr, j, knownNew);
        }

        db_printf("\tSecond Half:\n");
        VertInf *pointsEnd = router()->vertices.end();
        for (VertInf *k = shapeEnd; k != pointsEnd; k = k->lstNext)
        {
            if (k->id == dummyOrthogID)
            {
                // Don't include orthogonal dummy vertices.
                continue;
            }
            EdgeInf::checkEdgeVisibility(curr, k, knownNew);
        }
    }
}


void Obstacle::computeVisibilitySweep(void)
{
    if ( !(router()->InvisibilityGrph) )
    {
        // Clear shape from graph.
        removeFromGraph();
    }

    VertInf *startIter = firstVert();
    VertInf *endIter = lastVert()->lstNext;

    for (VertInf *i = startIter; i != endIter; i = i->lstNext)
    {
        vertexSweep(i);
    }
}


void vertexVisibility(VertInf *point, VertInf *partner, bool knownNew,
        const bool gen_contains)
{
    Router *router = point->_router;
    const VertID& pID = point->id;

    // Make sure we're only doing ptVis for endpoints.
    COLA_ASSERT(pID.isConnPt());

    if ( !(router->InvisibilityGrph) )
    {
        point->removeFromGraph();
    }

    if (gen_contains && pID.isConnPt())
    {
        router->generateContains(point);
    }

    if (router->UseLeesAlgorithm)
    {
        vertexSweep(point);
    }
    else
    {
        VertInf *shapesEnd = router->vertices.end();
        for (VertInf *k = router->vertices.connsBegin(); k != shapesEnd;
                k = k->lstNext)
        {
            if (k->id == dummyOrthogID)
            {
                // Don't include orthogonal dummy vertices.
                continue;
            }
            else if (k->id.isConnPt() && !k->id.isConnectionPin() &&
                    !(k->id.isConnCheckpoint() && k->id.objID == pID.objID))
            {
                // Include connection pins, but not connectors.
                // Also include checkpoints with same ID as sweep point.
                continue;
            }
            EdgeInf::checkEdgeVisibility(point, k, knownNew);
        }
        if (partner)
        {
            EdgeInf::checkEdgeVisibility(point, partner, knownNew);
        }
    }
}


//============================================================================
//  SWEEP CODE
//


class PointPair
{
    public:
        PointPair(const Point& centerPoint, VertInf *inf)
            : vInf(inf),
              centerPoint(centerPoint)
        {
            angle = rotationalAngle(vInf->point - centerPoint);
            distance = euclideanDist(centerPoint, vInf->point);
        }
        bool operator<(const PointPair& rhs) const
        {
            // Firstly order by angle.
            if (angle == rhs.angle)
            {
                // If the points are collinear, then order them in increasing
                // distance from the point we are sweeping around.
                if (distance == rhs.distance)
                {
                    // XXX: Add this assertion back if we require that 
                    //      connector endpoints have unique IDs. For the 
                    //      moment it is okay for them to have the same ID.
                    //COLA_ASSERT(vInf->id != rhs.vInf->id);
                    
                    // If comparing two points at the same physical 
                    // position, then order them by their VertIDs.
                    return vInf->id < rhs.vInf->id;
                }
                return distance < rhs.distance;
            }
            return angle < rhs.angle;
        }

        VertInf    *vInf;
        double     angle;
        double     distance;
        Point      centerPoint;
};

typedef std::set<PointPair > VertSet;


class EdgePair
{
    public:
        EdgePair() 
            : vInf1(nullptr), 
              vInf2(nullptr), 
              dist1(0.0),
              dist2(0.0),
              angle(0.0),
              angleDist(0.0)
        {
            // The default constuctor should never be called.  
            // This is defined to appease the MSVC compiler.
            COLA_ASSERT(false);
        }
        EdgePair(const PointPair& p1, VertInf *v) 
            : vInf1(p1.vInf), 
              vInf2(v),
              dist1(p1.distance),
              dist2(euclideanDist(vInf2->point, p1.centerPoint)),
              angle(p1.angle),
              angleDist(p1.distance),
              centerPoint(p1.centerPoint)
        {
        }
        bool operator<(const EdgePair& rhs) const
        {
            COLA_ASSERT(angle == rhs.angle);
            if (angleDist == rhs.angleDist)
            {
                return (dist2 < rhs.dist2);
            }
            return (angleDist < rhs.angleDist);
        }
        bool operator==(const EdgePair& rhs) const
        {
            if (((vInf1->id == rhs.vInf1->id) &&
                        (vInf2->id == rhs.vInf2->id)) ||
                ((vInf1->id == rhs.vInf2->id) &&
                        (vInf2->id == rhs.vInf1->id)))
            {
                return true;
            }
            return false;
        }
        bool operator!=(const EdgePair& rhs) const
        {
            if (((vInf1->id == rhs.vInf1->id) &&
                        (vInf2->id == rhs.vInf2->id)) ||
                ((vInf1->id == rhs.vInf2->id) &&
                        (vInf2->id == rhs.vInf1->id)))
            {
                return false;
            }
            return true;
        }
        void setNegativeAngle(void)
        {
            angle = -1.0;
        }
        double setCurrAngle(const PointPair& p)
        {
            if (p.vInf->point == vInf1->point)
            {
                angleDist = dist1;
                angle = p.angle;
            }
            else if (p.vInf->point == vInf2->point)
            {
                angleDist = dist2;
                angle = p.angle;
            }
            else if (p.angle != angle)
            {
                COLA_ASSERT(p.angle > angle);
                angle = p.angle;
                Point pp;
                int result = rayIntersectPoint(vInf1->point, vInf2->point,
                        centerPoint, p.vInf->point, &(pp.x), &(pp.y));
                if (result != DO_INTERSECT) 
                {
                    // This can happen with points that appear to have the
                    // same angle but at are at slightly different positions
                    angleDist = std::min(dist1, dist2);
                }
                else
                {
                    angleDist = euclideanDist(pp, centerPoint);
                }
            }

            return angleDist;
        }

        VertInf *vInf1;
        VertInf *vInf2;
        double dist1;
        double dist2;
        double angle;
        double angleDist;
        Point centerPoint;
};

typedef std::list<EdgePair> SweepEdgeList;


#define AHEAD    1
#define BEHIND  -1

class isBoundingShape
{
    public:
        // Class instance remembers the ShapeSet.
        isBoundingShape(ShapeSet& set) : 
            ss(set)
        { }
        // The following is an overloading of the function call operator.
        bool operator () (const PointPair& pp)
        {
            if (!(pp.vInf->id.isConnPt()) &&
                    (ss.find(pp.vInf->id.objID) != ss.end()))
            {
                return true;
            }
            return false;
        }
    private:
        // MSVC wants to generate the assignment operator and the default 
        // constructor, but fails.  Therefore we declare them private and 
        // don't implement them.
        isBoundingShape & operator=(isBoundingShape const &);
        isBoundingShape();

        ShapeSet& ss;
};


static bool sweepVisible(SweepEdgeList& T, const PointPair& point, 
        std::set<unsigned int>& onBorderIDs, int *blocker)
{
    if (T.empty())
    {
        // No blocking edges.
        return true;
    }

    Router *router = point.vInf->_router;
    bool visible = true;

    SweepEdgeList::const_iterator closestIt = T.begin();
    SweepEdgeList::const_iterator end = T.end();
    while (closestIt != end)
    {
        if ((point.vInf->point == closestIt->vInf1->point) ||
                (point.vInf->point == closestIt->vInf2->point))
        {
            // If the ray intersects just the endpoint of a 
            // blocking edge then ignore that edge.
            ++closestIt;
            continue;
        }
        break;
    }
    if (closestIt == end)
    {
        return true;
    }

    if (point.vInf->id.isConnPt())
    {
        // It's a connector endpoint, so we have to ignore 
        // edges of containing shapes for determining visibility.
        ShapeSet& rss = router->contains[point.vInf->id];
        while (closestIt != end)
        {
            if (rss.find(closestIt->vInf1->id.objID) == rss.end())
            {
                // This is not a containing edge so do the normal 
                // test and then stop.
                if (point.distance > closestIt->angleDist)
                {
                    visible = false;
                }
                else if ((point.distance == closestIt->angleDist) && 
                        onBorderIDs.find(closestIt->vInf1->id.objID) != 
                                onBorderIDs.end())
                {
                    // Touching, but centerPoint is on another edge of
                    // shape shape, so count as blocking.
                    visible = false;
                }
                break;
            }
            // This was a containing edge, so consider the next along.
            ++closestIt;
        }
    }
    else
    {
        // Just test to see if this point is closer than the closest 
        // edge blocking this ray.
        if (point.distance > closestIt->angleDist)
        {
            visible =  false;
        }
        else if ((point.distance == closestIt->angleDist) && 
                onBorderIDs.find(closestIt->vInf1->id.objID) != 
                        onBorderIDs.end())
        {
            // Touching, but centerPoint is on another edge of
            // shape shape, so count as blocking.
            visible = false;
        }
    }

    if (!visible)
    {
        *blocker = (*closestIt).vInf1->id.objID;
    }
    return visible;
}


static void vertexSweep(VertInf *vert)
{
    Router *router = vert->_router;
    VertID& pID = vert->id;
    Point& pPoint = vert->point;

    VertInf *centerInf = vert;
    VertID centerID = pID;
    Point centerPoint = pPoint;

    // List of shape (and maybe endpt) vertices, except p
    // Sort list, around
    VertSet v;

    // Initialise the vertex list
    ShapeSet& ss = router->contains[centerID];
    VertInf *beginVert = router->vertices.connsBegin();
    VertInf *endVert = router->vertices.end();
    for (VertInf *inf = beginVert; inf != endVert; inf = inf->lstNext)
    {
        if (inf == centerInf)
        {
            // Don't include the center point itself.
            continue;
        }
        else if (inf->id == dummyOrthogID)
        {
            // Don't include orthogonal dummy vertices.
            continue;
        }

        if (centerID.isConnPt() && (ss.find(inf->id.objID) != ss.end()) &&
                !inf->id.isConnPt())
        {
            // Don't include edge points of containing shapes.
            unsigned int shapeID = inf->id.objID;
            db_printf("Center is inside shape %u so ignore shape edges.\n",
                    shapeID);
            continue;
        }
        
        if (inf->id.isConnPt())
        {
            // Add connector endpoint.
            if (centerID.isConnPt())
            {
                if (inf->id.isConnectionPin())
                {
                    v.insert(PointPair(centerPoint, inf));
                }
                else if (centerID.isConnectionPin())
                {
                    // Connection pins have visibility to everything.
                    v.insert(PointPair(centerPoint, inf));
                }
                else if (inf->id.objID == centerID.objID)
                {
                    // Center is an endpoint, so only include the other
                    // endpoints or checkpoints from the matching connector.
                    v.insert(PointPair(centerPoint, inf));
                }
            }
            else
            {
                // Center is a shape vertex, so add all endpoint vertices.
                v.insert(PointPair(centerPoint, inf));
            }
        }
        else
        {
            // Add shape vertex.
            v.insert(PointPair(centerPoint, inf));
        }
    }
    std::set<unsigned int> onBorderIDs;

    // Add edges to T that intersect the initial ray.
    SweepEdgeList e;
    VertSet::const_iterator vbegin = v.begin();
    VertSet::const_iterator vend = v.end();
    const Point xaxis(DBL_MAX, centerInf->point.y);
    for (VertSet::const_iterator t = vbegin; t != vend; ++t)
    {
        VertInf *k = t->vInf;

        COLA_ASSERT(centerInf != k);

        VertInf *kPrev = k->shPrev;
        VertInf *kNext = k->shNext;
        if (kPrev && (kPrev != centerInf) && 
                (vecDir(centerInf->point, xaxis, kPrev->point) == AHEAD))
        {
            if (segmentIntersect(centerInf->point, xaxis, kPrev->point, 
                        k->point))
            {
                EdgePair intPair = EdgePair(*t, kPrev);
                e.push_back(intPair);
            }
            if (pointOnLine(kPrev->point, k->point, centerInf->point))
            {
                // Record that centerPoint is on an obstacle line.
                onBorderIDs.insert(k->id.objID);
            }
        }
        else if (kNext && (kNext != centerInf) && 
                (vecDir(centerInf->point, xaxis, kNext->point) == AHEAD))
        {
            if (segmentIntersect(centerInf->point, xaxis, kNext->point, 
                        k->point))
            {
                EdgePair intPair = EdgePair(*t, kNext);
                e.push_back(intPair);
            }
            if (pointOnLine(kNext->point, k->point, centerInf->point))
            {
                // Record that centerPoint is on an obstacle line.
                onBorderIDs.insert(k->id.objID);
            }
        }
    }
    for (SweepEdgeList::iterator c = e.begin(); c != e.end(); ++c)
    {
        (*c).setNegativeAngle();
    }


    // Start the actual sweep.
    db_printf("SWEEP: "); centerID.db_print(); db_printf("\n");

    isBoundingShape isBounding(ss);
    for (VertSet::const_iterator t = vbegin; t != vend; ++t)
    {
        VertInf *currInf = (*t).vInf;
        VertID& currID = currInf->id;
        Point&  currPt = currInf->point;

        const double& currDist = (*t).distance;

        EdgeInf *edge = EdgeInf::existingEdge(centerInf, currInf);
        if (edge == nullptr)
        {
            edge = new EdgeInf(centerInf, currInf);
        }

        for (SweepEdgeList::iterator c = e.begin(); c != e.end(); ++c)
        {
            (*c).setCurrAngle(*t);
        }
        e.sort();

        // Check visibility.
        int blocker = 0;
        bool currVisible = sweepVisible(e, *t, onBorderIDs, &blocker);

        bool cone1 = true, cone2 = true;
        if (!(centerID.isConnPt()))
        {
            cone1 = inValidRegion(router->IgnoreRegions,
                    centerInf->shPrev->point, centerPoint,
                    centerInf->shNext->point, currInf->point);
        }
        if (!(currInf->id.isConnPt()))
        {
            cone2 = inValidRegion(router->IgnoreRegions,
                    currInf->shPrev->point, currInf->point,
                    currInf->shNext->point, centerPoint);
        }

        if (!cone1 || !cone2)
        {
            if (router->InvisibilityGrph)
            {
                db_printf("\tSetting invisibility edge... \n\t\t");
                edge->addBlocker(0);
                edge->db_print();
            }
        }
        else
        {
            if (currVisible)
            {
                db_printf("\tSetting visibility edge... \n\t\t");
                edge->setDist(currDist);
                edge->db_print();
            }
            else if (router->InvisibilityGrph)
            {
                db_printf("\tSetting invisibility edge... \n\t\t");
                edge->addBlocker(blocker);
                edge->db_print();
            }
        }
        
        if (!(edge->added()) && !(router->InvisibilityGrph))
        {
            delete edge;
            edge = nullptr;
        }

        if (!(currID.isConnPt()))
        {
            // This is a shape edge

            if (currInf->shPrev != centerInf)
            {
                Point& prevPt = currInf->shPrev->point;
                int prevDir = vecDir(centerPoint, currPt, prevPt);
                EdgePair prevPair = EdgePair(*t, currInf->shPrev);

                if (prevDir == BEHIND)
                {
                    e.remove(prevPair);
                }
                else if (prevDir == AHEAD)
                {
                    e.push_front(prevPair);
                }
            }

            if (currInf->shNext != centerInf)
            {
                Point& nextPt = currInf->shNext->point;
                int nextDir = vecDir(centerPoint, currPt, nextPt);
                EdgePair nextPair = EdgePair(*t, currInf->shNext);

                if (nextDir == BEHIND)
                {
                    e.remove(nextPair);
                }
                else if (nextDir == AHEAD)
                {
                    e.push_front(nextPair);
                }
            }
        }
    }
}


}


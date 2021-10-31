/*
 * vim: ts=4 sw=4 et tw=0 wm=0
 *
 * libavoid - Fast, Incremental, Object-avoiding Line Router
 *
 * Copyright (C) 2004-2011  Monash University
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


#include <cmath>

#include "libavoid/debug.h"
#include "libavoid/graph.h"
#include "libavoid/connector.h"
#include "libavoid/geometry.h"
#include "libavoid/timer.h"
#include "libavoid/vertices.h"
#include "libavoid/router.h"
#include "libavoid/assertions.h"


using std::pair;

namespace Avoid {


EdgeInf::EdgeInf(VertInf *v1, VertInf *v2, const bool orthogonal)
    : lstPrev(nullptr),
      lstNext(nullptr),
      m_router(nullptr),
      m_blocker(0),
      m_added(false),
      m_visible(false),
      m_orthogonal(orthogonal),
      m_isHyperedgeSegment(false),
      m_disabled(false),
      m_vert1(v1),
      m_vert2(v2),
      m_dist(-1)
{
    // Not passed nullptr values.
    COLA_ASSERT(v1 && v2);

    // We are in the same instance
    COLA_ASSERT(m_vert1->_router == m_vert2->_router);
    m_router = m_vert1->_router;

    m_conns.clear();
}


EdgeInf::~EdgeInf()
{
    if (m_added)
    {
        makeInactive();
    }
}


// Gives an order value between 0 and 3 for the point c, given the last
// segment was from a to b.  Returns the following value:
//    0 : Point c is directly backwards from point b.
//    1 : Point c is a left-hand 90 degree turn.
//    2 : Point c is a right-hand 90 degree turn.
//    3 : Point c is straight ahead (collinear).
//    4 : Point c is not orthogonally positioned.
//
static inline int orthogTurnOrder(const Point& a, const Point& b, 
        const Point& c)
{
    if ( ((c.x != b.x) && (c.y != b.y)) || ((a.x != b.x) && (a.y != b.y)) )
    {
        // Not orthogonally positioned.
        return 4;
    }

    int direction = vecDir(a, b, c);

    if (direction > 0)
    {
        // Counterclockwise := left
        return 1;
    }
    else if (direction < 0)
    {
        // Clockwise := right
        return 2;
    }

    if (b.x == c.x)
    {
        if ( ((a.y < b.y) && (c.y < b.y)) || 
             ((a.y > b.y) && (c.y > b.y)) ) 
        {
            // Behind.
            return 0;
        }
    }
    else
    {
        if ( ((a.x < b.x) && (c.x < b.x)) || 
             ((a.x > b.x) && (c.x > b.x)) ) 
        {
            // Behind.
            return 0;
        }
    }

    // Ahead.
    return 3;
}


// Returns a less than operation for a set exploration order for orthogonal
// searching.  Forward, then left, then right.  Or if there is no previous 
// point, then the order is north, east, south, then west.
// Note: This method assumes the two Edges that share a common point.
bool EdgeInf::rotationLessThan(const VertInf *lastV, const EdgeInf *rhs) const
{
    if ((m_vert1 == rhs->m_vert1) && (m_vert2 == rhs->m_vert2))
    {
        // Effectively the same visibility edge, so they are equal.
        return false;
    }
    VertInf *lhsV = nullptr, *rhsV = nullptr, *commonV = nullptr;
    
    // Determine common Point and the comparison point on the left- and
    // the right-hand-side.
    if (m_vert1 == rhs->m_vert1)
    {
        commonV = m_vert1;
        lhsV = m_vert2;
        rhsV = rhs->m_vert2;
    }
    else if (m_vert1 == rhs->m_vert2)
    {
        commonV = m_vert1;
        lhsV = m_vert2;
        rhsV = rhs->m_vert1;
    }
    else if (m_vert2 == rhs->m_vert1)
    {
        commonV = m_vert2;
        lhsV = m_vert1;
        rhsV = rhs->m_vert2;
    }
    else if (m_vert2 == rhs->m_vert2)
    {
        commonV = m_vert2;
        lhsV = m_vert1;
        rhsV = rhs->m_vert1;
    }

    const Point& lhsPt = lhsV->point;
    const Point& rhsPt = rhsV->point;
    const Point& commonPt = commonV->point;
    
    // If no lastPt, use one directly to the left;
    Point lastPt = (lastV) ? lastV->point : Point(commonPt.x - 10,  commonPt.y);

    int lhsVal = orthogTurnOrder(lastPt, commonPt, lhsPt);
    int rhsVal = orthogTurnOrder(lastPt, commonPt, rhsPt);

    return lhsVal < rhsVal;
}


void EdgeInf::makeActive(void)
{
    COLA_ASSERT(m_added == false);

    if (m_orthogonal)
    {
        COLA_ASSERT(m_visible);
        m_router->visOrthogGraph.addEdge(this);
        m_pos1 = m_vert1->orthogVisList.insert(m_vert1->orthogVisList.begin(), this);
        m_vert1->orthogVisListSize++;
        m_pos2 = m_vert2->orthogVisList.insert(m_vert2->orthogVisList.begin(), this);
        m_vert2->orthogVisListSize++;
    }
    else
    {
        if (m_visible)
        {
            m_router->visGraph.addEdge(this);
            m_pos1 = m_vert1->visList.insert(m_vert1->visList.begin(), this);
            m_vert1->visListSize++;
            m_pos2 = m_vert2->visList.insert(m_vert2->visList.begin(), this);
            m_vert2->visListSize++;
        }
        else // if (invisible)
        {
            m_router->invisGraph.addEdge(this);
            m_pos1 = m_vert1->invisList.insert(m_vert1->invisList.begin(), this);
            m_vert1->invisListSize++;
            m_pos2 = m_vert2->invisList.insert(m_vert2->invisList.begin(), this);
            m_vert2->invisListSize++;
        }
    }
    m_added = true;
}


void EdgeInf::makeInactive(void)
{
    COLA_ASSERT(m_added == true);

    if (m_orthogonal)
    {
        COLA_ASSERT(m_visible);
        m_router->visOrthogGraph.removeEdge(this);
        m_vert1->orthogVisList.erase(m_pos1);
        m_vert1->orthogVisListSize--;
        m_vert2->orthogVisList.erase(m_pos2);
        m_vert2->orthogVisListSize--;
    }
    else
    {
        if (m_visible)
        {
            m_router->visGraph.removeEdge(this);
            m_vert1->visList.erase(m_pos1);
            m_vert1->visListSize--;
            m_vert2->visList.erase(m_pos2);
            m_vert2->visListSize--;
        }
        else // if (invisible)
        {
            m_router->invisGraph.removeEdge(this);
            m_vert1->invisList.erase(m_pos1);
            m_vert1->invisListSize--;
            m_vert2->invisList.erase(m_pos2);
            m_vert2->invisListSize--;
        }
    }
    m_blocker = 0;
    m_conns.clear();
    m_added = false;
}


void EdgeInf::setDist(double dist)
{
    //COLA_ASSERT(dist != 0);

    if (m_added && !m_visible)
    {
        makeInactive();
        COLA_ASSERT(!m_added);
    }
    if (!m_added)
    {
        m_visible = true;
        makeActive();
    }
    m_dist = dist;
    m_blocker = 0;
}


void EdgeInf::setMtstDist(const double joinCost)
{
    m_mtst_dist = joinCost;
}

double EdgeInf::mtstDist(void) const
{
    return m_mtst_dist;
}

bool EdgeInf::isHyperedgeSegment(void) const
{
    return m_isHyperedgeSegment;
}

bool EdgeInf::isDisabled(void) const
{
    return m_disabled;
}

void EdgeInf::setDisabled(const bool disabled)
{
    m_disabled = disabled;
}

void EdgeInf::setHyperedgeSegment(const bool hyperedge)
{
    m_isHyperedgeSegment = hyperedge;
}

bool EdgeInf::added(void)
{
    return m_added;
}

int EdgeInf::blocker(void) const
{
    return m_blocker;
}

void EdgeInf::alertConns(void)
{
    FlagList::iterator finish = m_conns.end();
    for (FlagList::iterator i = m_conns.begin(); i != finish; ++i)
    {
        *(*i) = true;
    }
    m_conns.clear();
}


void EdgeInf::addConn(bool *flag)
{
    m_conns.push_back(flag);
}


void EdgeInf::addCycleBlocker(void)
{
    // Needs to be in invisibility graph.
    addBlocker(-1);
}


void EdgeInf::addBlocker(int b)
{
    COLA_ASSERT(m_router->InvisibilityGrph);

    if (m_added && m_visible)
    {
        makeInactive();
        COLA_ASSERT(!m_added);
    }
    if (!m_added)
    {
        m_visible = false;
        makeActive();
    }
    m_dist = 0;
    m_blocker = b;
}


pair<VertID, VertID> EdgeInf::ids(void) const
{
    return std::make_pair(m_vert1->id, m_vert2->id);
}


pair<Point, Point> EdgeInf::points(void) const
{
    return std::make_pair(m_vert1->point, m_vert2->point);
}


void EdgeInf::db_print(void)
{
    db_printf("Edge(");
    m_vert1->id.db_print();
    db_printf(",");
    m_vert2->id.db_print();
    db_printf(")\n");
}


void EdgeInf::checkVis(void)
{
    if (m_added && !m_visible)
    {
        db_printf("\tChecking visibility for existing invisibility edge..."
                "\n\t\t");
        db_print();
    }
    else if (m_added && m_visible)
    {
        db_printf("\tChecking visibility for existing visibility edge..."
                "\n\t\t");
        db_print();
    }

    int blocker = 0;
    bool cone1 = true;
    bool cone2 = true;

    VertInf *i = m_vert1;
    VertInf *j = m_vert2;
    const VertID& iID = i->id;
    const VertID& jID = j->id;
    const Point& iPoint = i->point;
    const Point& jPoint = j->point;

    m_router->st_checked_edges++;

    if (!(iID.isConnPt()))
    {
        cone1 = inValidRegion(m_router->IgnoreRegions, i->shPrev->point,
                iPoint, i->shNext->point, jPoint);
    }
    else if (m_router->IgnoreRegions == false)
    {
        // If Ignoring regions then this case is already caught by 
        // the invalid regions, so only check it when not ignoring
        // regions.
        ShapeSet& ss = m_router->contains[iID];

        if (!(jID.isConnPt()) && (ss.find(jID.objID) != ss.end()))
        {
            db_printf("1: Edge of bounding shape\n");
            // Don't even check this edge, it should be zero,
            // since a point in a shape can't see it's corners
            cone1 = false;
        }
    }

    if (cone1)
    {
        // If outside the first cone, don't even bother checking.
        if (!(jID.isConnPt()))
        {
            cone2 = inValidRegion(m_router->IgnoreRegions, j->shPrev->point,
                    jPoint, j->shNext->point, iPoint);
        }
        else if (m_router->IgnoreRegions == false)
        {
            // If Ignoring regions then this case is already caught by 
            // the invalid regions, so only check it when not ignoring
            // regions.
            ShapeSet& ss = m_router->contains[jID];

            if (!(iID.isConnPt()) && (ss.find(iID.objID) != ss.end()))
            {
                db_printf("2: Edge of bounding shape\n");
                // Don't even check this edge, it should be zero,
                // since a point in a shape can't see it's corners
                cone2 = false;
            }
        }
    }

    if (cone1 && cone2 && ((blocker = firstBlocker()) == 0))
    {

        // if i and j see each other, add edge
        db_printf("\tSetting visibility edge... \n\t\t");
        db_print();

        double d = euclideanDist(iPoint, jPoint);

        setDist(d);

    }
    else if (m_router->InvisibilityGrph)
    {
#if 0
        db_printf("%d, %d, %d\n", cone1, cone2, blocker);
        db_printf("\t(%d, %d)--(%d, %d)\n", (int) iInfo.point.x,
                (int) iInfo.point.y, (int) jInfo.point.x,
                (int) jInfo.point.y);
#endif

        // if i and j can't see each other, add blank edge
        db_printf("\tSetting invisibility edge... \n\t\t");
        db_print();
        addBlocker(blocker);
    }
}


int EdgeInf::firstBlocker(void)
{
    ShapeSet ss = ShapeSet();

    Point& pti = m_vert1->point;
    Point& ptj = m_vert2->point;
    VertID& iID = m_vert1->id;
    VertID& jID = m_vert2->id;

    ContainsMap &contains = m_router->contains;
    if (iID.isConnPt())
    {
        ss.insert(contains[iID].begin(), contains[iID].end());
    }
    if (jID.isConnPt())
    {
        ss.insert(contains[jID].begin(), contains[jID].end());
    }

    VertInf *last = m_router->vertices.end();
    unsigned int lastId = 0;
    bool seenIntersectionAtEndpoint = false;
    for (VertInf *k = m_router->vertices.shapesBegin(); k != last; )
    {
        VertID kID = k->id;
        if (k->id == dummyOrthogID)
        {
            // Don't include orthogonal dummy vertices.
            k = k->lstNext;
            continue;
        }
        if (kID.objID != lastId)
        {
            if ((ss.find(kID.objID) != ss.end()))
            {
                unsigned int shapeID = kID.objID;
                db_printf("Endpoint is inside shape %u so ignore shape "
                        "edges.\n", kID.objID);
                // One of the endpoints is inside this shape so ignore it.
                while ((k != last) && (k->id.objID == shapeID))
                {
                    // And skip the other vertices from this shape.
                    k = k->lstNext;
                }
                continue;
            }
            seenIntersectionAtEndpoint = false;
            lastId = kID.objID;
        }
        Point& kPoint = k->point;
        Point& kPrevPoint = k->shPrev->point;
        if (segmentShapeIntersect(pti, ptj, kPrevPoint, kPoint, 
                    seenIntersectionAtEndpoint))
        {
            ss.clear();
            return kID.objID;
        }
        k = k->lstNext;
    }
    ss.clear();
    return 0;
}


bool EdgeInf::isBetween(VertInf *i, VertInf *j)
{
    if ( ((i == m_vert1) && (j == m_vert2)) ||
         ((i == m_vert2) && (j == m_vert1)) )
    {
        return true;
    }
    return false;
}


    // Returns true if this edge is a vertical or horizontal line segment.
bool EdgeInf::isOrthogonal(void) const
{
    return ((m_vert1->point.x == m_vert2->point.x) || 
            (m_vert1->point.y == m_vert2->point.y));
}


bool EdgeInf::isDummyConnection(void) const
{
    // This is a dummy edge from a shape centre to 
    // a set of its ShapeConnectionPins.
    return ((m_vert1->id.isConnectionPin() && m_vert2->id.isConnPt()) ||
            (m_vert2->id.isConnectionPin() && m_vert1->id.isConnPt()));
}


VertInf *EdgeInf::otherVert(const VertInf *vert) const
{
    COLA_ASSERT((vert == m_vert1) || (vert == m_vert2));

    return (vert == m_vert1) ? m_vert2 : m_vert1;
}


EdgeInf *EdgeInf::checkEdgeVisibility(VertInf *i, VertInf *j, bool knownNew)
{
    // This is for polyline routing, so check we're not 
    // considering orthogonal vertices.
    COLA_ASSERT(i->id != dummyOrthogID);
    COLA_ASSERT(j->id != dummyOrthogID);
    
    Router *router = i->_router;
    EdgeInf *edge = nullptr;

    if (knownNew)
    {
        COLA_ASSERT(existingEdge(i, j) == nullptr);
        edge = new EdgeInf(i, j);
    }
    else
    {
        edge = existingEdge(i, j);
        if (edge == nullptr)
        {
            edge = new EdgeInf(i, j);
        }
    }
    edge->checkVis();
    if (!(edge->m_added) && !(router->InvisibilityGrph))
    {
        delete edge;
        edge = nullptr;
    }

    return edge;
}


    // XXX: This function is inefficient, and shouldn't even really be
    //      required.
EdgeInf *EdgeInf::existingEdge(VertInf *i, VertInf *j)
{
    VertInf *selected = nullptr;

    // Look through poly-line visibility edges.
    selected = (i->visListSize <= j->visListSize) ? i : j;
    EdgeInfList& visList = selected->visList;
    EdgeInfList::const_iterator finish = visList.end();
    for (EdgeInfList::const_iterator edge = visList.begin(); edge != finish;
            ++edge)
    {
        if ((*edge)->isBetween(i, j))
        {
            return (*edge);
        }
    }

    // Look through orthogonal visibility edges.
    selected = (i->orthogVisListSize <= j->orthogVisListSize) ? i : j;
    EdgeInfList& orthogVisList = selected->orthogVisList;
    finish = orthogVisList.end();
    for (EdgeInfList::const_iterator edge = orthogVisList.begin(); 
            edge != finish; ++edge)
    {
        if ((*edge)->isBetween(i, j))
        {
            return (*edge);
        }
    }

    // Look through poly-line invisibility edges.
    selected = (i->invisListSize <= j->invisListSize) ? i : j;
    EdgeInfList& invisList = selected->invisList;
    finish = invisList.end();
    for (EdgeInfList::const_iterator edge = invisList.begin(); edge != finish;
            ++edge)
    {
        if ((*edge)->isBetween(i, j))
        {
            return (*edge);
        }
    }

    return nullptr;
}


//===========================================================================


EdgeList::EdgeList(bool orthogonal)
    : m_orthogonal(orthogonal),
      m_first_edge(nullptr),
      m_last_edge(nullptr),
      m_count(0)
{
}


EdgeList::~EdgeList()
{
    clear();
}


void EdgeList::clear(void)
{
    while (m_first_edge)
    {
        // The Edge destructor results in EdgeList:::removeEdge() being called
        // for this edge and m_first_edge being updated to the subsequent edge
        // in the EdgeList.
        delete m_first_edge;
    }
    COLA_ASSERT(m_count == 0);
    m_last_edge = nullptr;
}


int EdgeList::size(void) const
{
    return m_count;
}


void EdgeList::addEdge(EdgeInf *edge)
{
    // Dummy connections for ShapeConnectionPins won't be orthogonal, 
    // even in the orthogonal visibility graph.
    COLA_UNUSED(m_orthogonal);
    COLA_ASSERT(!m_orthogonal || edge->isOrthogonal() ||
            edge->isDummyConnection());
    
    if (m_first_edge == nullptr)
    {
        COLA_ASSERT(m_last_edge == nullptr);

        m_last_edge = edge;
        m_first_edge = edge;

        edge->lstPrev = nullptr;
        edge->lstNext = nullptr;
    }
    else
    {
        COLA_ASSERT(m_last_edge != nullptr);

        m_last_edge->lstNext = edge;
        edge->lstPrev = m_last_edge;

        m_last_edge = edge;

        edge->lstNext = nullptr;
    }
    m_count++;
}


void EdgeList::removeEdge(EdgeInf *edge)
{
    if (edge->lstPrev)
    {
        edge->lstPrev->lstNext = edge->lstNext;
    }
    if (edge->lstNext)
    {
        edge->lstNext->lstPrev = edge->lstPrev;
    }
    if (edge == m_last_edge)
    {
        m_last_edge = edge->lstPrev;
        if (edge == m_first_edge)
        {
            m_first_edge = nullptr;
        }
    }
    else if (edge == m_first_edge)
    {
        m_first_edge = edge->lstNext;
    }


    edge->lstPrev = nullptr;
    edge->lstNext = nullptr;

    m_count--;
}


EdgeInf *EdgeList::begin(void)
{
    return m_first_edge;
}


EdgeInf *EdgeList::end(void)
{
    return nullptr;
}


}



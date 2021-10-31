/*
 * vim: ts=4 sw=4 et tw=0 wm=0
 *
 * libavoid - Fast, Incremental, Object-avoiding Line Router
 *
 * Copyright (C) 2004-2009  Monash University
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


#include <iostream>
#include <cstdlib>

#include "libavoid/vertices.h"
#include "libavoid/geometry.h"
#include "libavoid/graph.h"  // For alertConns
#include "libavoid/debug.h"
#include "libavoid/router.h"
#include "libavoid/assertions.h"
#include "libavoid/connend.h"

using std::ostream;


namespace Avoid {


VertID::VertID()
{
}


VertID::VertID(unsigned int id, unsigned short n, VertIDProps p)
    : objID(id),
      vn(n),
      props(p)
{
}


VertID::VertID(const VertID& other)
    : objID(other.objID),
      vn(other.vn),
      props(other.props)
{
}


VertID& VertID::operator= (const VertID& rhs)
{
    // Gracefully handle self assignment
    //if (this == &rhs) return *this;

    objID = rhs.objID;
    vn = rhs.vn;
    props = rhs.props;

    return *this;
}


bool VertID::operator==(const VertID& rhs) const
{
    if ((objID != rhs.objID) || (vn != rhs.vn))
    {
        return false;
    }
    return true;
}


bool VertID::operator!=(const VertID& rhs) const
{
    if ((objID != rhs.objID) || (vn != rhs.vn))
    {
        return true;
    }
    return false;
}


bool VertID::operator<(const VertID& rhs) const
{
    if ((objID < rhs.objID) ||
            ((objID == rhs.objID) && (vn < rhs.vn)))
    {
        return true;
    }
    return false;
}


VertID VertID::operator+(const int& rhs) const
{
    return VertID(objID, vn + rhs, props);
}


VertID VertID::operator-(const int& rhs) const
{
    return VertID(objID, vn - rhs, props);
}


VertID& VertID::operator++(int)
{
    vn += 1;
    return *this;
}


void VertID::print(FILE *file) const
{
    fprintf(file, "[%u,%d, p=%u]", objID, vn, (unsigned int) props);
}

void VertID::db_print(void) const
{
    db_printf("[%u,%d, p=%u]", objID, vn, (unsigned int) props);
}


const unsigned short VertID::src = 1;
const unsigned short VertID::tar = 2;

// Property flags:
const unsigned short VertID::PROP_ConnPoint      = 1;
const unsigned short VertID::PROP_OrthShapeEdge  = 2;
const unsigned short VertID::PROP_ConnectionPin  = 4;
const unsigned short VertID::PROP_ConnCheckpoint = 8;
const unsigned short VertID::PROP_DummyPinHelper = 16;


ostream& operator<<(ostream& os, const VertID& vID)
{
    return os << '[' << vID.objID << ',' << vID.vn << ']';
}



VertInf::VertInf(Router *router, const VertID& vid, const Point& vpoint, 
        const bool addToRouter)
    : _router(router),
      id(vid),
      point(vpoint),
      lstPrev(nullptr),
      lstNext(nullptr),
      shPrev(nullptr),
      shNext(nullptr),
      visListSize(0),
      orthogVisListSize(0),
      invisListSize(0),
      pathNext(nullptr),
      m_orthogonalPartner(nullptr),
      m_treeRoot(nullptr),
      visDirections(ConnDirNone),
      orthogVisPropFlags(0)
{
    point.id = vid.objID;
    point.vn = vid.vn;

    if (addToRouter)
    {
        _router->vertices.addVertex(this);
    }
}


VertInf::~VertInf()
{
    COLA_ASSERT(orphaned());
}


EdgeInf *VertInf::hasNeighbour(VertInf *target, bool orthogonal) const
{
    const EdgeInfList& visEdgeList = (orthogonal) ? orthogVisList : visList;
    EdgeInfList::const_iterator finish = visEdgeList.end();
    for (EdgeInfList::const_iterator edge = visEdgeList.begin(); edge != finish; ++edge)
    {
        if ((*edge)->otherVert(this) == target)
        {
            return *edge;
        }
    }
    return nullptr;
}

void VertInf::Reset(const VertID& vid, const Point& vpoint)
{
    id = vid;
    point = vpoint;
    point.id = id.objID;
    point.vn = id.vn;
}


void VertInf::Reset(const Point& vpoint)
{
    point = vpoint;
    point.id = id.objID;
    point.vn = id.vn;
}


// Returns true if this vertex is not involved in any (in)visibility graphs.
bool VertInf::orphaned(void)
{
    return (visList.empty() && invisList.empty() && orthogVisList.empty());
}


void VertInf::removeFromGraph(const bool isConnVert)
{
    if (isConnVert)
    {
        COLA_ASSERT(id.isConnPt());
    }

    // For each vertex.
    EdgeInfList::const_iterator finish = visList.end();
    EdgeInfList::const_iterator edge;
    while ((edge = visList.begin()) != finish)
    {
        // Remove each visibility edge
        (*edge)->alertConns();
        delete (*edge);
    }

    finish = orthogVisList.end();
    while ((edge = orthogVisList.begin()) != finish)
    {
        // Remove each orthogonal visibility edge.
        (*edge)->alertConns();
        delete (*edge);
    }

    finish = invisList.end();
    while ((edge = invisList.begin()) != finish)
    {
        // Remove each invisibility edge
        delete (*edge);
    }
}


void VertInf::orphan(void)
{
    // For each vertex.
    EdgeInfList::const_iterator finish = visList.end();
    EdgeInfList::const_iterator edge;
    while ((edge = visList.begin()) != finish)
    {
        // Remove each visibility edge
        (*edge)->makeInactive();
    }

    finish = orthogVisList.end();
    while ((edge = orthogVisList.begin()) != finish)
    {
        // Remove each orthogonal visibility edge.
        (*edge)->makeInactive();
    }

    finish = invisList.end();
    while ((edge = invisList.begin()) != finish)
    {
        // Remove each invisibility edge
        (*edge)->makeInactive();
    }
}

// Returns the direction of this vertex relative to the other specified vertex.
//
ConnDirFlags VertInf::directionFrom(const VertInf *other) const
{
    double epsilon = 0.000001;
    Point thisPoint = point;
    Point otherPoint = other->point;
    Point diff = thisPoint - otherPoint;

    ConnDirFlags directions = ConnDirNone;
    if (diff.y > epsilon)
    {
        directions |= ConnDirUp;
    }
    if (diff.y < -epsilon)
    {
        directions |= ConnDirDown;
    }
    if (diff.x > epsilon)
    {
        directions |= ConnDirRight;
    }
    if (diff.x < -epsilon)
    {
        directions |= ConnDirLeft;
    }
    return directions;
}

// Given a set of directions, mark visibility edges in all other directions
// as being invalid so they get ignored during the search.
//
void VertInf::setVisibleDirections(const ConnDirFlags directions)
{
    for (EdgeInfList::const_iterator edge = visList.begin();
            edge != visList.end(); ++edge)
    {
        if (directions == ConnDirAll)
        {
            (*edge)->setDisabled(false);
        }
        else
        {
            VertInf *otherVert = (*edge)->otherVert(this);
            ConnDirFlags direction = otherVert->directionFrom(this);
            bool visible = (direction & directions);
            (*edge)->setDisabled(!visible);
        }
    }

    for (EdgeInfList::const_iterator edge = orthogVisList.begin();
            edge != orthogVisList.end(); ++edge)
    {
        if (directions == ConnDirAll)
        {
            (*edge)->setDisabled(false);
        }
        else
        {
            VertInf *otherVert = (*edge)->otherVert(this);
            ConnDirFlags direction = otherVert->directionFrom(this);
            bool visible = (direction & directions);
            (*edge)->setDisabled(!visible);
        }
    }
}

// Number of points in path from end back to start, or zero if no path exists.
//
unsigned int VertInf::pathLeadsBackTo(const VertInf *start) const
{
    unsigned int pathlen = 1;
    for (const VertInf *i = this; i != start; i = i->pathNext)
    {
        if ((pathlen > 1) && (i == this))
        {
            // We have a circular path, so path not found.
            return 0;
        }

        pathlen++;
        if (i == nullptr)
        {
            // Path not found.
            return 0;
        }

        // Check we don't have an apparent infinite connector path.
        COLA_ASSERT(pathlen < 20000);
    }
    return pathlen;
}

VertInf **VertInf::makeTreeRootPointer(VertInf *root)
{
    m_treeRoot = (VertInf **) malloc(sizeof(VertInf *));
    *m_treeRoot = root;
    return m_treeRoot;
}

VertInf *VertInf::treeRoot(void) const
{
    return (m_treeRoot) ? *m_treeRoot : nullptr;
}

VertInf **VertInf::treeRootPointer(void) const
{
    return m_treeRoot;
}

void VertInf::clearTreeRootPointer(void)
{
    m_treeRoot = nullptr;
}

void VertInf::setTreeRootPointer(VertInf **pointer)
{
    m_treeRoot = pointer;
}
        
void VertInf::setSPTFRoot(VertInf *root)
{
    // Use the m_treeRoot instance var, but as just a normal VertInf pointer.
    m_treeRoot = (VertInf **) root;
}


VertInf *VertInf::sptfRoot(void) const
{
    // Use the m_treeRoot instance var, but as just a normal VertInf pointer.
    return (VertInf *) m_treeRoot;
}


bool directVis(VertInf *src, VertInf *dst)
{
    ShapeSet ss = ShapeSet();

    Point& p = src->point;
    Point& q = dst->point;

    VertID& pID = src->id;
    VertID& qID = dst->id;

    // We better be part of the same instance of libavoid.
    Router *router = src->_router;
    COLA_ASSERT(router == dst->_router);

    ContainsMap& contains = router->contains;
    if (pID.isConnPt())
    {
        ss.insert(contains[pID].begin(), contains[pID].end());
    }
    if (qID.isConnPt())
    {
        ss.insert(contains[qID].begin(), contains[qID].end());
    }

    // The "beginning" should be the first shape vertex, rather
    // than an endpoint, which are also stored in "vertices".
    VertInf *endVert = router->vertices.end();
    for (VertInf *k = router->vertices.shapesBegin(); k != endVert;
            k = k->lstNext)
    {
        if ((ss.find(k->id.objID) == ss.end()))
        {
            if (segmentIntersect(p, q, k->point, k->shNext->point))
            {
                return false;
            }
        }
    }
    return true;
}


VertInfList::VertInfList()
    : _firstShapeVert(nullptr),
      _firstConnVert(nullptr),
      _lastShapeVert(nullptr),
      _lastConnVert(nullptr),
      _shapeVertices(0),
      _connVertices(0)
{
}


#define checkVertInfListConditions() \
        do { \
            COLA_ASSERT((!_firstConnVert && (_connVertices == 0)) || \
                    ((_firstConnVert->lstPrev == nullptr) && (_connVertices > 0))); \
            COLA_ASSERT((!_firstShapeVert && (_shapeVertices == 0)) || \
                    ((_firstShapeVert->lstPrev == nullptr) && (_shapeVertices > 0))); \
            COLA_ASSERT(!_lastShapeVert || (_lastShapeVert->lstNext == nullptr)); \
            COLA_ASSERT(!_lastConnVert || (_lastConnVert->lstNext == _firstShapeVert)); \
            COLA_ASSERT((!_firstConnVert && !_lastConnVert) || \
                    (_firstConnVert &&  _lastConnVert) ); \
            COLA_ASSERT((!_firstShapeVert && !_lastShapeVert) || \
                    (_firstShapeVert &&  _lastShapeVert) ); \
            COLA_ASSERT(!_firstShapeVert || !(_firstShapeVert->id.isConnPt())); \
            COLA_ASSERT(!_lastShapeVert || !(_lastShapeVert->id.isConnPt())); \
            COLA_ASSERT(!_firstConnVert || _firstConnVert->id.isConnPt()); \
            COLA_ASSERT(!_lastConnVert || _lastConnVert->id.isConnPt()); \
        } while(0)


void VertInfList::addVertex(VertInf *vert)
{
    checkVertInfListConditions();
    COLA_ASSERT(vert->lstPrev == nullptr);
    COLA_ASSERT(vert->lstNext == nullptr);

    if (vert->id.isConnPt())
    {
        // A Connector vertex
        if (_firstConnVert)
        {
            // Join with previous front
            vert->lstNext = _firstConnVert;
            _firstConnVert->lstPrev = vert;

            // Make front
            _firstConnVert = vert;
        }
        else
        {
            // Make front and back
            _firstConnVert = vert;
            _lastConnVert = vert;

            // Link to front of shapes list
            vert->lstNext = _firstShapeVert;
        }
        _connVertices++;
    }
    else // if (vert->id.shape > 0)
    {
        // A Shape vertex
        if (_lastShapeVert)
        {
            // Join with previous back
            vert->lstPrev = _lastShapeVert;
            _lastShapeVert->lstNext = vert;

            // Make back
            _lastShapeVert = vert;
        }
        else
        {
            // Make first and last
            _firstShapeVert = vert;
            _lastShapeVert = vert;

            // Join with conns list
            if (_lastConnVert)
            {
                COLA_ASSERT(_lastConnVert->lstNext == nullptr);

                _lastConnVert->lstNext = vert;
            }
        }
        _shapeVertices++;
    }
    checkVertInfListConditions();
}


// Removes a vertex from the list and returns a pointer to the vertex
// following the removed one.
VertInf *VertInfList::removeVertex(VertInf *vert)
{
    if (vert == nullptr)
    {
        return nullptr;
    }
    // Conditions for correct data structure
    checkVertInfListConditions();
    
    VertInf *following = vert->lstNext;

    if (vert->id.isConnPt())
    {
        // A Connector vertex
        if (vert == _firstConnVert)
        {

            if (vert == _lastConnVert)
            {
                _firstConnVert = nullptr;
                _lastConnVert = nullptr;
            }
            else
            {
                // Set new first
                _firstConnVert = _firstConnVert->lstNext;

                if (_firstConnVert)
                {
                    // Set previous
                    _firstConnVert->lstPrev = nullptr;
                }
            }
        }
        else if (vert == _lastConnVert)
        {
            // Set new last
            _lastConnVert = _lastConnVert->lstPrev;

            // Make last point to shapes list
            _lastConnVert->lstNext = _firstShapeVert;
        }
        else
        {
            vert->lstNext->lstPrev = vert->lstPrev;
            vert->lstPrev->lstNext = vert->lstNext;
        }
        _connVertices--;
    }
    else // if (vert->id.shape > 0)
    {
        // A Shape vertex
        if (vert == _lastShapeVert)
        {
            // Set new last
            _lastShapeVert = _lastShapeVert->lstPrev;

            if (vert == _firstShapeVert)
            {
                _firstShapeVert = nullptr;
                if (_lastConnVert)
                {
                    _lastConnVert->lstNext = nullptr;
                }
            }

            if (_lastShapeVert)
            {
                _lastShapeVert->lstNext = nullptr;
            }
        }
        else if (vert == _firstShapeVert)
        {
            // Set new first
            _firstShapeVert = _firstShapeVert->lstNext;

            // Correct the last conn vertex
            if (_lastConnVert)
            {
                _lastConnVert->lstNext = _firstShapeVert;
            }

            if (_firstShapeVert)
            {
                _firstShapeVert->lstPrev = nullptr;
            }
        }
        else
        {
            vert->lstNext->lstPrev = vert->lstPrev;
            vert->lstPrev->lstNext = vert->lstNext;
        }
        _shapeVertices--;
    }
    vert->lstPrev = nullptr;
    vert->lstNext = nullptr;

    checkVertInfListConditions();

    return following;
}


VertInf *VertInfList::getVertexByID(const VertID& id)
{
    VertID searchID = id;
    if (searchID.vn == kUnassignedVertexNumber)
    {
        unsigned int topbit = ((unsigned int) 1) << 31;
        if (searchID.objID & topbit)
        {
            searchID.objID = searchID.objID & ~topbit;
            searchID.vn = VertID::src;
        }
        else
        {
            searchID.vn = VertID::tar;
        }
    }
    VertInf *last = end();
    for (VertInf *curr = connsBegin(); curr != last; curr = curr->lstNext)
    {
        if (curr->id == searchID)
        {
            return curr;
        }
    }
    return nullptr;
}


VertInf *VertInfList::getVertexByPos(const Point& p)
{
    VertInf *last = end();
    for (VertInf *curr = shapesBegin(); curr != last; curr = curr->lstNext)
    {
        if (curr->point == p)
        {
            return curr;
        }
    }
    return nullptr;
}


VertInf *VertInfList::shapesBegin(void)
{
    return _firstShapeVert;
}


VertInf *VertInfList::connsBegin(void)
{
    if (_firstConnVert)
    {
        return _firstConnVert;
    }
    // No connector vertices
    return _firstShapeVert;
}


VertInf *VertInfList::end(void)
{
    return nullptr;
}


unsigned int VertInfList::connsSize(void) const
{
    return _connVertices;
}


unsigned int VertInfList::shapesSize(void) const
{
    return _shapeVertices;
}


}



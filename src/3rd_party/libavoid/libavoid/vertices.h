/*
 * vim: ts=4 sw=4 et tw=0 wm=0
 *
 * libavoid - Fast, Incremental, Object-avoiding Line Router
 *
 * Copyright (C) 2004-2013  Monash University
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


#ifndef AVOID_VERTICES_H
#define AVOID_VERTICES_H

#include <list>
#include <set>
#include <map>
#include <iostream>
#include <cstdio>
#include <utility>

#include "libavoid/geomtypes.h"

namespace Avoid {

class EdgeInf;
class VertInf;
class Router;

typedef std::list<EdgeInf *> EdgeInfList;
typedef std::pair<VertInf *, VertInf *> VertexPair;

typedef unsigned int ConnDirFlags;
typedef unsigned short VertIDProps;


class VertID
{
    public:
        unsigned int objID;
        unsigned short vn;
        // Properties:
        VertIDProps props;

        static const unsigned short src;
        static const unsigned short tar;
        
        static const VertIDProps PROP_ConnPoint;
        static const VertIDProps PROP_OrthShapeEdge;
        static const VertIDProps PROP_ConnectionPin;
        static const VertIDProps PROP_ConnCheckpoint;
        static const VertIDProps PROP_DummyPinHelper;

        VertID();
        VertID(unsigned int id, unsigned short n, VertIDProps p = 0);
        VertID(const VertID& other);
        VertID& operator= (const VertID& rhs);
        bool operator==(const VertID& rhs) const;
        bool operator!=(const VertID& rhs) const;
        bool operator<(const VertID& rhs) const;
        VertID operator+(const int& rhs) const;
        VertID operator-(const int& rhs) const;
        VertID& operator++(int);
        void print(FILE *file = stdout) const;
        void db_print(void) const;
        friend std::ostream& operator<<(std::ostream& os, const VertID& vID);
        
        // Property tests:
        inline bool isOrthShapeEdge(void) const
        {
            return (props & PROP_OrthShapeEdge) ? true : false;
        }
        inline bool isConnPt(void) const
        {
            return (props & PROP_ConnPoint) ? true : false;
        }
        inline bool isConnectionPin(void) const
        {
            return (props & PROP_ConnectionPin) ? true : false;
        }
        inline bool isConnCheckpoint(void) const
        {
            return (props & PROP_ConnCheckpoint) ? true : false;
        }
        inline bool isDummyPinHelper(void) const
        {
            return (props & PROP_DummyPinHelper) ? true : false;
        }
};


// An ID given to all dummy vertices inserted to allow creation of the
// orthogonal visibility graph since the vertices in the orthogonal graph 
// mostly do not correspond to shape corners or connector endpoints.
//
static const VertID dummyOrthogID(0, 0);
static const VertID dummyOrthogShapeID(0, 0, VertID::PROP_OrthShapeEdge);

class ANode;

class VertInf
{
    public:
        VertInf(Router *router, const VertID& vid, const Point& vpoint,
                const bool addToRouter = true);
        ~VertInf();
        void Reset(const VertID& vid, const Point& vpoint);
        void Reset(const Point& vpoint);
        void removeFromGraph(const bool isConnVert = true);
        bool orphaned(void);

        unsigned int pathLeadsBackTo(const VertInf *start) const;
        void setVisibleDirections(const ConnDirFlags directions);
        ConnDirFlags directionFrom(const VertInf *other) const;
        // Checks if this vertex has the target as a visibility neighbour.
        EdgeInf *hasNeighbour(VertInf *target, bool orthogonal) const;
        void orphan(void);

        VertInf **makeTreeRootPointer(VertInf *root);
        VertInf *treeRoot(void) const;
        VertInf **treeRootPointer(void) const;
        void setTreeRootPointer(VertInf **pointer);
        void clearTreeRootPointer(void);

        void setSPTFRoot(VertInf *root);
        VertInf *sptfRoot(void) const;

        Router *_router;
        VertID id;
        Point  point;
        VertInf *lstPrev;
        VertInf *lstNext;
        VertInf *shPrev;
        VertInf *shNext;
        EdgeInfList visList;
        unsigned int visListSize;
        EdgeInfList orthogVisList;
        unsigned int orthogVisListSize;
        EdgeInfList invisList;
        unsigned int invisListSize;
        VertInf *pathNext;

        // The tree root and distance value used when computing MTSTs.
        // XXX: Maybe these should be allocated as a separate struct
        //      and referenced via a pointer.  This would be slower due
        //      to memory allocation, but would save 2 x 8 = 24 bytes per 
        //      VertInf on 64-bit machines.
        VertInf *m_orthogonalPartner;
        VertInf **m_treeRoot;
        double sptfDist;

        ConnDirFlags visDirections;
        std::list<ANode *> aStarDoneNodes;
        std::list<ANode *> aStarPendingNodes;
        // Flags for orthogonal visibility properties, i.e., whether the 
        // line points to a shape edge, connection point or an obstacle.
        unsigned int orthogVisPropFlags;
};


// Orthogonal visibility property flags
static const unsigned int XL_EDGE = 1;
static const unsigned int XL_CONN = 2;
static const unsigned int XH_EDGE = 4;
static const unsigned int XH_CONN = 8;
static const unsigned int YL_EDGE = 16;
static const unsigned int YL_CONN = 32;
static const unsigned int YH_EDGE = 64;
static const unsigned int YH_CONN = 128;


bool directVis(VertInf *src, VertInf *dst);


// A linked list of all the vertices in the router instance.  All the 
// connector endpoints are listed first, then all the shape vertices.
// Dummy vertices inserted for orthogonal routing are classed as shape
// vertices but have VertID(0, 0).
//
class VertInfList
{
    public:
        VertInfList();
        void addVertex(VertInf *vert);
        VertInf *removeVertex(VertInf *vert);
        VertInf *getVertexByID(const VertID& id);
        VertInf *getVertexByPos(const Point& p);
        VertInf *shapesBegin(void);
        VertInf *connsBegin(void);
        VertInf *end(void);
        unsigned int connsSize(void) const;
        unsigned int shapesSize(void) const;
    private:
        VertInf *_firstShapeVert;
        VertInf *_firstConnVert;
        VertInf *_lastShapeVert;
        VertInf *_lastConnVert;
        unsigned int _shapeVertices;
        unsigned int _connVertices;
};


typedef std::set<unsigned int> ShapeSet;
typedef std::map<VertID, ShapeSet> ContainsMap;


}


#endif



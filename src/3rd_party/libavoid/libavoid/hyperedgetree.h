/*
 * vim: ts=4 sw=4 et tw=0 wm=0
 *
 * libavoid - Fast, Incremental, Object-avoiding Line Router
 *
 * Copyright (C) 2011-2014  Monash University
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

#ifndef AVOID_HYPEREDGETREE_H
#define AVOID_HYPEREDGETREE_H

#include <cstdio>
#include <list>
#include <map>
#include <set>
#include <utility>

#include "libavoid/geomtypes.h"

//#define MAJOR_HYPEREDGE_IMPROVEMENT_DEBUG

namespace Avoid {

// These classes are not intended for public use.
// They are used to represent a hyperedge as a tree that certain
// transformations can be easily performed on to improve the routing
// of the hyperedge.

class JunctionRef;
class ConnRef;
class HyperedgeShiftSegment;
class VertInf;
class Router;

struct HyperedgeTreeEdge;
struct HyperedgeTreeNode;

typedef std::map<JunctionRef *, HyperedgeTreeNode *>
        JunctionHyperedgeTreeNodeMap;
typedef std::set<JunctionRef *> JunctionSet;
typedef std::list<JunctionRef *> JunctionRefList;
typedef std::list<ConnRef *> ConnRefList;

class CmpNodesInDim;

typedef std::set<HyperedgeTreeNode *, CmpNodesInDim> OrderedHENodeSet;

struct HyperedgeTreeNode
{
    HyperedgeTreeNode();
    ~HyperedgeTreeNode();

    void deleteEdgesExcept(HyperedgeTreeEdge *ignored);
    bool removeOtherJunctionsFrom(HyperedgeTreeEdge *ignored, 
            JunctionSet &treeRoots);
    void outputEdgesExcept(FILE *fp, HyperedgeTreeEdge *ignored);
    void disconnectEdge(HyperedgeTreeEdge *edge);
    void spliceEdgesFrom(HyperedgeTreeNode *oldNode);
    void writeEdgesToConns(HyperedgeTreeEdge *ignored, size_t pass);
    void addConns(HyperedgeTreeEdge *ignored, Router *router, 
            ConnRefList& oldConns, ConnRef *conn);
    void updateConnEnds(HyperedgeTreeEdge *ignored, bool forward,
            ConnRefList& changedConns);
    void listJunctionsAndConnectors(HyperedgeTreeEdge *ignored,
            JunctionRefList& junctions, ConnRefList& connectors);
    bool isImmovable(void) const;
    void validateHyperedge(const HyperedgeTreeEdge *ignored, 
            const size_t dist) const;

    std::list<HyperedgeTreeEdge *> edges;
    JunctionRef *junction;
    Point point;
    OrderedHENodeSet *shiftSegmentNodeSet;
    VertInf *finalVertex;
    bool isConnectorSource;
    bool isPinDummyEndpoint;
    bool visited;
};

struct HyperedgeTreeEdge
{
    HyperedgeTreeEdge(HyperedgeTreeNode *node1, HyperedgeTreeNode *node2,
            ConnRef *conn);

    HyperedgeTreeNode *followFrom(HyperedgeTreeNode *from) const;
    bool zeroLength(void) const;
    void splitFromNodeAtPoint(HyperedgeTreeNode *source, const Point& point);
    bool hasOrientation(const size_t dimension) const;
    void outputNodesExcept(FILE *file, HyperedgeTreeNode *ignored);
    void deleteNodesExcept(HyperedgeTreeNode *ignored);
    bool removeOtherJunctionsFrom(HyperedgeTreeNode *ignored, 
            JunctionSet &treeRoots);
    void writeEdgesToConns(HyperedgeTreeNode *ignored, size_t pass);
    void addConns(HyperedgeTreeNode *ignored, Router *router,
            ConnRefList& oldConns);
    void updateConnEnds(HyperedgeTreeNode *ignored, bool forward,
            ConnRefList& changedConns);
    void disconnectEdge(void);
    void replaceNode(HyperedgeTreeNode *oldNode,
            HyperedgeTreeNode *newNode);
    void listJunctionsAndConnectors(HyperedgeTreeNode *ignored,
            JunctionRefList& junctions, ConnRefList& connectors);
    void validateHyperedge(const HyperedgeTreeNode *ignored, 
            const size_t dist) const;

    std::pair<HyperedgeTreeNode *, HyperedgeTreeNode *> ends;
    ConnRef *conn;
    bool hasFixedRoute;
};


typedef std::map<VertInf *, HyperedgeTreeNode *> VertexNodeMap;


class CmpNodesInDim
{
    public:
        CmpNodesInDim(const size_t dim);
        bool operator()(const HyperedgeTreeNode *lhs,
                const HyperedgeTreeNode *rhs) const;
    private:
        const size_t m_dimension;
};

}

#endif

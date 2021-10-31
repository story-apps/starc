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
 * Author(s):   Michael Wybrow
*/


#ifndef AVOID_GRAPH_H
#define AVOID_GRAPH_H


#include <cassert>
#include <list>
#include <utility>
#include "libavoid/vertices.h"

namespace Avoid {


class ConnRef;
class Router;


typedef std::list<int> ShapeList;
typedef std::list<bool *> FlagList;


class EdgeInf
{
    public:
        EdgeInf(VertInf *v1, VertInf *v2, const bool orthogonal = false);
        ~EdgeInf();
        inline double getDist(void)
        {
            return m_dist;
        }
        void setDist(double dist);
        void alertConns(void);
        void addConn(bool *flag);
        void addCycleBlocker(void);
        void addBlocker(int b);
        bool added(void);
        bool isOrthogonal(void) const;
        bool isDummyConnection(void) const;
        bool isDisabled(void) const;
        void setDisabled(const bool disabled);
        bool rotationLessThan(const VertInf* last, const EdgeInf *rhs) const;
        std::pair<VertID, VertID> ids(void) const;
        std::pair<Point, Point> points(void) const;
        void db_print(void);
        void checkVis(void);
        VertInf *otherVert(const VertInf *vert) const;
        static EdgeInf *checkEdgeVisibility(VertInf *i, VertInf *j,
                bool knownNew = false);
        static EdgeInf *existingEdge(VertInf *i, VertInf *j);
        int blocker(void) const;
        
        bool isHyperedgeSegment(void) const;
        void setHyperedgeSegment(const bool hyperedge);
        double mtstDist(void) const;
        void setMtstDist(const double joinCost);

        EdgeInf *lstPrev;
        EdgeInf *lstNext;
    private:
        friend class MinimumTerminalSpanningTree;
        friend class VertInf;

        void makeActive(void);
        void makeInactive(void);
        int firstBlocker(void);
        bool isBetween(VertInf *i, VertInf *j);

        Router *m_router;
        int m_blocker;
        bool m_added;
        bool m_visible;
        bool m_orthogonal;
        bool m_isHyperedgeSegment;
        bool m_disabled;
        VertInf *m_vert1;
        VertInf *m_vert2;
        EdgeInfList::iterator m_pos1;
        EdgeInfList::iterator m_pos2;
        FlagList  m_conns;
        double  m_dist;
        double  m_mtst_dist;
};


class EdgeList
{
    public:
        friend class EdgeInf;
        EdgeList(bool orthogonal = false);
        ~EdgeList();
        void clear(void);
        EdgeInf *begin(void);
        EdgeInf *end(void);
        int size(void) const;
    private:
        void addEdge(EdgeInf *edge);
        void removeEdge(EdgeInf *edge);

        bool m_orthogonal;
        EdgeInf *m_first_edge;
        EdgeInf *m_last_edge;
        unsigned int m_count;
};


}


#endif



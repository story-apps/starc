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
 * Author(s):   Michael Wybrow
*/

//! @file    obstacle.h
//! @brief   Contains the interface for the Obstacle class, 
//!          the superclass for ShapeRef and JunctionRef.


#ifndef AVOID_OBSTACLE_H
#define AVOID_OBSTACLE_H

#include <list>
#include <set>

#include <cstdio>

#include "libavoid/geometry.h"
#include "libavoid/connectionpin.h"


namespace Avoid {

class VertInf;
class Router;
class Obstacle;
class ConnEnd;
class ConnRef;
typedef std::list<Obstacle *> ObstacleList;
typedef std::list<ConnRef *> ConnRefList;


// @brief   The Obstacle class represents an obstacle that must be 
//          routed around.  Superclass of ShapeRef and JunctionRef.
//
class Obstacle
{
    public:
        //! @brief  Obstacle reference constructor.
        //!
        //! Creates an obstacle object reference.  It is expected that you
        //! would not instantiate an Obstacle directly, but would instead
        //! use either ShapeRef() or JunctionRef().
        //!
        //! @param[in]  router  The router scene to place the shape into.
        //! @param[in]  poly    A Polygon representing the boundary of the 
        //!                     shape.
        //! @param[in]  id      A unique positive integer ID for the shape.  
        Obstacle(Router *router, Polygon poly, const unsigned int id = 0);

// To prevent C++ objects from being destroyed in garbage collected languages
// when the libraries are called from SWIG, we hide the declarations of the
// destructors and prevent generation of default destructors.
#ifndef SWIG
        //! @brief  Obstacle reference destructor.
        //!
        virtual ~Obstacle();
#endif
        
        //! @brief   Returns the ID of this obstacle.
        //! @returns The ID of the obstacle.
        unsigned int id(void) const;
        //! @brief   Returns a reference to the polygon boundary of this
        //!          obstacle.
        //! @returns A reference to the polygon boundary of the obstacle.
        const Polygon& polygon(void) const;
        //! @brief   Returns a pointer to the router scene this obstacle
        //!          is in.
        //! @returns A pointer to the router scene for this obstacle.
        Router *router(void) const;
        //! @brief   Returns the position of this obstacle.
        //! @returns A point representing the position of this obstacle.
        virtual Point position(void) const = 0;
        
        void setNewPoly(const Polygon& poly);
        VertInf *firstVert(void);
        VertInf *lastVert(void);
        Box routingBox(void) const;
        Polygon routingPolygon(void) const;
        ConnRefList attachedConnectors(void) const;

    private:
        friend class Router;
        friend class ConnEnd;
        friend class ShapeConnectionPin;
        friend class HyperedgeRerouter;
        friend class HyperedgeImprover;
        friend class MinimumTerminalSpanningTree;

        // Defined in visibility.cpp:
        void computeVisibilityNaive(void);
        void computeVisibilitySweep(void);
       
        virtual void outputCode(FILE *fp) const = 0;
        void makeActive(void);
        void makeInactive(void);
        bool isActive(void) const;
        void updatePinPolyLineVisibility(void);
        void removeFromGraph(void);
        Point shapeCentre(void);

        VertInf *getPointVertex(const Point& point);

        void addFollowingConnEnd(ConnEnd *connEnd);
        void removeFollowingConnEnd(ConnEnd *connEnd);
        size_t addConnectionPin(ShapeConnectionPin *pin);
        void removeConnectionPin(ShapeConnectionPin *pin);
        void assignPinVisibilityTo(const unsigned int pinClassId, 
                VertInf *dummyConnectionVert);
        std::vector<Point> possiblePinPoints(unsigned int pinClassId) const;

    protected:
        Router *m_router;
        unsigned int m_id;
        Polygon m_polygon;
        bool m_active;
        ObstacleList::iterator m_router_obstacles_pos;
        VertInf *m_first_vert;
        VertInf *m_last_vert;
        std::set<ConnEnd *> m_following_conns;
        ShapeConnectionPinSet m_connection_pins;
};


}


#endif



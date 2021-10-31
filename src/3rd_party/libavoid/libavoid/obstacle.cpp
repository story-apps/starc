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


#include "libavoid/obstacle.h"
#include "libavoid/router.h"
#include "libavoid/connectionpin.h"
#include "libavoid/debug.h"

namespace Avoid {


Obstacle::Obstacle(Router *router, Polygon ply, const unsigned int id)
    : m_router(router),
      m_polygon(ply),
      m_active(false),
      m_first_vert(nullptr),
      m_last_vert(nullptr)
{
    COLA_ASSERT(m_router != nullptr);
    m_id = m_router->assignId(id);

    VertID i = VertID(m_id, 0);

    Polygon routingPoly = routingPolygon();
    const bool addToRouterNow = false;
    VertInf *last = nullptr;
    VertInf *node = nullptr;
    for (size_t pt_i = 0; pt_i < routingPoly.size(); ++pt_i)
    {
        node = new VertInf(m_router, i, routingPoly.ps[pt_i], addToRouterNow);

        if (!m_first_vert)
        {
            m_first_vert = node;
        }
        else
        {
            node->shPrev = last;
            last->shNext = node;
            //node->lstPrev = last;
            //last->lstNext = node;
        }
        
        last = node;
        i++;
    }
    m_last_vert = node;
    
    m_last_vert->shNext = m_first_vert;
    m_first_vert->shPrev = m_last_vert;
}


Obstacle::~Obstacle()
{
    COLA_ASSERT(m_active == false);
    COLA_ASSERT(m_first_vert != nullptr);
    
    VertInf *it = m_first_vert;
    do
    {
        VertInf *tmp = it;
        it = it->shNext;

        delete tmp;
    }
    while (it != m_first_vert);
    m_first_vert = m_last_vert = nullptr;

    // Free and clear any connection pins.
    while (!m_connection_pins.empty())
    {
        delete *(m_connection_pins.begin());
    }
}


void Obstacle::setNewPoly(const Polygon& poly)
{
    COLA_ASSERT(m_first_vert != nullptr);
    COLA_ASSERT(m_polygon.size() == poly.size());
    
    m_polygon = poly;
    Polygon routingPoly = routingPolygon();

    VertInf *curr = m_first_vert;
    for (size_t pt_i = 0; pt_i < routingPoly.size(); ++pt_i)
    {
        COLA_ASSERT(curr->visListSize == 0);
        COLA_ASSERT(curr->invisListSize == 0);

        // Reset with the new polygon point.
        curr->Reset(routingPoly.ps[pt_i]);
        curr->pathNext = nullptr;
        
        curr = curr->shNext;
    }
    COLA_ASSERT(curr == m_first_vert);
        
    // It may be that the polygon for the obstacle has been updated after
    // creating the shape.  These events may have been combined for a single
    // transaction, so update pin positions.
    for (ShapeConnectionPinSet::iterator curr =
            m_connection_pins.begin(); curr != m_connection_pins.end(); ++curr)
    {
        ShapeConnectionPin *pin = *curr;
        pin->updatePosition(m_polygon);
    }
}


void Obstacle::makeActive(void)
{
    COLA_ASSERT(!m_active);
    
    // Add to shapeRefs list.
    m_router_obstacles_pos = m_router->m_obstacles.insert(
            m_router->m_obstacles.begin(), this);

    // Add points to vertex list.
    VertInf *it = m_first_vert;
    do
    {
        VertInf *tmp = it;
        it = it->shNext;

        m_router->vertices.addVertex(tmp);
    }
    while (it != m_first_vert);
   
    m_active = true;
}


void Obstacle::makeInactive(void)
{
    COLA_ASSERT(m_active);
    
    // Remove from shapeRefs list.
    m_router->m_obstacles.erase(m_router_obstacles_pos);

    // Remove points from vertex list.
    VertInf *it = m_first_vert;
    do
    {
        VertInf *tmp = it;
        it = it->shNext;

        m_router->vertices.removeVertex(tmp);
    }
    while (it != m_first_vert);
    
    m_active = false;
    
    // Turn attached ConnEnds into manual points.
    bool deletedShape = true;
    while (!m_following_conns.empty())
    {
        ConnEnd *connEnd = *(m_following_conns.begin());
        connEnd->disconnect(deletedShape);
    }
}


void Obstacle::updatePinPolyLineVisibility(void)
{
    for (ShapeConnectionPinSet::iterator curr = 
            m_connection_pins.begin(); 
            curr != m_connection_pins.end(); ++curr)
    {
        (*curr)->updateVisibility();
    }
}


std::vector<Point> Obstacle::possiblePinPoints(unsigned int pinClassId) const
{
    std::vector<Point> points;
    for (ShapeConnectionPinSet::const_iterator curr = 
            m_connection_pins.begin(); 
            curr != m_connection_pins.end(); ++curr)
    {
        ShapeConnectionPin *currPin = *curr;
        if ((currPin->m_class_id == pinClassId) && 
                (!currPin->m_exclusive || currPin->m_connend_users.empty()))
        {
            points.push_back(currPin->m_vertex->point);
        }
    }
    return points;
}


size_t Obstacle::addConnectionPin(ShapeConnectionPin *pin)
{
    m_connection_pins.insert(pin);
    m_router->modifyConnectionPin(pin);

    return m_connection_pins.size();
}

void Obstacle::removeConnectionPin(ShapeConnectionPin *pin)
{
    m_connection_pins.erase(pin);
    m_router->modifyConnectionPin(pin);
}


bool Obstacle::isActive(void) const
{
    return m_active;
}


VertInf *Obstacle::firstVert(void)
{
    return m_first_vert;
}


VertInf *Obstacle::lastVert(void)
{
    return m_last_vert;
}


unsigned int Obstacle::id(void) const
{
    return m_id;
}


const Polygon& Obstacle::polygon(void) const
{
    return m_polygon;
}


Router *Obstacle::router(void) const
{
    return m_router;
}


Box Obstacle::routingBox(void) const
{
    COLA_ASSERT(!m_polygon.empty());
    COLA_ASSERT(m_router);

    double bufferSpace = m_router->routingParameter(shapeBufferDistance);
    return m_polygon.offsetBoundingBox(bufferSpace);
}


Polygon Obstacle::routingPolygon(void) const
{
    COLA_ASSERT(!m_polygon.empty());
    COLA_ASSERT(m_router);

    double bufferSpace = m_router->routingParameter(shapeBufferDistance);
    return m_polygon.offsetPolygon(bufferSpace);
}


Point Obstacle::shapeCentre(void)
{
    Box bb = routingBox();

    Point centre;
    centre.x = bb.min.x + (0.5 * (bb.max.x - bb.min.x));
    centre.y = bb.min.y + (0.5 * (bb.max.y - bb.min.y));
    return centre;
}


void Obstacle::removeFromGraph(void)
{
    bool isConnPt = false;
    for (VertInf *iter = firstVert(); iter != lastVert()->lstNext; )
    {
        VertInf *tmp = iter;
        iter = iter->lstNext;
 
        tmp->removeFromGraph(isConnPt);
    }
}


VertInf *Obstacle::getPointVertex(const Point& point)
{
    VertInf *curr = m_first_vert;
    do
    {
        if (curr->point == point)
        {
            return curr;
        }
        curr = curr->shNext;
    }
    while (curr != m_first_vert);

    return nullptr;
}


void Obstacle::addFollowingConnEnd(ConnEnd *connEnd)
{
    m_following_conns.insert(connEnd);
}


void Obstacle::removeFollowingConnEnd(ConnEnd *connEnd)
{
    m_following_conns.erase(connEnd);
}


ConnRefList Obstacle::attachedConnectors(void) const
{
    ConnRefList attachedConns;
    for (std::set<ConnEnd *>::const_iterator curr = m_following_conns.begin();
            curr != m_following_conns.end(); ++curr)
    {
        ConnEnd *connEnd = *curr;
        COLA_ASSERT(connEnd->m_conn_ref != nullptr);
        attachedConns.push_back(connEnd->m_conn_ref);
    }
    return attachedConns;
}

}



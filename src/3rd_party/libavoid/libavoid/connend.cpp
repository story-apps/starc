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

#include <cstring>
#include <cmath>
#include <cstdlib>
#include <cfloat>
#include <algorithm>

#include "libavoid/router.h"
#include "libavoid/connend.h"
#include "libavoid/connector.h"
#include "libavoid/connectionpin.h"
#include "libavoid/junction.h"
#include "libavoid/shape.h"
#include "libavoid/debug.h"
#include "libavoid/graph.h"
#include "libavoid/visibility.h"

namespace Avoid {

    
ConnEnd::ConnEnd()
    : m_type(ConnEndEmpty),
      m_point(Point(0,0)),
      m_directions(ConnDirAll),
      m_connection_pin_class_id(CONNECTIONPIN_UNSET),
      m_anchor_obj(nullptr),
      m_conn_ref(nullptr),
      m_active_pin(nullptr)
{
}

ConnEnd::ConnEnd(const Point& point) 
    : m_type(ConnEndPoint),
      m_point(point),
      m_directions(ConnDirAll),
      m_connection_pin_class_id(CONNECTIONPIN_UNSET),
      m_anchor_obj(nullptr),
      m_conn_ref(nullptr),
      m_active_pin(nullptr)
{
}


ConnEnd::ConnEnd(const Point& point, const ConnDirFlags visDirs) 
    : m_type(ConnEndPoint),
      m_point(point),
      m_directions(visDirs),
      m_connection_pin_class_id(CONNECTIONPIN_UNSET),
      m_anchor_obj(nullptr),
      m_conn_ref(nullptr),
      m_active_pin(nullptr)
{
}


ConnEnd::ConnEnd(ShapeRef *shapeRef, const unsigned int connectionPinClassID)
    : m_type(ConnEndShapePin),
      m_point(Point(0,0)),
      m_directions(ConnDirAll),
      m_connection_pin_class_id(connectionPinClassID),
      m_anchor_obj(shapeRef),
      m_conn_ref(nullptr),
      m_active_pin(nullptr)
{
    COLA_ASSERT(m_anchor_obj != nullptr);
    COLA_ASSERT(m_connection_pin_class_id > 0);

    m_point = m_anchor_obj->position();
    COLA_ASSERT(m_connection_pin_class_id != CONNECTIONPIN_UNSET);
}


ConnEnd::ConnEnd(JunctionRef *junctionRef)
    : m_type(ConnEndJunction),
      m_directions(ConnDirAll),
      m_connection_pin_class_id(CONNECTIONPIN_CENTRE),
      m_anchor_obj(junctionRef),
      m_conn_ref(nullptr),
      m_active_pin(nullptr)
{
    COLA_ASSERT(m_anchor_obj != nullptr);
    m_point = m_anchor_obj->position();
}


ConnEnd::~ConnEnd()
{
}


ConnEndType ConnEnd::type(void) const
{
    return m_type;
}


const Point ConnEnd::position(void) const
{
    if (m_active_pin)  // Attached to a pin!
    {
        return m_active_pin->position();
    }
    else if (m_anchor_obj)
    {
        return m_anchor_obj->position();
    }
    else
    {
        return m_point;
    }
}


ConnDirFlags ConnEnd::directions(void) const
{
    if (m_active_pin)  // Attached to a pin!
    {
        return m_active_pin->directions();
    }
    else
    {
        return m_directions;
    }
}


ShapeRef *ConnEnd::shape(void) const
{
    return dynamic_cast<ShapeRef *> (m_anchor_obj);
}


JunctionRef *ConnEnd::junction(void) const
{
    return dynamic_cast<JunctionRef *> (m_anchor_obj);
}


unsigned int ConnEnd::pinClassId(void) const
{
    return m_connection_pin_class_id;
}



bool ConnEnd::isPinConnection(void) const
{
    return (m_type == ConnEndShapePin) || (m_type == ConnEndJunction);
}

unsigned int ConnEnd::endpointType(void) const
{
    COLA_ASSERT(m_conn_ref != nullptr);
    return (m_conn_ref->m_dst_connend == this) ? VertID::tar : VertID::src;
}


// Marks this ConnEnd as using a particular ShapeConnectionPin.
void ConnEnd::usePin(ShapeConnectionPin *pin)
{
    COLA_ASSERT(m_active_pin == nullptr);
    
    m_active_pin = pin;
    if (m_active_pin)
    {
        m_active_pin->m_connend_users.insert(this);
    }
}


// Marks this ConnEnd as using a particular ShapeConnectionPin's vertex.
void ConnEnd::usePinVertex(VertInf *pinVert)
{
    COLA_ASSERT(m_active_pin == nullptr);
    
    for (ShapeConnectionPinSet::iterator curr = 
            m_anchor_obj->m_connection_pins.begin(); 
            curr != m_anchor_obj->m_connection_pins.end(); ++curr)
    {
        ShapeConnectionPin *currPin = *curr;
        if (currPin->m_vertex == pinVert)
        {
            usePin(currPin);
            break;
        }
    }
}


// Returns a vector of the possible Points for this ConnEnd
std::vector<Point> ConnEnd::possiblePinPoints(void) const
{
    std::vector<Point> points;
    if (!m_anchor_obj || (m_connection_pin_class_id == CONNECTIONPIN_UNSET))
    {
        return points;
    }

    return m_anchor_obj->possiblePinPoints(m_connection_pin_class_id);
}

// Marks this ConnEnd as no longer using the active ShapeConnectionPin
void ConnEnd::freeActivePin(void)
{
    if (m_active_pin)
    {
        m_active_pin->m_connend_users.erase(this);
    }
    m_active_pin = nullptr;
}


// Creates the connection between a connector and a shape/junction.
void ConnEnd::connect(ConnRef *conn)
{
    COLA_ASSERT(isPinConnection());
    COLA_ASSERT(m_anchor_obj);
    COLA_ASSERT(m_conn_ref == nullptr);

    m_anchor_obj->addFollowingConnEnd(this);
    m_conn_ref = conn;
}


// Removes the connection between a connector and a shape/junction.
void ConnEnd::disconnect(const bool shapeDeleted)
{
    if (m_conn_ref == nullptr)
    {
        // Not connected.
        return;
    }

    m_point = position();
    m_anchor_obj->removeFollowingConnEnd(this);
    m_conn_ref = nullptr;

    if (shapeDeleted)
    {
        // Turn this into a manual ConnEnd.
        m_point = position();
        m_anchor_obj = nullptr;
        m_type = ConnEndPoint;
        m_connection_pin_class_id = CONNECTIONPIN_UNSET;
    }
}


// Assign visibility to a dummy vertex representing all the possible pins
// for this pinClassId.
void ConnEnd::assignPinVisibilityTo(VertInf *dummyConnectionVert, 
        VertInf *targetVert)
{
    unsigned int validPinCount = 0;

    COLA_ASSERT(m_anchor_obj);
    COLA_ASSERT(m_connection_pin_class_id != CONNECTIONPIN_UNSET);
 
    Router *router = m_anchor_obj->router();
    for (ShapeConnectionPinSet::iterator curr = 
            m_anchor_obj->m_connection_pins.begin(); 
            curr != m_anchor_obj->m_connection_pins.end(); ++curr)
    {
        ShapeConnectionPin *currPin = *curr;
        if ((currPin->m_class_id == m_connection_pin_class_id) && 
                (!currPin->m_exclusive || currPin->m_connend_users.empty()))
        {
            double routingCost = currPin->m_connection_cost;
            Point adjTargetPt = targetVert->point - currPin->m_vertex->point;
            double angle = rotationalAngle(adjTargetPt);
            bool inVisibilityRange = false;

            if (angle <= 45 || angle >= 315)
            {
                if (currPin->directions() & ConnDirRight)
                {
                    inVisibilityRange = true;
                }
            }
            if (angle >= 45 && angle <= 135)
            {
                if (currPin->directions() & ConnDirDown)
                {
                    inVisibilityRange = true;
                }
            }
            if (angle >= 135 && angle <= 225)
            {
                if (currPin->directions() & ConnDirLeft)
                {
                    inVisibilityRange = true;
                }
            }
            if (angle >= 225 && angle <= 315)
            {
                if (currPin->directions() & ConnDirUp)
                {
                    inVisibilityRange = true;
                }
            }
            if (!inVisibilityRange)
            {
                routingCost += router->routingParameter(portDirectionPenalty);
            }

            if (router->m_allows_orthogonal_routing)
            {
                // This has same ID and is either unconnected or not 
                // exclusive, so give it visibility.
                EdgeInf *edge = new EdgeInf(dummyConnectionVert,
                        currPin->m_vertex, true);
                // XXX Can't use a zero cost due to assumptions 
                //     elsewhere in code.
                edge->setDist(manhattanDist(dummyConnectionVert->point,
                            currPin->m_vertex->point) + 
                        std::max(0.001, routingCost));
            }

            if (router->m_allows_polyline_routing)
            {
                // This has same ID and is either unconnected or not 
                // exclusive, so give it visibility.
                EdgeInf *edge = new EdgeInf(dummyConnectionVert,
                        currPin->m_vertex, false);
                // XXX Can't use a zero cost due to assumptions 
                //     elsewhere in code.
                edge->setDist(euclideanDist(dummyConnectionVert->point,
                            currPin->m_vertex->point) + 
                        std::max(0.001, routingCost));
            }

            // Increment the number of valid pins for this ConnEnd connection.
            validPinCount++;
        }
    }

    if (validPinCount == 0)
    {
        // There should be at least one pin, otherwise we will have 
        // problems finding connector routes.
        err_printf("Warning: In ConnEnd::assignPinVisibilityTo():\n"
                   "         ConnEnd for connector %d can't connect to shape %d\n"
                   "         since it has no pins with class id of %u.\n", 
                   (int) m_conn_ref->id(), (int) m_anchor_obj->id(), 
                   m_connection_pin_class_id);
    }
}


std::pair<bool, VertInf *> ConnEnd::getHyperedgeVertex(Router *router) const
{
    bool addedVertex = false;
    VertInf *vertex = nullptr;

    if (m_anchor_obj)
    {
        for (ShapeConnectionPinSet::iterator curr =
                m_anchor_obj->m_connection_pins.begin();
                curr != m_anchor_obj->m_connection_pins.end(); ++curr)
        {
            ShapeConnectionPin *currPin = *curr;
            if ((currPin->m_class_id == m_connection_pin_class_id) &&
                    (!currPin->m_exclusive || currPin->m_connend_users.empty()))
            {
                vertex = currPin->m_vertex;
            }
        }
        COLA_ASSERT(vertex != nullptr);
    }
    else
    {
        VertID id(0, kUnassignedVertexNumber,
                VertID::PROP_ConnPoint);
        vertex = new VertInf(router, id, m_point);
        vertex->visDirections = m_directions;
        addedVertex = true;

        if (router->m_allows_polyline_routing)
        {
            vertexVisibility(vertex, nullptr, true, true);
        }
    }

    return std::make_pair(addedVertex, vertex);
}


void ConnEnd::outputCode(FILE *fp, const char *srcDst) const
{
    if (junction())
    {
        fprintf(fp, "    %sPt = ConnEnd(junctionRef%u);\n", srcDst,
                m_anchor_obj->id());
    }
    else if (shape())
    {
        fprintf(fp, "    %sPt = ConnEnd(shapeRef%u, %u);\n", srcDst,
                m_anchor_obj->id(), m_connection_pin_class_id);
    }
    else
    {
        fprintf(fp, "    %sPt = ConnEnd(Point(%" PREC "g, %" PREC "g), (ConnDirFlags) %u);\n",
                srcDst, m_point.x, m_point.y, m_directions);
    }
}


//============================================================================

}



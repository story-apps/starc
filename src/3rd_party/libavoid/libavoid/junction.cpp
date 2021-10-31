/*
 * vim: ts=4 sw=4 et tw=0 wm=0
 *
 * libavoid - Fast, Incremental, Object-avoiding Line Router
 *
 * Copyright (C) 2010-2011  Monash University
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

#include <cstdlib>
#include <algorithm>

#include "libavoid/junction.h"
#include "libavoid/router.h"
#include "libavoid/connectionpin.h"
#include "libavoid/debug.h"


namespace Avoid {


JunctionRef::JunctionRef(Router *router, Point position, const unsigned int id)
    : Obstacle(router, makeRectangle(router, position), id),
      m_position(position),
      m_recommended_position(position),
      m_position_fixed(false)
{
    // For Junctions we use a single non-exclusive pin.
    ShapeConnectionPin *pin = new Avoid::ShapeConnectionPin(this,
            Avoid::CONNECTIONPIN_CENTRE, ConnDirAll);
    pin->setExclusive(false);
    m_connection_pins.insert(pin);

    m_router->addJunction(this);
}

JunctionRef::~JunctionRef()
{
    if (m_router->m_currently_calling_destructors == false)
    {
        err_printf("ERROR: JunctionRef::~JunctionRef() shouldn't be called directly.\n");
        err_printf("       It is owned by the router.  Call Router::deleteJunction() instead.\n");
        abort();
    }
}


Rectangle JunctionRef::makeRectangle(Router *router, const Point& position)
{
    COLA_ASSERT(router);

    // A suitably small value.
    double nudgeDist = router->routingParameter(idealNudgingDistance);
    nudgeDist = std::min(1.0, nudgeDist);

    Point low = position;
    low.x -= nudgeDist;
    low.y -= nudgeDist;

    Point high = position;
    high.x += nudgeDist;
    high.y += nudgeDist;

    return Rectangle(low, high);
}


void JunctionRef::setPositionFixed(bool fixed)
{
    m_position_fixed = fixed;
    m_router->registerSettingsChange();
}


bool JunctionRef::positionFixed(void) const
{
    return m_position_fixed;
}


void JunctionRef::preferOrthogonalDimension(const size_t dim)
{
    const double smallPenalty = 1.0;
    for (ShapeConnectionPinSet::iterator curr = 
            m_connection_pins.begin(); curr != m_connection_pins.end(); ++curr)
    {
        ShapeConnectionPin *pin = *curr;
        if (dim == YDIM)
        {
            if (pin->directions() & (ConnDirLeft | ConnDirRight))
            {
                pin->setConnectionCost(smallPenalty);
            }
        }
        else if (dim == XDIM)
        {
            if (pin->directions() & (ConnDirUp | ConnDirDown))
            {
                pin->setConnectionCost(smallPenalty);
            }
        }
    }
}


Point JunctionRef::position(void) const
{
    return m_position;
}


void JunctionRef::setPosition(const Point& position)
{
    m_position = position;
    m_recommended_position = position;
    m_polygon = makeRectangle(m_router, m_position);
    setNewPoly(m_polygon);
}


Point JunctionRef::recommendedPosition(void) const
{
    return m_recommended_position;
}


void JunctionRef::setRecommendedPosition(const Point& position)
{
    m_recommended_position = position;
}


void JunctionRef::outputCode(FILE *fp) const
{
    fprintf(fp, "    JunctionRef *junctionRef%u = new JunctionRef(router, "
            "Point(%g, %g), %u);\n", id(), position().x, position().y, id());
    if (m_position_fixed)
    {
        fprintf(fp, "    junctionRef%u->setPositionFixed(true);\n", id());
    }
    
    fprintf(fp, "    /*\n");
    fprintf(fp, "    // This may be useful if junction pins are modified.\n");
    for (ShapeConnectionPinSet::const_iterator curr = 
            m_connection_pins.begin(); 
            curr != m_connection_pins.end(); ++curr)
    {
        (*curr)->outputCode(fp);
    }
    fprintf(fp, "    */\n");
    fprintf(fp, "\n");
}


void JunctionRef::moveAttachedConns(const Point& newPosition)
{
    // Update positions of attached connector ends.
    for (std::set<ConnEnd *>::iterator curr = m_following_conns.begin();
            curr != m_following_conns.end(); ++curr)
    {
        ConnEnd *connEnd = *curr;
        COLA_ASSERT(connEnd->m_conn_ref != nullptr);
        m_router->modifyConnector(connEnd->m_conn_ref, connEnd->endpointType(),
                *connEnd);
    }
    for (ShapeConnectionPinSet::iterator curr = 
            m_connection_pins.begin(); curr != m_connection_pins.end(); ++curr)
    {
        ShapeConnectionPin *pin = *curr;
        pin->updatePosition(newPosition);
    }
}


ConnRef *JunctionRef::removeJunctionAndMergeConnectors(void)
{
    if (m_following_conns.size() != 2)
    {
        return nullptr;
    }

    std::set<ConnEnd *>::iterator curr = m_following_conns.begin();
    ConnEnd *connEnd1 = *curr;
    ++curr;
    ConnEnd *connEnd2 = *curr;
    COLA_ASSERT(connEnd2->m_conn_ref != nullptr);
    COLA_ASSERT(connEnd1->m_conn_ref != nullptr);

    // The second conn will be the one we will delete.
    ConnRef *conn2 = connEnd2->m_conn_ref;
    // Determine its endpoint that is not attached to the junction.
    ConnEnd *connEnd2Other = (conn2->m_src_connend == connEnd2) ? 
            conn2->m_dst_connend : conn2->m_src_connend;
    if (connEnd2Other == nullptr)
    {
        // If it doesn't have a valid other endpoint, then ignore.
        return nullptr;
    }
    // Modify the first connector's junction endpoint to connect to the 
    // other end of the second connector.
    m_router->modifyConnector(connEnd1->m_conn_ref,
            connEnd1->endpointType(), *connEnd2Other);

    // Delete the second connector.
    m_router->deleteConnector(conn2);

    // Remove the junction from the router scene.  It should get deleted later.
    m_router->deleteJunction(this);

    // Return the first (i.e. merged) connector.
    return connEnd1->m_conn_ref;
}


//============================================================================
}



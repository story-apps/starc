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


#include <cstring>
#include <cfloat>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <queue>
#include <limits>

#include "libavoid/connector.h"
#include "libavoid/connend.h"
#include "libavoid/router.h"
#include "libavoid/visibility.h"
#include "libavoid/debug.h"
#include "libavoid/assertions.h"
#include "libavoid/junction.h"
#include "libavoid/makepath.h"
#include "libavoid/debughandler.h"


namespace Avoid {

    
ConnRef::ConnRef(Router *router, const unsigned int id)
    : m_router(router),
      m_type(router->validConnType()),
      m_reroute_flag_ptr(nullptr),
      m_needs_reroute_flag(true),
      m_false_path(false),
      m_needs_repaint(false),
      m_active(false),
      m_hate_crossings(false),
      m_has_fixed_route(false),
      m_route_dist(0),
      m_src_vert(nullptr),
      m_dst_vert(nullptr),
      m_start_vert(nullptr),
      m_callback_func(nullptr),
      m_connector(nullptr),
      m_src_connend(nullptr),
      m_dst_connend(nullptr)
{
    COLA_ASSERT(m_router != nullptr);
    m_id = m_router->assignId(id);

    // TODO: Store endpoints and details.
    m_route.clear();

    m_reroute_flag_ptr = m_router->m_conn_reroute_flags.addConn(this);
}


ConnRef::ConnRef(Router *router, const ConnEnd& src, const ConnEnd& dst,
        const unsigned int id)
    : m_router(router),
      m_type(router->validConnType()),
      m_reroute_flag_ptr(nullptr),
      m_needs_reroute_flag(true),
      m_false_path(false),
      m_needs_repaint(false),
      m_active(false),
      m_hate_crossings(false),
      m_has_fixed_route(false),
      m_route_dist(0),
      m_src_vert(nullptr),
      m_dst_vert(nullptr),
      m_callback_func(nullptr),
      m_connector(nullptr),
      m_src_connend(nullptr),
      m_dst_connend(nullptr)
{
    COLA_ASSERT(m_router != nullptr);
    m_id = m_router->assignId(id);
    m_route.clear();

    // Set endpoint values.
    setEndpoints(src, dst);

    m_reroute_flag_ptr = m_router->m_conn_reroute_flags.addConn(this);
}


ConnRef::~ConnRef()
{
    COLA_ASSERT(m_router);

    if (m_router->m_currently_calling_destructors == false)
    {
        err_printf("ERROR: ConnRef::~ConnRef() shouldn't be called directly.\n");
        err_printf("       It is owned by the router.  Call Router::deleteConnector() instead.\n");
        abort();
    }

    m_router->m_conn_reroute_flags.removeConn(this);

    m_router->removeObjectFromQueuedActions(this);

    freeRoutes();

    if (m_src_vert)
    {
        m_src_vert->removeFromGraph();
        m_router->vertices.removeVertex(m_src_vert);
        delete m_src_vert;
        m_src_vert = nullptr;
    }
    if (m_src_connend)
    {
        m_src_connend->disconnect();
        m_src_connend->freeActivePin();
        delete m_src_connend;
        m_src_connend = nullptr;
    }

    if (m_dst_vert)
    {
        m_dst_vert->removeFromGraph();
        m_router->vertices.removeVertex(m_dst_vert);
        delete m_dst_vert;
        m_dst_vert = nullptr;
    }
    if (m_dst_connend)
    {
        m_dst_connend->disconnect();
        m_dst_connend->freeActivePin();
        delete m_dst_connend;
        m_dst_connend = nullptr;
    }

    // Clear checkpoint vertices.
    for (size_t i = 0; i < m_checkpoint_vertices.size(); ++i)
    {
        m_checkpoint_vertices[i]->removeFromGraph(true);
        m_router->vertices.removeVertex(m_checkpoint_vertices[i]);
        delete m_checkpoint_vertices[i];
    }
    m_checkpoint_vertices.clear();

    if (m_active)
    {
        makeInactive();
    }
}


ConnType ConnRef::routingType(void) const
{
    return m_type;
}


void ConnRef::setRoutingType(ConnType type)
{
    type = m_router->validConnType(type);
    if (m_type != type)
    {
        m_type = type;

        makePathInvalid();

        m_router->modifyConnector(this);
    }
}


std::vector<Checkpoint> ConnRef::routingCheckpoints(void) const
{
    return m_checkpoints;
}


void ConnRef::setRoutingCheckpoints(const std::vector<Checkpoint>& checkpoints)
{
    m_checkpoints = checkpoints;
    
    // Clear previous checkpoint vertices.
    for (size_t i = 0; i < m_checkpoint_vertices.size(); ++i)
    {
        m_checkpoint_vertices[i]->removeFromGraph(true);
        m_router->vertices.removeVertex(m_checkpoint_vertices[i]);
        delete m_checkpoint_vertices[i];
    }
    m_checkpoint_vertices.clear();

    for (size_t i = 0; i < m_checkpoints.size(); ++i)
    {
        VertID ptID(m_id, 2 + i, 
                VertID::PROP_ConnPoint | VertID::PROP_ConnCheckpoint);
        VertInf *vertex = new VertInf(m_router, ptID, m_checkpoints[i].point);
        vertex->visDirections = ConnDirAll;

        m_checkpoint_vertices.push_back(vertex);
    }
    if (m_router->m_allows_polyline_routing)
    {
        for (size_t i = 0; i < m_checkpoints.size(); ++i)
        {
            vertexVisibility(m_checkpoint_vertices[i], nullptr, true, true);
        }
    }
}


void ConnRef::common_updateEndPoint(const unsigned int type, ConnEnd connEnd)
{
    const Point& point = connEnd.position();
    //db_printf("common_updateEndPoint(%d,(pid=%d,vn=%d,(%f,%f)))\n",
    //      type,point.id,point.vn,point.x,point.y);
    COLA_ASSERT((type == (unsigned int) VertID::src) ||
                (type == (unsigned int) VertID::tar));

    // The connEnd is a copy of a ConnEnd that will get disconnected,
    // so don't leave it looking like it is still connected.
    connEnd.m_conn_ref = nullptr;

    if (!m_active)
    {
        makeActive();
    }
    
    VertInf *altered = nullptr;

    VertIDProps properties = VertID::PROP_ConnPoint;
    if (connEnd.isPinConnection())
    {
        properties |= VertID::PROP_DummyPinHelper;
    }
    VertID ptID(m_id, type, properties);
    if (type == (unsigned int) VertID::src)
    {
        if (m_src_vert)
        {
            m_src_vert->Reset(ptID, point);
        }
        else
        {
            m_src_vert = new VertInf(m_router, ptID, point);
        }
        m_src_vert->visDirections = connEnd.directions();

        if (m_src_connend)
        {
            m_src_connend->disconnect();
            m_src_connend->freeActivePin();
            delete m_src_connend;
            m_src_connend = nullptr;
        }
        if (connEnd.isPinConnection())
        {
            m_src_connend = new ConnEnd(connEnd);
            m_src_connend->connect(this);
            // Don't need this to have visibility since we won't 
            // be connecting to it.
            m_src_vert->visDirections = ConnDirNone;
        }
        
        altered = m_src_vert;
    }
    else // if (type == (unsigned int) VertID::tar)
    {
        if (m_dst_vert)
        {
            m_dst_vert->Reset(ptID, point);
        }
        else
        {
            m_dst_vert = new VertInf(m_router, ptID, point);
        }
        m_dst_vert->visDirections = connEnd.directions();
        
        if (m_dst_connend)
        {
            m_dst_connend->disconnect();
            m_dst_connend->freeActivePin();
            delete m_dst_connend;
            m_dst_connend = nullptr;
        }
        if (connEnd.isPinConnection())
        {
            m_dst_connend = new ConnEnd(connEnd);
            m_dst_connend->connect(this);
            // Don't need this to have visibility since we won't 
            // be connecting to it.
            m_dst_vert->visDirections = ConnDirNone;
        }
        
        altered = m_dst_vert;
    }
    
    // XXX: Seems to be faster to just remove the edges and recreate
    bool isConn = true;
    altered->removeFromGraph(isConn);

    makePathInvalid();
    m_router->setStaticGraphInvalidated(true);
}


void ConnRef::setEndpoints(const ConnEnd& srcPoint, const ConnEnd& dstPoint)
{
    m_router->modifyConnector(this, VertID::src, srcPoint);
    m_router->modifyConnector(this, VertID::tar, dstPoint);
}


void ConnRef::setEndpoint(const unsigned int type, const ConnEnd& connEnd)
{
    m_router->modifyConnector(this, type, connEnd);
}


void ConnRef::setSourceEndpoint(const ConnEnd& srcPoint)
{
    m_router->modifyConnector(this, VertID::src, srcPoint);
}


void ConnRef::setDestEndpoint(const ConnEnd& dstPoint)
{
    m_router->modifyConnector(this, VertID::tar, dstPoint);
}


// Given the start or end vertex of a connector, returns the ConnEnd that 
// can be used to reproduce that endpoint.  This is used for hyperedge routing.
//
bool ConnRef::getConnEndForEndpointVertex(VertInf *vertex, 
        ConnEnd& connEnd) const 
{
    if (vertex == nullptr)
    {
        err_printf("Warning: In ConnRef::getConnEndForEndpointVertex():\n"
                   "         ConnEnd for connector %d is uninitialised.  It may have been\n"
                   "         set but Router::processTrancaction has not yet been called.\n",
                   (int) id());
        return false;
    }

    if (vertex == m_src_vert)
    {
        if (m_src_connend)
        {
            connEnd = *m_src_connend;
        }
        else
        {
            connEnd = ConnEnd(Point(m_src_vert->point.x, m_src_vert->point.y),
                    m_src_vert->visDirections);
        }
        return true;
    }
    else if (vertex == m_dst_vert)
    {
        if (m_dst_connend)
        {
            connEnd = *m_dst_connend;
        }
        else
        {
            connEnd = ConnEnd(Point(m_dst_vert->point.x, m_dst_vert->point.y),
                    m_dst_vert->visDirections);
        }
        return true;
    }
    return false;
}


void ConnRef::updateEndPoint(const unsigned int type, const ConnEnd& connEnd)
{
    common_updateEndPoint(type, connEnd);

    if (m_has_fixed_route)
    {
        // Don't need to continue and compute visibility if route is fixed.
        return;
    }

    if (m_router->m_allows_polyline_routing)
    {
        bool knownNew = true;
        bool genContains = true;
        if (type == (unsigned int) VertID::src)
        {
            bool dummySrc = m_src_connend && m_src_connend->isPinConnection();
            if (!dummySrc)
            {
                // Only generate visibility if not attached to a pin.
                vertexVisibility(m_src_vert, m_dst_vert, knownNew, genContains);
            }
        }
        else
        {
            bool dummyDst = m_dst_connend && m_dst_connend->isPinConnection();
            if (!dummyDst)
            {
                // Only generate visibility if not attached to a pin.
                vertexVisibility(m_dst_vert, m_src_vert, knownNew, genContains);
            }
        }
    }
}


void ConnRef::outputCode(FILE *fp) const
{
    fprintf(fp, "    // connRef%u\n", id());
    fprintf(fp, "    connRef = new ConnRef(router, %u);\n", id());
    if (m_src_connend)
    {
        m_src_connend->outputCode(fp, "src");
        fprintf(fp, "    connRef->setSourceEndpoint(srcPt);\n");
    }
    else if (src())
    {
        fprintf(fp, "    srcPt = ConnEnd(Point(%" PREC "g, %" PREC "g), %u);\n",
                src()->point.x, src()->point.y, src()->visDirections);
        fprintf(fp, "    connRef->setSourceEndpoint(srcPt);\n");
    }
    if (m_dst_connend)
    {
        m_dst_connend->outputCode(fp, "dst");
        fprintf(fp, "    connRef->setDestEndpoint(dstPt);\n");
    }
    else if (dst())
    {
        fprintf(fp, "    dstPt = ConnEnd(Point(%" PREC "g, %" PREC "g), %u);\n",
                dst()->point.x, dst()->point.y, dst()->visDirections);
        fprintf(fp, "    connRef->setDestEndpoint(dstPt);\n");
    }
    fprintf(fp, "    connRef->setRoutingType((ConnType)%u);\n", routingType());

    if (m_has_fixed_route)
    {
        PolyLine currRoute = route();
        fprintf(fp, "    newRoute._id = %u;\n", id());
        fprintf(fp, "    newRoute.ps.resize(%d);\n", (int)currRoute.size());
        for (size_t i = 0; i < currRoute.size(); ++i)
        {
            fprintf(fp, "    newRoute.ps[%d] = Point(%" PREC "g, %" PREC "g);\n", 
                    (int) i, currRoute.ps[i].x, currRoute.ps[i].y);
            fprintf(fp, "    newRoute.ps[%d].id = %u;\n", 
                    (int) i, currRoute.ps[i].id);
            fprintf(fp, "    newRoute.ps[%d].vn = %u;\n",
                    (int) i, currRoute.ps[i].vn);
        }
        fprintf(fp, "    connRef->setFixedRoute(newRoute);\n");
    }

    if (!m_checkpoints.empty())
    {
        fprintf(fp, "    std::vector<Checkpoint> checkpoints%u(%d);\n", id(),
                (int) m_checkpoints.size());
        for (size_t cInd = 0; cInd < m_checkpoints.size(); ++cInd)
        {
            fprintf(fp, "    checkpoints%u[%d] = Checkpoint(Point("
                    "%" PREC "g, %" PREC "g), (ConnDirFlags) %d, "
                    "(ConnDirFlags) %d);\n", id(), (int) cInd, 
                    m_checkpoints[cInd].point.x, m_checkpoints[cInd].point.y,
                    m_checkpoints[cInd].arrivalDirections,
                    m_checkpoints[cInd].departureDirections);
        }
        fprintf(fp, "    connRef->setRoutingCheckpoints(checkpoints%u);\n", 
                id());
    }
    fprintf(fp, "\n");
}


std::pair<Obstacle *, Obstacle *> ConnRef::endpointAnchors(void) const
{
    std::pair<Obstacle *, Obstacle *> anchors;
    anchors.first = nullptr;
    anchors.second = nullptr;

    if (m_src_connend)
    {
        anchors.first = m_src_connend->m_anchor_obj;
    }
    if (m_dst_connend)
    {
        anchors.second = m_dst_connend->m_anchor_obj;
    }
    return anchors;
}

std::pair<ConnEnd, ConnEnd> ConnRef::endpointConnEnds(void) const
{
    std::pair<ConnEnd, ConnEnd> endpoints;
    getConnEndForEndpointVertex(m_src_vert, endpoints.first);
    getConnEndForEndpointVertex(m_dst_vert, endpoints.second);
    return endpoints;
}

bool ConnRef::setEndpoint(const unsigned int type, const VertID& pointID,
        Point *pointSuggestion)
{
    VertInf *vInf = m_router->vertices.getVertexByID(pointID);
    if (vInf == nullptr)
    {
        return false;
    }
    Point& point = vInf->point;
    if (pointSuggestion)
    {
        if (euclideanDist(point, *pointSuggestion) > 0.5)
        {
            return false;
        }
    }

    common_updateEndPoint(type, point);

    // Give this visibility just to the point it is over.
    EdgeInf *edge = new EdgeInf(
            (type == VertID::src) ? m_src_vert : m_dst_vert, vInf);
    // XXX: We should be able to set this to zero, but can't due to 
    //      assumptions elsewhere in the code.
    edge->setDist(0.001);

    m_router->processTransaction();
    return true;
}


void ConnRef::makeActive(void)
{
    COLA_ASSERT(!m_active);
    
    // Add to connRefs list.
    m_connrefs_pos = m_router->connRefs.insert(m_router->connRefs.begin(), this);
    m_active = true;
}


void ConnRef::freeActivePins(void)
{
    if (m_src_connend)
    {
        m_src_connend->freeActivePin();
    }
    if (m_dst_connend)
    {
        m_dst_connend->freeActivePin();
    }
}


void ConnRef::makeInactive(void)
{
    COLA_ASSERT(m_active);
    
    // Remove from connRefs list.
    m_router->connRefs.erase(m_connrefs_pos);
    m_active = false;
}


void ConnRef::freeRoutes(void)
{
    m_route.clear();
    m_display_route.clear();
}
    

const PolyLine& ConnRef::route(void) const
{
    return m_route;
}


PolyLine& ConnRef::routeRef(void)
{
    return m_route;
}


void ConnRef::set_route(const PolyLine& route)
{
    if (&m_display_route == &route)
    {
        db_printf("Error:\tTrying to update libavoid route with itself.\n");
        return;
    }
    m_display_route.ps = route.ps;

    //_display_route.clear();
}

void ConnRef::setFixedExistingRoute(void)
{
    COLA_ASSERT(m_route.size() >= 2);
    m_has_fixed_route = true;
    m_router->registerSettingsChange();
}

void ConnRef::setFixedRoute(const PolyLine& route)
{
    if (route.size() >= 2)
    {
        // Set endpoints based on the fixed route incase the 
        // fixed route is later cleared.
        setEndpoints(route.ps[0], route.ps[route.size() - 1]);
    }
    m_has_fixed_route = true;
    m_route = route;
    m_display_route = m_route.simplify();
    m_router->registerSettingsChange();
}

bool ConnRef::hasFixedRoute(void) const
{
    return m_has_fixed_route;
}

void ConnRef::clearFixedRoute(void)
{
    m_has_fixed_route = false;
    makePathInvalid();
    m_router->registerSettingsChange();
}

Polygon& ConnRef::displayRoute(void)
{
    if (m_display_route.empty())
    {
        // No displayRoute is set.  Simplify the current route to get it.
        m_display_route = m_route.simplify();
    }
    return m_display_route;
}


void ConnRef::calcRouteDist(void)
{
    double (*dist)(const Point& a, const Point& b) = 
            (m_type == ConnType_PolyLine) ? euclideanDist : manhattanDist;

    m_route_dist = 0;
    for (size_t i = 1; i < m_route.size(); ++i)
    {
        m_route_dist += dist(m_route.at(i), m_route.at(i - 1));
    }
}


bool ConnRef::needsRepaint(void) const
{
    return m_needs_repaint;
}


unsigned int ConnRef::id(void) const
{
    return m_id;
}


Point midpoint(Point a, Point b)
{
    Point midpoint;

    midpoint.x = (a.x + b.x) / 2.0;
    midpoint.y = (a.y + b.y) / 2.0;

    return midpoint;
}


std::pair<JunctionRef *, ConnRef *> ConnRef::splitAtSegment(
                const size_t segmentN)
{
    ConnRef *newConn = nullptr;
    JunctionRef *newJunction = nullptr;

    if (m_display_route.size() > segmentN)
    {
        // Position the junction at the midpoint of the desired segment.
        Point junctionPos = midpoint(m_display_route.at(segmentN - 1),
                m_display_route.at(segmentN));

        // Create the new junction.
        newJunction = new JunctionRef(router(), junctionPos);
        router()->addJunction(newJunction);
        newJunction->preferOrthogonalDimension(
                (m_display_route.at(segmentN - 1).x == 
                    m_display_route.at(segmentN).x) ? YDIM : XDIM);

        // Create a new connection routing from the junction to the original
        // connector's endpoint.
        ConnEnd newConnSrc = ConnEnd(newJunction);
        ConnEnd newConnDst = *m_dst_connend;
        newConn = new ConnRef(router(), newConnSrc, newConnDst);
        
        // Reroute the endpoint of the original connector to attach to the
        // new junction.
        ConnEnd oldConnDst = ConnEnd(newJunction);
        this->setDestEndpoint(oldConnDst);
    }

    return std::make_pair(newJunction, newConn);
}


VertInf *ConnRef::src(void) const
{
    return m_src_vert;
}

    
VertInf *ConnRef::dst(void) const
{
    return m_dst_vert;
}


VertInf *ConnRef::start(void)
{
    return m_start_vert;
}


bool ConnRef::isInitialised(void) const
{
    return m_active;
}


void ConnRef::unInitialise(void)
{
    m_router->vertices.removeVertex(m_src_vert);
    m_router->vertices.removeVertex(m_dst_vert);
    makeInactive();
}


void ConnRef::removeFromGraph(void)
{
    if (m_src_vert)
    {
        m_src_vert->removeFromGraph();
    }

    if (m_dst_vert)
    {
        m_dst_vert->removeFromGraph();
    }
}


void ConnRef::setCallback(void (*cb)(void *), void *ptr)
{
    m_callback_func = cb;
    m_connector = ptr;
}


void ConnRef::performCallback(void)
{
    if (m_callback_func)
    {
        m_callback_func(m_connector);
    }
}


void ConnRef::makePathInvalid(void)
{
    m_needs_reroute_flag = true;
}


Router *ConnRef::router(void) const
{
    return m_router;
}


// Validates a bend point on a path to check it does not form a zigzag corner.
// a, b, c are consecutive points on the path.  d and e are b's neighbours,
// forming the shape corner d-b-e.
//
bool validateBendPoint(VertInf *aInf, VertInf *bInf, VertInf *cInf)
{
    if (bInf->id.isConnectionPin() || bInf->id.isConnCheckpoint())
    {
        // We shouldn't check connection pins or checkpoints.
        return true;
    }
    bool bendOkay = true;

    if ((aInf == nullptr) || (cInf == nullptr))
    {
        // Not a bendpoint, i.e., the end of the connector, so don't test.
        return bendOkay;
    }

    COLA_ASSERT(bInf != nullptr);
    VertInf *dInf = bInf->shPrev;
    VertInf *eInf = bInf->shNext;
    COLA_ASSERT(dInf != nullptr);
    COLA_ASSERT(eInf != nullptr);

    Point& a = aInf->point;
    Point& b = bInf->point;
    Point& c = cInf->point;
    Point& d = dInf->point;
    Point& e = eInf->point;

    if ((a == b) || (b == c))
    {
        return bendOkay;
    }

#ifdef PATHDEBUG
    db_printf("a=(%g, %g)\n", a.x, a.y);
    db_printf("b=(%g, %g)\n", b.x, b.y);
    db_printf("c=(%g, %g)\n", c.x, c.y);
    db_printf("d=(%g, %g)\n", d.x, d.y);
    db_printf("e=(%g, %g)\n", e.x, e.y);
#endif
    // Check angle:
    int abc = vecDir(a, b, c);
#ifdef PATHDEBUG
    db_printf("(abc == %d) ", abc);
#endif
   
    if (abc == 0)
    {
        // The three consecutive point on the path are in a line.
        // There should always be an equally short path that skips this
        // bend point, but this check is used during rubber-band routing
        // so we allow this case.
        bendOkay = true;
    }
    else // (abc != 0)
    {
        COLA_ASSERT(vecDir(d, b, e) > 0);
        int abe = vecDir(a, b, e);
        int abd = vecDir(a, b, d);
        int bce = vecDir(b, c, e);
        int bcd = vecDir(b, c, d);
#ifdef PATHDEBUG
        db_printf("&& (abe == %d) && (abd == %d) &&\n(bce == %d) && (bcd == %d)",
                abe, abd, bce, bcd);
#endif

        bendOkay = false;
        if (abe > 0)
        {
            if ((abc > 0) && (abd >= 0) && (bce >= 0))
            {
                bendOkay = true;
            }
        }
        else if (abd < 0)
        {
            if ((abc < 0) && (abe <= 0) && (bcd <= 0))
            {
                bendOkay = true;
            }
        }
    }
#ifdef PATHDEBUG
    db_printf("\n");
#endif
    return bendOkay;
}


std::pair<bool, bool> ConnRef::assignConnectionPinVisibility(const bool connect)
{
    // XXX This is kind of a hack for connection pins.  Probably we want to
    //     not use m_src_vert and m_dst_vert.  For the moment we will clear
    //     their visibility and give them visibility to the pins.
    bool dummySrc = m_src_connend && m_src_connend->isPinConnection();
    if (dummySrc)
    {
        m_src_vert->removeFromGraph();
        if (connect)
        {
            m_src_connend->assignPinVisibilityTo(m_src_vert, m_dst_vert);
        }
    }
    bool dummyDst = m_dst_connend && m_dst_connend->isPinConnection();
    if (dummyDst)
    {
        m_dst_vert->removeFromGraph();
        if (connect)
        {
            m_dst_connend->assignPinVisibilityTo(m_dst_vert, m_src_vert);
        }
    }

    return std::make_pair(dummySrc, dummyDst);
}


bool ConnRef::generatePath(void)
{
    // XXX Currently rubber-band routing only works when dragging the
    //     destination point of a connector, but not the source.  The code
    //     needs to be reworked to work in both directions.

    if (!m_false_path && !m_needs_reroute_flag)
    {
        // This connector is up to date.
        return false;
    }

    if (!m_dst_vert || !m_src_vert)
    {
        // Connector is not fully initialised.
        return false;
    }
    
    //COLA_ASSERT(_srcVert->point != _dstVert->point);

    m_false_path = false;
    m_needs_reroute_flag = false;

    m_start_vert = m_src_vert;

    // Some connectors may attach to connection pins, which means they route
    // to the closest of multiple pins on a shape.  How we handle this is to
    // add a dummy vertex as the source or target vertex.  This is then given
    // visibility to each of the possible pins and tiny distance.  Here we
    // assign this visibility by adding edges to the visibility graph that we
    // later remove.
    std::pair<bool, bool> isDummyAtEnd = assignConnectionPinVisibility(true);
    

    if (m_router->RubberBandRouting && route().size() > 0)
    {
        if (isDummyAtEnd.first)
        {
            //ShapeConnectionPin *activePin = m_src_connend->active
            Point firstPoint = m_src_vert->point;
            firstPoint.id = m_src_vert->id.objID;
            firstPoint.vn = m_src_vert->id.vn;
            PolyLine& existingRoute = routeRef();
            existingRoute.ps.insert(existingRoute.ps.begin(), 1, firstPoint);
        }
    }

    std::vector<Point> path;
    std::vector<VertInf *> vertices;
    if (m_checkpoints.empty())
    {
        generateStandardPath(path, vertices);
    }
    else
    {
        generateCheckpointsPath(path, vertices);
    }

    COLA_ASSERT(vertices.size() >= 2);
    COLA_ASSERT(vertices[0] == src());
    COLA_ASSERT(vertices[vertices.size() - 1] == dst());
    COLA_ASSERT(m_reroute_flag_ptr != nullptr);

    for (size_t i = 1; i < vertices.size(); ++i)
    {
        if (m_router->InvisibilityGrph && (m_type == ConnType_PolyLine))
        {
            // TODO: Again, we could know this edge without searching.
            EdgeInf *edge = EdgeInf::existingEdge(vertices[i - 1], vertices[i]);
            if (edge) {
                edge->addConn(m_reroute_flag_ptr);
            }
        }
        else
        {
            m_false_path = true;
        }

        VertInf *vertex = vertices[i];
        if (vertex->pathNext && 
                (vertex->pathNext->point == vertex->point))
        {
            if (!(vertex->pathNext->id.isConnPt()) && !(vertex->id.isConnPt()))
            {
                // Check for consecutive points on opposite 
                // corners of two touching shapes.
                COLA_ASSERT(abs(vertex->pathNext->id.vn - vertex->id.vn) != 2);
            }
        }
    }

    // Get rid of dummy ShapeConnectionPin bridging points at beginning
    // and end of path.
    std::vector<Point> clippedPath;
    std::vector<Point>::iterator pathBegin = path.begin();
    std::vector<Point>::iterator pathEnd = path.end();
    if (path.size() > 2 && isDummyAtEnd.first)
    {
        ++pathBegin;
        m_src_connend->usePinVertex(vertices[1]);
    }
    if (path.size() > 2 && isDummyAtEnd.second)
    {
        --pathEnd;
        m_dst_connend->usePinVertex(vertices[vertices.size() - 2]);
    }
    clippedPath.insert(clippedPath.end(), pathBegin, pathEnd);
    
    // Clear visibility edges added for connection pins dummy vertices.
    assignConnectionPinVisibility(false);

    freeRoutes();
    PolyLine& output_route = m_route;
    output_route.ps = clippedPath;
 
#ifdef PATHDEBUG
    db_printf("Output route:\n");
    for (size_t i = 0; i < output_route.ps.size(); ++i)
    {
        db_printf("[%d,%d] %g, %g   ", output_route.ps[i].id, 
                output_route.ps[i].vn, output_route.ps[i].x, 
                output_route.ps[i].y);
    }
    db_printf("\n\n");
#endif

#ifdef DEBUGHANDLER
    if (m_router->debugHandler())
    {
        m_router->debugHandler()->updateConnectorRoute(this, -1, -1);
    }
#endif

    return true;
}

void ConnRef::generateCheckpointsPath(std::vector<Point>& path,
        std::vector<VertInf *>& vertices)
{
    std::vector<VertInf *> checkpoints = m_checkpoint_vertices;
    checkpoints.insert(checkpoints.begin(), src());
    checkpoints.push_back(dst());
    
    path.clear();
    vertices.clear();
    path.push_back(src()->point);
    vertices.push_back(src());
 
    size_t lastSuccessfulIndex = 0;
    for (size_t i = 1; i < checkpoints.size(); ++i)
    {
        VertInf *start = checkpoints[lastSuccessfulIndex];
        VertInf *end = checkpoints[i];
        
        // Handle checkpoint directions by disabling some visibility edges.
        if (lastSuccessfulIndex > 0)
        {
            Checkpoint& srcCP = m_checkpoints[lastSuccessfulIndex - 1];
            if (srcCP.departureDirections != ConnDirAll)
            {
                start->setVisibleDirections(srcCP.departureDirections);
            }
        }
        if ((i + 1) < checkpoints.size())
        {
            Checkpoint& dstCP = m_checkpoints[i - 1];
            if (dstCP.arrivalDirections != ConnDirAll)
            {
                end->setVisibleDirections(dstCP.arrivalDirections);
            }
        }
        
        AStarPath aStar;
        // Route the connector
        aStar.search(this, start, end, nullptr); 

        // Restore changes made for checkpoint visibility directions.
        if (lastSuccessfulIndex > 0)
        {
            start->setVisibleDirections(ConnDirAll);
        }
        if ((i + 1) < checkpoints.size())
        {
            end->setVisibleDirections(ConnDirAll);
        }

        // Process the path.
        int pathlen = end->pathLeadsBackTo(start);
        if (pathlen >= 2)
        {
            size_t prev_path_size = path.size();
            path.resize(prev_path_size + (pathlen - 1));
            vertices.resize(prev_path_size + (pathlen - 1));
            VertInf *vertInf = end;
            for (size_t index = path.size() - 1; index >= prev_path_size;
                    --index)
            {
                path[index] = vertInf->point;
                if (vertInf->id.isConnPt())
                {
                    path[index].id = m_id;
                    path[index].vn = kUnassignedVertexNumber;
                }
                else
                {
                    path[index].id = vertInf->id.objID;
                    path[index].vn = vertInf->id.vn;
                }
                vertices[index] = vertInf;
                vertInf = vertInf->pathNext;
            }
            lastSuccessfulIndex = i;
        }
        else if (i + 1 == checkpoints.size())
        {
            // There is no valid path.
            db_printf("Warning: Path not found...\n");
            m_needs_reroute_flag = true;
            
            path.push_back(dst()->point);
            vertices.push_back(dst());

            COLA_ASSERT(path.size() >= 2);
        }
        else
        {
            err_printf("Warning: skipping checkpoint for connector "
                    "%d at (%g, %g).\n", (int) id(), 
                    checkpoints[i]->point.x, checkpoints[i]->point.y);
            fflush(stderr);
        }
    }
    // Use topbit to differentiate between start and end point of connector.
    // They need unique IDs for nudging.
    unsigned int topbit = ((unsigned int) 1) << 31;
    path[path.size() - 1].id = m_id | topbit; 
    path[path.size() - 1].vn = kUnassignedVertexNumber;
}


void ConnRef::generateStandardPath(std::vector<Point>& path,
        std::vector<VertInf *>& vertices)
{
    VertInf *tar = m_dst_vert;
    size_t existingPathStart = 0;
    const PolyLine& currRoute = route();
    if (m_router->RubberBandRouting)
    {
        COLA_ASSERT(m_router->IgnoreRegions == true);

#ifdef PATHDEBUG
        db_printf("\n");
        src()->id.db_print();
        db_printf(": %g, %g\n", src()->point.x, src()->point.y);
        tar->id.db_print();
        db_printf(": %g, %g\n", tar->point.x, tar->point.y);
        for (size_t i = 0; i < currRoute.ps.size(); ++i)
        {
            db_printf("%g, %g  ", currRoute.ps[i].x, currRoute.ps[i].y);
        }
        db_printf("\n");
#endif
        if (currRoute.size() > 2)
        {
            if (m_src_vert->point == currRoute.ps[0])
            {
                existingPathStart = currRoute.size() - 2;
                COLA_ASSERT(existingPathStart != 0);
                const Point& pnt = currRoute.at(existingPathStart);
                VertID vID(pnt.id, pnt.vn);

                m_start_vert = m_router->vertices.getVertexByID(vID);
                COLA_ASSERT(m_start_vert);
            }
        }
    }
    //db_printf("GO\n");
    //db_printf("src: %X strt: %X dst: %X\n", (int) m_src_vert, (int) m_start_vert, (int) m_dst_vert);
    unsigned int pathlen = 0;
    while (pathlen == 0)
    {
        AStarPath aStar;
        aStar.search(this, src(), dst(), start());
        pathlen = dst()->pathLeadsBackTo(src());
        if (pathlen < 2)
        {
            if (existingPathStart == 0)
            {
                break;
            }
#ifdef PATHDEBUG
            db_printf("BACK\n");
#endif
            existingPathStart--;
            const Point& pnt = currRoute.at(existingPathStart);
            VertIDProps props = (existingPathStart > 0) ? 0 :
                    VertID::PROP_ConnPoint;
            VertID vID(pnt.id, pnt.vn, props);

            m_start_vert = m_router->vertices.getVertexByID(vID);
            COLA_ASSERT(m_start_vert);
        }
        else if (m_router->RubberBandRouting)
        {
            // found.
            bool unwind = false;

#ifdef PATHDEBUG
            db_printf("\n\n\nSTART:\n\n");
#endif
            VertInf *prior = nullptr;
            for (VertInf *curr = tar; curr != m_start_vert->pathNext;
                    curr = curr->pathNext)
            {
                if (!validateBendPoint(curr->pathNext, curr, prior))
                {
                    unwind = true;
                    break;
                }
                prior = curr;
            }
            if (unwind)
            {
#ifdef PATHDEBUG
                db_printf("BACK II\n");
#endif
                if (existingPathStart == 0)
                {
                    break;
                }
                existingPathStart--;
                const Point& pnt = currRoute.at(existingPathStart);
                VertIDProps props = (existingPathStart > 0) ? 0 :
                        VertID::PROP_ConnPoint;
                VertID vID(pnt.id, pnt.vn, props);

                m_start_vert = m_router->vertices.getVertexByID(vID);
                COLA_ASSERT(m_start_vert);

                pathlen = 0;
            }
        }
    }

    if (pathlen < 2)
    {
        // There is no valid path.
        db_printf("Warning: Path not found...\n");
        m_needs_reroute_flag = true;
        pathlen = 2;
        tar->pathNext = m_src_vert;
        if ((m_type == ConnType_PolyLine) && m_router->InvisibilityGrph)
        {
            // TODO:  Could we know this edge already?
            //EdgeInf *edge = EdgeInf::existingEdge(m_src_vert, tar);
            //COLA_ASSERT(edge != nullptr);
            //edge->addCycleBlocker();
        }
    }
    path.resize(pathlen);
    vertices.resize(pathlen);

    unsigned int j = pathlen - 1;
    for (VertInf *i = tar; i != m_src_vert; i = i->pathNext)
    {
        path[j] = i->point;
        vertices[j] = i;
        path[j].id = i->id.objID;
        path[j].vn = i->id.vn;

        j--;
    }
    vertices[0] = m_src_vert;
    path[0] = m_src_vert->point;
    path[0].id = m_src_vert->id.objID;
    path[0].vn =m_src_vert->id.vn;
}


void ConnRef::setHateCrossings(bool value)
{
    m_hate_crossings = value;
}


bool ConnRef::doesHateCrossings(void) const
{
    return m_hate_crossings;
}


std::vector<Point> ConnRef::possibleDstPinPoints(void) const
{
    std::vector<Point> points;
    if (m_dst_connend)
    {
        points = m_dst_connend->possiblePinPoints();
    }
    return points;
}


PtOrder::PtOrder()
{
    // We have sorted neither list initially.
    for (size_t dim = 0; dim < 2; ++dim)
    {
        sorted[dim] = false;
    }
}


PtOrder::~PtOrder()
{
}


PointRepVector PtOrder::sortedPoints(const size_t dim)
{
    // Sort if not already sorted.
    if (sorted[dim] == false)
    {
        sort(dim);
    }
    return sortedConnVector[dim];
}


int PtOrder::positionFor(const size_t dim, const ConnRef *conn)
{
    // Sort if not already sorted.
    if (sorted[dim] == false)
    {
        sort(dim);
    }

    // Just return position from the sorted list.
    size_t i = 0;
    for ( ; i < sortedConnVector[dim].size(); ++i)
    {
        if (sortedConnVector[dim][i].second == conn)
        {
            return (int) i;
        }
    }
    return -1;
}


size_t PtOrder::insertPoint(const size_t dim, const PtConnPtrPair& pointPair)
{
    // Is this connector bendpoint already inserted?
    size_t i = 0;
    for ( ; i < nodes[dim].size(); ++i)
    {
        if (nodes[dim][i].second == pointPair.second)
        {
            return i;
        }
    }
    // Not found, insert.
    nodes[dim].push_back(pointPair);
    return nodes[dim].size() - 1;
}

void PtOrder::addPoints(const size_t dim, const PtConnPtrPair& arg1, 
                const PtConnPtrPair& arg2)
{
    // Add points, but not ordering information.
    insertPoint(dim, arg1);
    insertPoint(dim, arg2);
}


void PtOrder::addOrderedPoints(const size_t dim, const PtConnPtrPair& innerArg, 
        const PtConnPtrPair& outerArg, bool swapped)
{
    PtConnPtrPair inner = (swapped) ? outerArg : innerArg;
    PtConnPtrPair outer = (swapped) ? innerArg : outerArg;
    COLA_ASSERT(inner != outer);

    // Add points.
    size_t innerIndex = insertPoint(dim, inner);
    size_t outerIndex = insertPoint(dim, outer);
   
    // And edge for ordering information.
    links[dim].push_back(std::make_pair(outerIndex, innerIndex));
}


void PtOrder::sort(const size_t dim)
{
    // This is just a topological sort of the points using the edges info.

    sorted[dim] = true;

    size_t n = nodes[dim].size();

    // Build an adjacency matrix for easy lookup.
    std::vector<std::vector<bool> > adjacencyMatrix(n);
    for (size_t i = 0; i < n; ++i)
    {
        adjacencyMatrix[i].assign(n, false);
    }
    std::vector<int> incomingDegree(n);
    std::queue<size_t> queue;

    // Populate the dependency matrix.
    for (NodeIndexPairLinkList::iterator it = links[dim].begin(); 
            it != links[dim].end(); ++it)
    {
        adjacencyMatrix[it->first][it->second] = true;
    }

    // Build incoming degree lookup structure, and add nodes with no
    // incoming edges to queue.
    for (size_t i = 0; i < n; ++i)
    {
        int degree = 0;
 
        for (size_t j = 0; j < n; ++j)
        {
            if (adjacencyMatrix[j][i])
            {
                degree++;
            }
        }
        incomingDegree[i] = degree;

        if (degree == 0)
        {
            queue.push(i);
        }
    }
 
    while (queue.empty() == false)
    {
        size_t k = queue.front();
        assert(k < nodes[dim].size());
        queue.pop();

        // Insert node k into the sorted list
        sortedConnVector[dim].push_back(nodes[dim][k]);

        // Remove all edges leaving node k:
        for (size_t i = 0; i < n; ++i)
        {
            if (adjacencyMatrix[k][i])
            {
                adjacencyMatrix[k][i] = false;
                incomingDegree[i]--;
 
                if (incomingDegree[i] == 0)
                {
                    queue.push(i);
                }
            }
        }
    }
}


// Returns a vertex number representing a point on the line between 
// two shape corners, represented by p0 and p1.
//
static int midVertexNumber(const Point& p0, const Point& p1, const Point& c)
{
    if (c.vn != kUnassignedVertexNumber)
    {
        // The split point is a shape corner, so doesn't need its 
        // vertex number adjusting.
        return c.vn;
    }
    if ((p0.vn >= 4) && (p0.vn < kUnassignedVertexNumber))
    {
        // The point next to this has the correct nudging direction,
        // so use that.
        return p0.vn;
    }
    if ((p1.vn >= 4) && (p1.vn < kUnassignedVertexNumber))
    {
        // The point next to this has the correct nudging direction,
        // so use that.
        return p1.vn;
    }
    if ((p0.vn < 4) && (p1.vn < 4))
    {
        if (p0.vn != p1.vn)
        {
            return p0.vn;
        }
        // Splitting between two ordinary shape corners.
        int vn_mid = std::min(p0.vn, p1.vn);
        if ((std::max(p0.vn, p1.vn) == 3) && (vn_mid == 0))
        {
            vn_mid = 3; // Next vn is effectively 4.
        }
        return vn_mid + 4;
    }
    COLA_ASSERT((p0.x == p1.x) || (p0.y == p1.y));
    if (p0.vn != kUnassignedVertexNumber)
    {
        if (p0.x == p1.x)
        {
            if ((p0.vn == 2) || (p0.vn == 3))
            {
                return 6;
            }
            return 4;
        }
        else
        {
            if ((p0.vn == 0) || (p0.vn == 3))
            {
                return 7;
            }
            return 5;
        }
    }
    else if (p1.vn != kUnassignedVertexNumber)
    {
        if (p0.x == p1.x)
        {
            if ((p1.vn == 2) || (p1.vn == 3))
            {
                return 6;
            }
            return 4;
        }
        else
        {
            if ((p1.vn == 0) || (p1.vn == 3))
            {
                return 7;
            }
            return 5;
        }
    }

    // Shouldn't both be new (kUnassignedVertexNumber) points.
    db_printf("midVertexNumber(): p0.vn and p1.vn both = "
            "kUnassignedVertexNumber\n");
    db_printf("p0.vn %d p1.vn %d\n", p0.vn, p1.vn);
    return kUnassignedVertexNumber;
}


// Break up overlapping parallel segments that are not the same edge in 
// the visibility graph, i.e., where one segment is a subsegment of another.
void splitBranchingSegments(Avoid::Polygon& poly, bool polyIsConn,
        Avoid::Polygon& conn, const double tolerance)
{
    for (std::vector<Avoid::Point>::iterator i = conn.ps.begin(); 
            i != conn.ps.end(); ++i)
    {
        if (i == conn.ps.begin())
        {
            // Skip the first point.
            // There are points-1 segments in a connector.
            continue;
        }

        for (std::vector<Avoid::Point>::iterator j = poly.ps.begin(); 
                j != poly.ps.end(); )
        {
            if (polyIsConn && (j == poly.ps.begin()))
            {
                // Skip the first point.
                // There are points-1 segments in a connector.
                ++j;
                continue;
            }
            Point& c0 = *(i - 1);
            Point& c1 = *i;

            Point& p0 = (j == poly.ps.begin()) ? poly.ps.back() : *(j - 1);
            Point& p1 = *j;

            // Check the first point of the first segment.
            if (((i - 1) == conn.ps.begin()) && 
                    pointOnLine(p0, p1, c0, tolerance))
            {
                //db_printf("add to poly %g %g\n", c0.x, c0.y);
                
                c0.vn = midVertexNumber(p0, p1, c0);
                j = poly.ps.insert(j, c0);
                if (j != poly.ps.begin())
                {
                    --j;
                }
                continue;
            }
            // And the second point of every segment.
            if (pointOnLine(p0, p1, c1, tolerance))
            {
                //db_printf("add to poly %g %g\n", c1.x, c1.y);
                
                c1.vn = midVertexNumber(p0, p1, c1);
                j = poly.ps.insert(j, c1);
                if (j != poly.ps.begin())
                {
                    --j;
                }
                continue;
            }

            // Check the first point of the first segment.
            if (polyIsConn && ((j - 1) == poly.ps.begin()) && 
                        pointOnLine(c0, c1, p0, tolerance))
            {
                //db_printf("add to conn %g %g\n", p0.x, p0.y);

                p0.vn = midVertexNumber(c0, c1, p0);
                i = conn.ps.insert(i, p0);
                continue;
            }
            // And the second point of every segment.
            if (pointOnLine(c0, c1, p1, tolerance))
            {
                //db_printf("add to conn %g %g\n", p1.x, p1.y);

                p1.vn = midVertexNumber(c0, c1, p1);
                i = conn.ps.insert(i, p1);
            }
            ++j;
        }
    }
}


static int segDir(const Point& p1, const Point& p2)
{
    int result = 1;
    if (p1.x == p2.x)
    {
        if (p2.y > p1.y)
        {
            result = -1;
        }
    }
    else if (p1.y == p2.y)
    {
        if (p2.x < p1.x)
        {
            result = -1;
        }
    }
    return result;
}


static bool posInlineWithConnEndSegs(const double pos, const size_t dim, 
        const Avoid::Polygon& poly, const Avoid::Polygon& conn)
{
    size_t pLast = poly.size() - 1;
    size_t cLast = conn.size() - 1;
    if ((
         // Is inline with the beginning of the "poly" line
         ((pos == poly.ps[0][dim]) && (pos == poly.ps[1][dim])) ||
         // Is inline with the end of the "poly" line
         ((pos == poly.ps[pLast][dim]) && (pos == poly.ps[pLast - 1][dim])) 
        ) && (
         // Is inline with the beginning of the "conn" line
         ((pos == conn.ps[0][dim]) && (pos == conn.ps[1][dim])) || 
         // Is inline with the end of the "conn" line
         ((pos == conn.ps[cLast][dim]) && (pos == conn.ps[cLast - 1][dim]))
        ))
    {
        return true;
    }
    return false;
}

ConnectorCrossings::ConnectorCrossings(Avoid::Polygon& poly, bool polyIsConn,
        Avoid::Polygon& conn, ConnRef *polyConnRef, ConnRef *connConnRef)
    : poly(poly),
      polyIsConn(polyIsConn),
      conn(conn),
      checkForBranchingSegments(false),
      polyConnRef(polyConnRef),
      connConnRef(connConnRef),
      crossingPoints(nullptr),
      pointOrders(nullptr),
      sharedPaths(nullptr)
{
}

void ConnectorCrossings::clear(void)
{
    crossingCount = 0;
    crossingFlags = CROSSING_NONE;
    firstSharedPathAtEndLength = DBL_MAX;
    secondSharedPathAtEndLength = DBL_MAX;
}


// Computes the *shared* length of these two shared paths.
//
static double pathLength(Avoid::Point **c_path, Avoid::Point **p_path, 
        size_t size)
{
    double length = 0;

    for (unsigned int ind = 1; ind < size; ++ind)
    {
        if ( (*(c_path[ind - 1]) == *(p_path[ind - 1])) && 
             (*(c_path[ind]) == *(p_path[ind])) )
        {
            // This segment is shared by both paths.
            //
            // This function will only be used for orthogonal paths, so we 
            // can use Manhattan distance here since it will be faster to 
            // compute.
            length += manhattanDist(*(c_path[ind - 1]), *(c_path[ind]));
        }
    }

    return length;
}

// Works out if the segment conn[cIndex-1]--conn[cIndex] really crosses poly.
// This does not not count non-crossing shared paths as crossings.
// poly can be either a connector (polyIsConn = true) or a cluster
// boundary (polyIsConn = false).
//
void ConnectorCrossings::countForSegment(size_t cIndex, const bool finalSegment)
{
    clear();

    bool polyIsOrthogonal = (polyConnRef && 
            (polyConnRef->routingType() == ConnType_Orthogonal));
    bool connIsOrthogonal = (connConnRef &&
            (connConnRef->routingType() == ConnType_Orthogonal));

    // Fixed routes will not have segment breaks at possible crossings.
    bool polyIsFixed = (polyConnRef && polyConnRef->hasFixedRoute());
    bool connIsFixed = (connConnRef && connConnRef->hasFixedRoute());
    
    // We need to split apart connectors at potential crossing points if
    // either has a fixed route or it is a polyline connector.  This is not
    // needed for orthogonal connectors where crossings occur at vertices 
    // in visibility graph and on the raw connector routes.
    if (checkForBranchingSegments || polyIsFixed || connIsFixed ||
            !polyIsOrthogonal || !connIsOrthogonal)
    {
        double epsilon = std::numeric_limits<double>::epsilon();
        size_t conn_pn = conn.size();
        // XXX When doing the pointOnLine test we allow the points to be 
        // slightly non-collinear.  This addresses a problem with clustered
        // routing where connectors could otherwise route cheaply through
        // shape corners that were not quite on the cluster boundary, but
        // reported to be on there by the line segment intersection code,
        // which I suspect is not numerically accurate enough.  This occurred
        // for points that only differed by about 10^-12 in the y-dimension.
        double tolerance = (!polyIsConn) ? epsilon : 0.0;
        splitBranchingSegments(poly, polyIsConn, conn, tolerance);
        // cIndex is going to be the last, so take into account added points.
        cIndex += (conn.size() - conn_pn);
    }
    COLA_ASSERT(cIndex >= 1);
    COLA_ASSERT(cIndex < conn.size());

    size_t poly_size = poly.size();

    Avoid::Point& a1 = conn.ps[cIndex - 1];
    Avoid::Point& a2 = conn.ps[cIndex];
    //db_printf("a1: %g %g\n", a1.x, a1.y);
    //db_printf("a2: %g %g\n", a2.x, a2.y);

    // Allocate arrays for computing shared paths.
    // Don't use dynamic array due to portablity issues.
    size_t max_path_size = std::min(poly_size, conn.size());
    Avoid::Point **c_path = new Avoid::Point*[max_path_size];
    Avoid::Point **p_path = new Avoid::Point*[max_path_size];
    size_t size = 0;

    for (size_t j = ((polyIsConn) ? 1 : 0); j < poly_size; ++j)
    {
        Avoid::Point& b1 = poly.ps[(j - 1 + poly_size) % poly_size];
        Avoid::Point& b2 = poly.ps[j];
        //db_printf("b1: %g %g\n", b1.x, b1.y);
        //db_printf("b2: %g %g\n", b2.x, b2.y);

        size = 0;

        bool converging = false;

        const bool a1_eq_b1 = (a1 == b1);
        const bool a2_eq_b1 = (a2 == b1);
        const bool a2_eq_b2 = (a2 == b2);
        const bool a1_eq_b2 = (a1 == b2);

        if ( (a1_eq_b1 && a2_eq_b2) ||
             (a2_eq_b1 && a1_eq_b2) )
        {
            if (finalSegment)
            {
                converging = true;
            }
            else
            {
                // Route along same segment: no penalty.  We detect
                // crossovers when we see the segments diverge.
                continue;
            }
        }
        else if (a2_eq_b1 || a2_eq_b2 || a1_eq_b2)
        {
            // Each crossing that is at a vertex in the 
            // visibility graph gets noticed four times.
            // We ignore three of these cases.
            // This also catches the case of a shared path,
            // but this is one that terminates at a common
            // endpoint, so we don't care about it.
            continue;
        }
    
        if (a1_eq_b1 || converging)
        {
            if (!converging)
            {
                if (polyIsConn && (j == 1))
                {
                    // Can't be the end of a shared path or crossing path 
                    // since the common point is the first point of the 
                    // connector path.  This is not a shared path at all.
                    continue;
                }

                Avoid::Point& b0 = poly.ps[(j - 2 + poly_size) % poly_size];
                // The segments share an endpoint -- a1==b1.
                if (a2 == b0)
                {
                    // a2 is not a split, continue.
                    continue;
                }
            }
            
            // If here and not converging, then we know that a2 != b2
            // And a2 and its pair in b are a split.
            COLA_ASSERT(converging || !a2_eq_b2);

            bool shared_path = false;
            
            // Initial values here don't matter. They are only used after 
            // being set to sensible values, but we set them to stop a MSVC
            // warning.
            bool p_dir_back;
            int p_dir = 0;
            int trace_c = 0;
            int trace_p = 0;
            
            if (converging)
            {
                // Determine direction we have to look through
                // the points of connector b.
                p_dir_back = a2_eq_b2 ? true : false;
                p_dir = p_dir_back ? -1 : 1;
                trace_c = (int) cIndex;
                trace_p = (int) j;
                if (!p_dir_back)
                {
                    if (finalSegment)
                    {
                        trace_p--;
                    }
                    else
                    {   
                        trace_c--;
                    }
                }

                shared_path = true;
            }
            else if (cIndex >= 2)
            {
                Avoid::Point& b0 = poly.ps[(j - 2 + poly_size) % poly_size];
                Avoid::Point& a0 = conn.ps[cIndex - 2];
            
                //db_printf("a0: %g %g\n", a0.x, a0.y);
                //db_printf("b0: %g %g\n", b0.x, b0.y);

                if ((a0 == b2) || (a0 == b0))
                {
                    // Determine direction we have to look through
                    // the points of connector b.
                    p_dir_back = (a0 == b0) ? true : false;
                    p_dir = p_dir_back ? -1 : 1;
                    trace_c = (int) cIndex;
                    trace_p = (int) (p_dir_back ? j : j - 2);
                    
                    shared_path = true;
                }
            }    

            if (shared_path)
            {
                crossingFlags |= CROSSING_SHARES_PATH;
                // Shouldn't be here if p_dir is still equal to zero.
                COLA_ASSERT(p_dir != 0);

                // Build the shared path, including the diverging points at
                // each end if the connector does not end at a common point.
                while ( (trace_c >= 0) && (!polyIsConn || 
                            ((trace_p >= 0) && (trace_p < (int) poly_size))) )
                {
                    // If poly is a cluster boundary, then it is a closed 
                    // poly-line and so it wraps around.
                    size_t index_p = (size_t)
                            ((trace_p + (2 * poly_size)) % poly_size);
                    size_t index_c = (size_t) trace_c;
                    c_path[size] = &conn.ps[index_c];
                    p_path[size] = &poly.ps[index_p];
                    ++size;
                    if ((size > 1) && (conn.ps[index_c] != poly.ps[index_p]))
                    {
                        // Points don't match, so break out of loop.
                        break;
                    }
                    trace_c--;
                    trace_p += p_dir;
                }

                // Are there diverging points at the ends of the shared path.
                bool front_same = (*(c_path[0]) == *(p_path[0]));
                bool back_same  = (*(c_path[size - 1]) == *(p_path[size - 1]));

                // Determine if the shared path originates at a junction.
                bool terminatesAtJunction = false;
                if (polyConnRef && connConnRef && (front_same || back_same))
                {
                    // To do this we find the two ConnEnds at the common 
                    // end of the shared path.  Then check if they are 
                    // attached to a junction and it is the same one.
                    std::pair<ConnEnd, ConnEnd> connEnds = 
                            connConnRef->endpointConnEnds();
                    JunctionRef *connJunction = nullptr;

                    std::pair<ConnEnd, ConnEnd> polyEnds = 
                            polyConnRef->endpointConnEnds();
                    JunctionRef *polyJunction = nullptr;
                   
                    // The front of the c_path corresponds to destination 
                    // of the connector.
                    connJunction = (front_same) ? connEnds.second.junction() : 
                            connEnds.first.junction();
                    bool use_first = back_same;
                    if (p_dir_back)
                    {
                        // Reversed, so use opposite.
                        use_first = !use_first;
                    }
                    // The front of the p_path corresponds to destination 
                    // of the connector.
                    polyJunction = (use_first) ? polyEnds.second.junction() :
                            polyEnds.first.junction();
                    
                    terminatesAtJunction = (connJunction && 
                            (connJunction == polyJunction));
                }

                if (sharedPaths)
                {
                    // Store a copy of the shared path
                    size_t start = (front_same) ? 0 : 1;
                    size_t limit = size - ((back_same) ? 0 : 1);
                
                    PointList sPath(limit - start);
                    for (size_t i = start; i < limit; ++i)
                    {
                        sPath[i - start] = *(c_path[i]);
                    }
                    sharedPaths->push_back(sPath);
                }

                // Check to see if these share a fixed segment.
                if (polyIsOrthogonal && connIsOrthogonal)
                {
                    size_t startPt = (front_same) ? 0 : 1;
                    size_t endPt = size - ((back_same) ? 1 : 2);
                    for (size_t dim = 0; dim < 2; ++dim)
                    {
                        if ((*c_path[startPt])[dim] == (*c_path[endPt])[dim])
                        {
                            double pos = (*c_path[startPt])[dim];
                            // See if this is inline with either the start
                            // or end point of both connectors.  We don't
                            // count them as crossing if they originate at a
                            // junction and are part of the same hyperedge.
                            if ( ((pos == poly.ps[0][dim]) ||
                                    (pos == poly.ps[poly_size - 1][dim])) &&
                                 ((pos == conn.ps[0][dim]) ||
                                    (pos == conn.ps[cIndex][dim])) &&
                                 (terminatesAtJunction == false))
                            {
                                crossingFlags |= CROSSING_SHARES_FIXED_SEGMENT;
                            }
                        }
                    }

                    if (!front_same && !back_same)
                    {
                        // Find overlapping segments that are constrained by
                        // the fact that both the adjoining segments are fixed
                        // in the other dimension, i.e.,
                        //
                        // X------++---X
                        //        ||
                        //        ||
                        //    X---++------X
                        //
                        // In the example above, altDim is X, and dim is Y.
                        //
                        
                        // For each dimension...
                        for (size_t dim = 0; dim < 2; ++dim)
                        {
                            size_t end = size - 1;
                            size_t altDim = (dim + 1) % 2;
                            // If segment is in this dimension...
                            if ((*c_path[1])[altDim] == (*c_path[end - 1])[altDim])
                            {
                                double posBeg = (*c_path[1])[dim];
                                double posEnd = (*c_path[end - 1])[dim];
                                // If both segment ends diverge at right-angles...
                                if ( (posBeg == (*c_path[0])[dim]) && 
                                        (posBeg == (*p_path[0])[dim]) &&
                                     (posEnd == (*c_path[end])[dim]) && 
                                        (posEnd == (*p_path[end])[dim]) )
                                {
                                    // and these segments are inline with the conn and path ends themselves...
                                    if (posInlineWithConnEndSegs(posBeg, dim,
                                                conn, poly) &&
                                        posInlineWithConnEndSegs(posEnd, dim,
                                                conn, poly))
                                    {
                                    // If all endpoints branch at right angles,
                                    // then penalise this since it is a segment
                                    // will will not be able to nudge apart
                                    // without introducing fixed segment 
                                    // crossings.
                                    crossingFlags |= 
                                            CROSSING_SHARES_FIXED_SEGMENT;
                                    }
                                }
                            }
                        }
                    }

#if 0
                    // XXX: What is this code for?  It is pretty much 
                    // incomprehensible and also causes one of the test
                    // cases to fail.  
                    //
                    if (!front_same && !back_same)
                    {
                        for (size_t dim = 0; dim < 2; ++dim)
                        {
                            size_t altDim = (dim + 1) % 2;
                            if ((*c_path[1])[altDim] == (*c_path[1])[altDim])
                            {
                                size_t n = c_path.size();
                                double yPosB = (*c_path[1])[dim];
                                if ( (yPosB == (*c_path[0])[dim]) && 
                                        (yPosB == (*p_path[0])[dim]) )
                                {
                                    crossingFlags |= 
                                            CROSSING_SHARES_FIXED_SEGMENT;
                                }

                                double yPosE = (*c_path[n - 2])[dim];
                                if ( (yPosE == (*c_path[n - 1])[dim]) && 
                                        (yPosE == (*p_path[n - 1])[dim]) )
                                {
                                    crossingFlags |= 
                                            CROSSING_SHARES_FIXED_SEGMENT;
                                }
                            }
                        }
                    }
#endif
                }

                // {start,end}CornerSide specifies which side of conn the 
                // poly path enters and leaves.
                int startCornerSide = 1;
                int endCornerSide = 1;

                bool reversed = false;
                if (!front_same)
                {
                    // If there is a divergence at the beginning, 
                    // then order the shared path based on this.
                    startCornerSide = Avoid::cornerSide(*c_path[0], *c_path[1], 
                            *c_path[2], *p_path[0]);
                }
                if (!back_same)
                {
                    // If there is a divergence at the end of the path, 
                    // then order the shared path based on this.
                    endCornerSide = Avoid::cornerSide(*c_path[size - 3], 
                            *c_path[size - 2], *c_path[size - 1], 
                            *p_path[size - 1]);
                }
                else
                {
                    endCornerSide = startCornerSide;
                }
                if (front_same)
                {
                    startCornerSide = endCornerSide;
                }
                
                if (endCornerSide != startCornerSide)
                {
                    // Mark that the shared path crosses.
                    //db_printf("shared path crosses.\n");
                    crossingCount += 1;
                    if (crossingPoints)
                    {
                        crossingPoints->insert(*c_path[1]);
                    }
                }

                if (front_same || back_same)
                {
                    crossingFlags |= CROSSING_SHARES_PATH_AT_END;

                    // Reduce the cost of paths that stay straight after 
                    // the split, and make this length available so that the
                    // paths can be ordered during the improveCrossings()
                    // step and the straight (usually better) paths will be 
                    // left alone while the router attempts to find better
                    // paths for the others.
                    double straightModifier = 200;
                    firstSharedPathAtEndLength = secondSharedPathAtEndLength = 
                            pathLength(c_path, p_path, size);
                    if (back_same && (size > 2))
                    {
                        if (vecDir(*p_path[0], *p_path[1], *p_path[2]) == 0)
                        {
                            firstSharedPathAtEndLength -= straightModifier;
                        }

                        if (vecDir(*c_path[0], *c_path[1], *c_path[2]) == 0)
                        {
                            secondSharedPathAtEndLength -= straightModifier;
                        }
                    }
                    else if (front_same && (size > 2))
                    {
                        if (vecDir(*p_path[size - 3], *p_path[size - 2],
                                    *p_path[size - 1]) == 0)
                        {
                            firstSharedPathAtEndLength -= straightModifier;
                        }

                        if (vecDir(*c_path[size - 3], *c_path[size - 2],
                                    *c_path[size - 1]) == 0)
                        {
                            secondSharedPathAtEndLength -= straightModifier;
                        }
                    }
                }
                else if (polyIsOrthogonal && connIsOrthogonal)
                {
                    int cStartDir = vecDir(*c_path[0], *c_path[1], *c_path[2]);
                    int pStartDir = vecDir(*p_path[0], *p_path[1], *p_path[2]);
                    if ((cStartDir != 0) && (cStartDir == -pStartDir))
                    {
                        // The start segments diverge at 180 degrees to each 
                        // other.  So order based on not introducing overlap
                        // of the diverging segments when these are nudged
                        // apart.
                        startCornerSide = -cStartDir;
                    }
                    else 
                    {
                        int cEndDir = vecDir(*c_path[size - 3], 
                                *c_path[size - 2], *c_path[size - 1]);
                        int pEndDir = vecDir(*p_path[size - 3], 
                                *p_path[size - 2], *p_path[size - 1]);
                        if ((cEndDir != 0) && (cEndDir == -pEndDir))
                        {
                            // The end segments diverge at 180 degrees to 
                            // each other.  So order based on not introducing 
                            // overlap of the diverging segments when these 
                            // are nudged apart.
                            startCornerSide = -cEndDir;
                        }
                    }
                }

#if 0
                int prevTurnDir = 0;
                if (pointOrders)
                {
                    // Return the ordering for the shared path.
                    COLA_ASSERT(c_path.size() > 0 || back_same);
                    size_t adj_size = (c_path.size() - ((back_same) ? 0 : 1));
                    for (size_t i = (front_same) ? 0 : 1; i < adj_size; ++i)
                    {
                        Avoid::Point& an = *(c_path[i]);
                        Avoid::Point& bn = *(p_path[i]);
                        int currTurnDir = ((i > 0) && (i < (adj_size - 1))) ?  
                                vecDir(*c_path[i - 1], an,
                                       *c_path[i + 1]) : 0;
                        VertID vID(an.id, true, an.vn);
                        if ( (currTurnDir == (-1 * prevTurnDir)) &&
                                (currTurnDir != 0) && (prevTurnDir != 0) )
                        {
                            // The connector turns the opposite way around 
                            // this shape as the previous bend on the path,
                            // so reverse the order so that the inner path
                            // become the outer path and vice versa.
                            reversed = !reversed;
                        }
                        bool orderSwapped = (*pointOrders)[an].addOrderedPoints(
                                &bn, &an, reversed);
                        if (orderSwapped)
                        {
                            // Reverse the order for later points.
                            reversed = !reversed;
                        }
                        prevTurnDir = currTurnDir;
                    }
                }
#endif
                if (pointOrders)
                {
                    reversed = false;
                    size_t startPt = (front_same) ? 0 : 1;
                    
                    // Orthogonal should always have at least one segment.
                    COLA_ASSERT(size > (startPt + 1));

                    if (startCornerSide > 0)
                    {
                        reversed = !reversed;
                    }

                    int prevDir = 0;
                    // Return the ordering for the shared path.
                    COLA_ASSERT(size > 0 || back_same);
                    size_t adj_size = (size - ((back_same) ? 0 : 1));
                    for (size_t i = startPt; i < adj_size; ++i)
                    {
                        Avoid::Point& an = *(c_path[i]);
                        Avoid::Point& bn = *(p_path[i]);
                        COLA_ASSERT(an == bn);

                        if (i > startPt)
                        {
                            Avoid::Point& ap = *(c_path[i - 1]);
                            Avoid::Point& bp = *(p_path[i - 1]);

                            int thisDir = segDir(ap, an);
                            if (prevDir == 0)
                            {
                                if (thisDir > 0)
                                {
                                    reversed = !reversed;
                                }
                            }
                            else if (thisDir != prevDir)
                            {
                                reversed = !reversed;
                            }

                            int orientation = (ap.x == an.x) ? 0 : 1;
                            //printf("prevOri %d\n", prevOrientation);
                            //printf("1: %X, %X\n", (int) &(bn), (int) &(an));
                            (*pointOrders)[an].addOrderedPoints(
                                    orientation, 
                                    std::make_pair(&bn, polyConnRef), 
                                    std::make_pair(&an, connConnRef), 
                                    reversed);
                            COLA_ASSERT(ap == bp);
                            //printf("2: %X, %X\n", (int) &bp, (int) &ap);
                            (*pointOrders)[ap].addOrderedPoints(
                                    orientation, 
                                    std::make_pair(&bp, polyConnRef), 
                                    std::make_pair(&ap, connConnRef), 
                                    reversed);
                            prevDir = thisDir;
                        }
                    }
                }
#if 0
                    int ymod = -1;
                    if ((id.vn == 1) || (id.vn == 2))
                    {
                        // bottom.
                        ymod = +1;
                    }
                    
                    int xmod = -1;
                    if ((id.vn == 0) || (id.vn == 1))
                    {
                        // right.
                        xmod = +1;
                    }
                    if(id.vn > 3)
                    {
                        xmod = ymod = 0;
                        if (id.vn == 4)
                        {
                            // right.
                            xmod = +1;
                        }
                        else if (id.vn == 5)
                        {
                            // bottom.
                            ymod = +1;
                        }
                        else if (id.vn == 6)
                        {
                            // left.
                            xmod = -1;
                        }
                        else if (id.vn == 7)
                        {
                            // top.
                            ymod = -1;
                        }
                    }
#endif
 
                crossingFlags |= CROSSING_TOUCHES;
            }
            else if (cIndex >= 2)
            {
                // The connectors cross or touch at this point.
                //db_printf("Cross or touch at point... \n");
                
                // Crossing shouldn't be at an endpoint.
                COLA_ASSERT(cIndex >= 2);
                COLA_ASSERT(!polyIsConn || (j >= 2));

                Avoid::Point& b0 = poly.ps[(j - 2 + poly_size) % poly_size];
                Avoid::Point& a0 = conn.ps[cIndex - 2];
            
                int side1 = Avoid::cornerSide(a0, a1, a2, b0);
                int side2 = Avoid::cornerSide(a0, a1, a2, b2);
                if (side1 != side2)
                {
                    // The connectors cross at this point.
                    //db_printf("cross.\n");
                    crossingCount += 1;
                    if (crossingPoints)
                    {
                        crossingPoints->insert(a1);
                    }
                }

                crossingFlags |= CROSSING_TOUCHES;
                if (pointOrders)
                {
                    if (polyIsOrthogonal && connIsOrthogonal)
                    {
                        // Orthogonal case:
                        // Just order based on which comes from the left and
                        // top in each dimension because this can only be two
                        // L-shaped segments touching at the bend.
                        bool reversedX = ((a0.x < a1.x) || (a2.x < a1.x));
                        bool reversedY = ((a0.y < a1.y) || (a2.y < a1.y));
                        // XXX: Why do we need to invert the reversed values 
                        //      here?  Are they wrong for orthogonal points
                        //      in the other places?
                        (*pointOrders)[b1].addOrderedPoints(0, 
                                std::make_pair(&b1, polyConnRef), 
                                std::make_pair(&a1, connConnRef), 
                                !reversedX);
                        (*pointOrders)[b1].addOrderedPoints(1, 
                                std::make_pair(&b1, polyConnRef), 
                                std::make_pair(&a1, connConnRef),
                                !reversedY);
                    }
#if 0
// Unused code.
                    else
                    {
                        int turnDirA = vecDir(a0, a1, a2); 
                        int turnDirB = vecDir(b0, b1, b2); 
                        bool reversed = (side1 != -turnDirA); 
                        if (side1 != side2) 
                        { 
                            // Interesting case where a connector routes round 
                            // the edge of a shape and intersects a connector 
                            // which is connected to a port on the edge of the 
                            // shape. 
                            if (turnDirA == 0) 
                            { 
                                // We'll make B the outer by preference,  
                                // because the points of A are collinear. 
                                reversed = false; 
                            } 
                            else if (turnDirB == 0) 
                            { 
                                reversed = true; 
                            } 
                            // TODO COLA_ASSERT((turnDirB != 0) || 
                            //          (turnDirA != 0)); 
                        }
                        VertID vID(b1.id, b1.vn);
                        //(*pointOrders)[b1].addOrderedPoints(&b1, &a1, reversed);
                    }
#endif
                }
            }
        }
        else
        {
            if ( polyIsOrthogonal && connIsOrthogonal)
            {
                // All crossings in orthogonal connectors will be at a
                // vertex in the visibility graph, so we need not bother
                // doing normal line intersection.
                continue;
            }

            // No endpoint is shared between these two line segments,
            // so just calculate normal segment intersection.

            Point cPt;
            int intersectResult = Avoid::segmentIntersectPoint(
                    a1, a2, b1, b2, &(cPt.x), &(cPt.y));

            if (intersectResult == Avoid::DO_INTERSECT)
            {
                if (!polyIsConn && 
                        ((a1 == cPt) || (a2 == cPt) || (b1 == cPt) || (b2 == cPt)))
                {
                    // XXX: This shouldn't actually happen, because these
                    //      points should be added as bends to each line by
                    //      splitBranchingSegments().  Thus, lets ignore them.
                    COLA_ASSERT(a1 != cPt);
                    COLA_ASSERT(a2 != cPt);
                    COLA_ASSERT(b1 != cPt);
                    COLA_ASSERT(b2 != cPt);
                    continue;
                }                
                //db_printf("crossing lines:\n");
                //db_printf("cPt: %g %g\n", cPt.x, cPt.y);
                crossingCount += 1;
                if (crossingPoints)
                {
                    crossingPoints->insert(cPt);
                }
            }
        }
    }
    //db_printf("crossingcount %d %d\n", crossingCount, crossingFlags);

    // Free shared path memory.
    delete[] c_path;
    delete[] p_path;
}


//============================================================================

}



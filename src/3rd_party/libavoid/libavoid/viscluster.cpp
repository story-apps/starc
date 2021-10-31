/*
 * vim: ts=4 sw=4 et tw=0 wm=0
 *
 * libavoid - Fast, Incremental, Object-avoiding Line Router
 *
 * Copyright (C) 2004-2008  Monash University
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

#include "libavoid/viscluster.h"
#include "libavoid/router.h"
#include "libavoid/assertions.h"
#include "libavoid/debug.h"


namespace Avoid {


ClusterRef::ClusterRef(Router *router, Polygon& polygon, const unsigned int id)
    : m_router(router),
      m_polygon(polygon, router),
      m_rectangular_polygon(m_polygon.boundingRectPolygon()),
      m_active(false)
{
    COLA_ASSERT(m_router != nullptr);
    m_id = m_router->assignId(id);

    m_router->addCluster(this);
}


ClusterRef::~ClusterRef()
{
    if (m_router->m_currently_calling_destructors == false)
    {
        err_printf("ERROR: ClusterRef::~ClusterRef() shouldn't be called directly.\n");
        err_printf("       It is owned by the router.  Call Router::deleteCluster() instead.\n");
        abort();
    }
}


void ClusterRef::makeActive(void)
{
    COLA_ASSERT(!m_active);
    
    // Add to clusterRefs list.
    m_clusterrefs_pos = m_router->clusterRefs.insert(
            m_router->clusterRefs.begin(), this);

    m_active = true;
}


void ClusterRef::makeInactive(void)
{
    COLA_ASSERT(m_active);
    
    // Remove from clusterRefs list.
    m_router->clusterRefs.erase(m_clusterrefs_pos);

    m_active = false;
}
    

void ClusterRef::setNewPoly(Polygon& poly)
{
    m_polygon = ReferencingPolygon(poly, m_router);
    m_rectangular_polygon = m_polygon.boundingRectPolygon();
}


unsigned int ClusterRef::id(void) const
{
    return m_id;
}


ReferencingPolygon& ClusterRef::polygon(void)
{
    return m_polygon;
}


Polygon& ClusterRef::rectangularPolygon(void)
{
    return m_rectangular_polygon;
}


Router *ClusterRef::router(void) const
{
    return m_router;
}


}


